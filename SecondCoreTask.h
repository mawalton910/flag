#ifndef SECOND_CORE_TASK_H
#define SECOND_CORE_TASK_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "NeoPixelControl.h"
#include "initSequence.h"
#include "Faction.h"
#include "freertos/semphr.h"
#include "secrets.h"

unsigned long deserializeStart = 0;
unsigned long deserializeTime = 0;
unsigned long totalExecutionTime = 0;

// Declare mutexes
SemaphoreHandle_t owningFactionMutex = NULL;
SemaphoreHandle_t secondCoreMutex = NULL;

unsigned long responseTime = 0;
bool wifiOn = true;

void initializeMutexes() {
    owningFactionMutex = xSemaphoreCreateMutex();
    secondCoreMutex = xSemaphoreCreateMutex();
}

void cleanupMutexes() {
    if (owningFactionMutex) vSemaphoreDelete(owningFactionMutex);
    if (secondCoreMutex) vSemaphoreDelete(secondCoreMutex);
}

void calculateTimeDifference(const String& currentTimeStr, const String& endTimeStr) {
    struct tm currentTime;
    struct tm endTime;
    memset(&currentTime, 0, sizeof(struct tm));
    memset(&endTime, 0, sizeof(struct tm));

    // Trim milliseconds from ISO string if present
    String trimmedEnd = endTimeStr;
    int dotIndex = trimmedEnd.indexOf('.');
    if (dotIndex != -1) {
        trimmedEnd = trimmedEnd.substring(0, dotIndex);
    }

    const char* fmtCurrent = "%a, %d %b %Y %H:%M:%S %Z";
    const char* fmtEnd = "%Y-%m-%dT%H:%M:%S";

    char currentBuffer[80];
    char endBuffer[80];
    strncpy(currentBuffer, currentTimeStr.c_str(), sizeof(currentBuffer));
    strncpy(endBuffer, trimmedEnd.c_str(), sizeof(endBuffer));

    char* currentParse = strptime(currentBuffer, fmtCurrent, &currentTime);
    char* endParse = strptime(endBuffer, fmtEnd, &endTime);

    if (!currentParse) {
        Serial.println("[ERROR] Failed to parse currentTime with format:");
        Serial.println(fmtCurrent);
    }
    if (!endParse) {
        Serial.println("[ERROR] Failed to parse endTime with format:");
        Serial.println(fmtEnd);
    }

    if (currentParse && endParse) {
        time_t currentEpoch = mktime(&currentTime);
        time_t endEpoch = mktime(&endTime);

        if (endEpoch > currentEpoch) {
            int remaining = endEpoch - currentEpoch;
            Serial.printf("==> Round time remaining: %d min, %d sec\n", remaining / 60, remaining % 60);
        } else {
            Serial.println("==> Minigame has ended.");
        }
    } else {
        Serial.println("[ERROR] One or both time strings failed to parse.");
    }
}

void secondCoreTask(void* parameter) {
    DynamicJsonDocument doc(8192);

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!wifiOn) {
                wifiOn = true;
                if (xSemaphoreTake(owningFactionMutex, portMAX_DELAY)) {
                    // On reconnect, just re-show current ownership (basic)
                    if (owningFactionId.isEmpty()) {
                        badgeSwipeBlink();
                        setStripWhite();
                        Serial.println("No owning faction. Setting pixels to white.");
                    } else {
                        Faction* owningFaction = nullptr;
                        for (int i = 0; i < factionsList.size(); i++) {
                            Faction* faction = factionsList.get(i);
                            if (faction->id == owningFactionId) { owningFaction = faction; break; }
                        }
                        if (owningFaction != nullptr) {
                            badgeSwipeBlink();
                            setStripColor(owningFaction);
                            Serial.println("Displaying owning faction color.");
                        } else {
                            badgeSwipeBlink();
                            setStripWhite();
                        }
                    }
                    xSemaphoreGive(owningFactionMutex);
                }
            }

            HTTPClient http;
            unsigned long startTime = millis();

            http.begin(active_minigame_url);
            http.setReuse(true);
            http.addHeader("Content-Type", "application/json");

            String patchData = "{\"mac_address\":\"" + WiFi.macAddress() +
                               "\", \"ip_address\":\"" + WiFi.localIP().toString() +
                               "\", \"serial_number\":\"" + serialNum +
                               "\", \"rtime\":\"" + String(responseTime) + "\"}";

            int httpCode = http.POST(patchData);
            responseTime = millis() - startTime;

            if (httpCode == 200) {
                String payload = http.getString();
                deserializeStart = millis();
                doc.clear();
                DeserializationError error = deserializeJson(doc, payload);
                deserializeTime = millis() - deserializeStart;

                if (error) {
                    Serial.printf("Failed to deserialize JSON: %s\n", error.c_str());
                    pixelFailBlink();
                    http.end();
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                    continue;
                }

                // -------------------------
                // Read POI basics
                // -------------------------
                String poiId = "";
                bool poiHasGameTargetKeyword = false;

                if (doc.containsKey("poi") && doc["poi"].is<JsonObject>()) {
                    JsonObject poiObj = doc["poi"];
                    if (poiObj.containsKey("_id") && !poiObj["_id"].isNull()) {
                        poiId = poiObj["_id"].as<String>();
                    }

                    if (poiObj.containsKey("keywords") && poiObj["keywords"].is<JsonArray>()) {
                        JsonArray kws = poiObj["keywords"].as<JsonArray>();
                        for (JsonVariant kw : kws) {
                            if (!kw.isNull() && kw.as<String>() == "GameTarget") {
                                poiHasGameTargetKeyword = true;
                                break;
                            }
                        }
                    }
                }

                // -------------------------
                // Minigame parsing (new shape)
                // -------------------------
                bool hasMinigame = (doc.containsKey("minigames") &&
                                  !doc["minigames"].isNull() &&
                                   doc["minigames"].is<JsonArray>() &&
                                   doc["minigames"].size() > 0);

                String newMinigameId = "";
                String newMinigameTypeName = "";
                String newEndTime = "";
                String serverTime = "";

                bool isHardpointMode = false;
                bool poiIsInHardpointList = false;
                bool poiIsLocked = false;

                if (hasMinigame) {
                    JsonObject mg = doc["minigames"][0];

                    if (mg.containsKey("_id") && !mg["_id"].isNull()) {
                        newMinigameId = mg["_id"].as<String>();
                    }

                    // NEW: minigameType.name
                    if (mg.containsKey("minigameType") && mg["minigameType"].is<JsonObject>()) {
                        JsonObject mgt = mg["minigameType"];
                        if (mgt.containsKey("name") && !mgt["name"].isNull()) {
                            newMinigameTypeName = mgt["name"].as<String>();
                        }
                    }

                    if (mg.containsKey("endTime") && !mg["endTime"].isNull()) {
                        newEndTime = mg["endTime"].as<String>();
                    }

                    // NEW: currentTime from server payload
                    if (doc.containsKey("currentTime") && !doc["currentTime"].isNull()) {
                        serverTime = doc["currentTime"].as<String>();
                    } else if (doc.containsKey("serverTime") && !doc["serverTime"].isNull()) {
                        // backwards compatibility
                        serverTime = doc["serverTime"].as<String>();
                    }

                    isHardpointMode = (newMinigameTypeName == "Hardpoints" || newMinigameTypeName == "Hardpoint Linear");

                    // Hardpoint list gate (specific_poi_target_list)
                    if (isHardpointMode && mg.containsKey("specific_poi_target_list") && mg["specific_poi_target_list"].is<JsonArray>()) {
                        JsonArray targets = mg["specific_poi_target_list"].as<JsonArray>();
                        for (JsonVariant t : targets) {
                            if (!t.is<JsonObject>()) continue;
                            JsonObject to = t.as<JsonObject>();
                            String targetPoi = to.containsKey("poi") && !to["poi"].isNull() ? to["poi"].as<String>() : "";
                            if (!poiId.isEmpty() && targetPoi == poiId) {
                                poiIsInHardpointList = true;
                                if (to.containsKey("locked") && !to["locked"].isNull()) {
                                    poiIsLocked = to["locked"].as<bool>();
                                }
                                break;
                            }
                        }
                    }
                }

                // -------------------------
                // Ownership check (unchanged, but using new payload)
                // -------------------------
                String newOwningFactionId = "";
                if (doc.containsKey("poi") && doc["poi"].is<JsonObject>()) {
                    JsonObject poiObject = doc["poi"];
                    if (poiObject.containsKey("fac_owner") && !poiObject["fac_owner"].isNull()) {
                        newOwningFactionId = poiObject["fac_owner"].as<String>();
                    }
                }

                // -------------------------
                // Update minigame state + timer print
                // -------------------------
                if (xSemaphoreTake(owningFactionMutex, portMAX_DELAY)) {
                    if (hasMinigame) {
                        if (currentMinigameId != newMinigameId) {
                            currentMinigameId = newMinigameId;
                            currentMinigameType = newMinigameTypeName;

                            Serial.println("Minigame ID updated: " + currentMinigameId);
                            Serial.println("Minigame Type: " + currentMinigameType);

                            newRoundBlink();
                        }

                        if (!newEndTime.isEmpty() && !serverTime.isEmpty()) {
                            calculateTimeDifference(serverTime, newEndTime);
                        }
                    } else {
                        currentMinigameId = "";
                        currentMinigameType = "";
                        Serial.println("No minigame available.");
                    }
                    xSemaphoreGive(owningFactionMutex);
                }

                // -------------------------
                // Apply POI gating + LED behavior
                // -------------------------
                // Determine active/inactive/locked for Hardpoints/Hardpoint Linear
                bool hardpointActiveFlag = false;

                if (hasMinigame && isHardpointMode) {
                    if (!poiIsInHardpointList) {
                        // Not part of minigame -> inactive
                        Serial.println("[Hardpoint] POI not in target list -> INACTIVE");
                        badgeSwipeBlink();
                        setStripInactive();
                    } else if (poiIsLocked) {
                        // Locked -> locked color
                        Serial.println("[Hardpoint] POI in list but LOCKED -> LOCKED COLOR");
                        badgeSwipeBlink();
                        setStripLocked();
                    } else {
                        // In list & unlocked: active only if GameTarget keyword exists
                        hardpointActiveFlag = poiHasGameTargetKeyword;
                        if (!hardpointActiveFlag) {
                            Serial.println("[Hardpoint] POI in list & unlocked but not GameTarget -> INACTIVE/STAGED");
                            badgeSwipeBlink();
                            setStripInactive();
                        }
                    }
                }

                // Standard behavior:
                // - if hardpointActiveFlag is true -> proceed with normal ownership display
                // - if NOT hardpoint mode -> proceed with normal ownership display
                bool shouldDoStandardOwnershipDisplay = (!hasMinigame) ? false : true;
                if (hasMinigame && isHardpointMode) {
                    shouldDoStandardOwnershipDisplay = hardpointActiveFlag;
                }

                // If no minigame, keep your existing "idle" behavior
                if (!hasMinigame) {
                    if (xSemaphoreTake(owningFactionMutex, portMAX_DELAY)) {
                        badgeSwipeBlink();
                        setStripBlueWhiteRed(); // your existing idle pattern
                        xSemaphoreGive(owningFactionMutex);
                    }
                    http.end();
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                    continue;
                }

                // Ownership update + display (only if allowed by gating)
                if (xSemaphoreTake(owningFactionMutex, portMAX_DELAY)) {
                    if (owningFactionId != newOwningFactionId) {
                        owningFactionId = newOwningFactionId;
                        Serial.println("Owning faction updated: " + owningFactionId);
                    }

                    if (shouldDoStandardOwnershipDisplay) {
                        if (owningFactionId.isEmpty()) {
                            setStripWhite();
                            Serial.println("Active flag with no owner. White.");
                        } else {
                            Faction* owningFaction = nullptr;
                            for (int i = 0; i < factionsList.size(); i++) {
                                Faction* faction = factionsList.get(i);
                                if (faction->id == owningFactionId) { owningFaction = faction; break; }
                            }
                            if (owningFaction != nullptr) {
                                setStripColor(owningFaction);
                                Serial.println("Active flag showing owning faction color.");
                            } else {
                                setStripWhite();
                                Serial.println("Active flag owner unknown. White.");
                            }
                        }
                    } else {
                        // In hardpoint mode, if we get here, we've already set inactive/locked above.
                        // In other modes, if you ever want a "non-standard" state, you can handle it here.
                    }

                    xSemaphoreGive(owningFactionMutex);
                }

                http.end();
                totalExecutionTime = millis() - startTime;
                Serial.printf("Response time: %.2f seconds\n", responseTime / 1000.0);

            } else {
                Serial.printf("HTTP POST failed with code: %d\n", httpCode);
                pixelFailBlink();
                setStripRedWhiteBlue();
                wifiOn = false;
            }

            vTaskDelay(5000 / portTICK_PERIOD_MS);

        } else {
            Serial.println("WiFi disconnected");
            pixelFailBlink();
            setStripRedWhiteBlue();
            wifiOn = false;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    doc.clear();
}



#endif // SECOND_CORE_TASK_H
