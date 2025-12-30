#ifndef FLAG_CAP_REQUEST_H
#define FLAG_CAP_REQUEST_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "NeoPixelControl.h"
#include "secrets.h"
#include "initSequence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "Faction.h"

extern SemaphoreHandle_t owningFactionMutex;
extern bool isShimmering;
TaskHandle_t shimmerTaskHandle = NULL;

void shimmerTask(void* parameter) {
    while (true) {
        if (isShimmering) {
            setRandomPixelColors();
            vTaskDelay(75 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

void startShimmerTask() {
    if (shimmerTaskHandle == NULL) {
        xTaskCreatePinnedToCore(
            shimmerTask,
            "ShimmerTask",
            2048,
            NULL,
            1,
            &shimmerTaskHandle,
            1
        );
    }
}

String sanitizeUuid(String uuid) {
    uuid.trim();
    uuid.replace(" ", "");
    uuid.toUpperCase();
    return uuid;
}

// =========================
// FLAG CAPTURE
// =========================
void sendFlagCaptureRequest(String lastUuid) {
    
    lastUuid = sanitizeUuid(lastUuid);
    Serial.println("Sending flag capture request");
    isShimmering = true;

    String previousFactionId;
    if (xSemaphoreTake(owningFactionMutex, portMAX_DELAY)) {
        previousFactionId = owningFactionId;
        xSemaphoreGive(owningFactionMutex);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        pixelFailBlink();
        isShimmering = false;
        return;
    }

    HTTPClient http;
    http.begin(flag_cap_url);
    http.addHeader("Content-Type", "application/json");

    String patchData = "{\"mac_address\":\"" + WiFi.macAddress() +
                       "\", \"serial_number\":\"" + serialNum +
                       "\", \"uuid\":\"" + lastUuid + "\"}";

    int httpCode = http.POST(patchData);
    String payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);

    String newFacId = "";
    if (httpCode == 200) {
        StaticJsonDocument<768> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc.containsKey("poi")) {
            JsonObject poi = doc["poi"];
            if (poi.containsKey("fac_owner") && !poi["fac_owner"].isNull()) {
                newFacId = poi["fac_owner"].as<String>();
            }
        }
    } else {
        Serial.printf("HTTP Error %d\n", httpCode);
        pixelFailBlink();
    }

    if (xSemaphoreTake(owningFactionMutex, portMAX_DELAY)) {
        owningFactionId = newFacId.isEmpty() ? previousFactionId : newFacId;

        Faction* factionToDisplay = nullptr;
        for (int i = 0; i < factionsList.size(); i++) {
            Faction* faction = factionsList.get(i);
            if (faction->id == owningFactionId) {
                factionToDisplay = faction;
                break;
            }
        }

        if (factionToDisplay) {
            fadeToFaction(factionToDisplay);
            Serial.println("Restored to owning faction color");
        } else {
            setStripWhite();
            Serial.println("No owning faction color found; set to white");
        }

        xSemaphoreGive(owningFactionMutex);
    }

    http.end();
    isShimmering = false;
}

// =========================
// RELAY DEVICE
// =========================
void sendPatchRequest(String lastUuid) {
    lastUuid = sanitizeUuid(lastUuid);
    isShimmering = true;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error in WiFi connection");
        isShimmering = false;
        return;
    }

    HTTPClient http;
    http.begin(update_uuid_url);
    http.addHeader("Content-Type", "application/json");

    String patchData = "{\"mac_address\":\"" + WiFi.macAddress() +
                       "\", \"ip_address\":\"" + WiFi.localIP().toString() +
                       "\", \"serial_number\":\"" + serialNum +
                       "\", \"last_uuid\":\"" + lastUuid + "\"}";

    int httpCode = http.POST(patchData);
    String payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);
    http.end();

    setStripWhite(); // immediately end shimmer with fallback color
    isShimmering = false;
}

// =========================
// ACCESS DEVICE
// =========================
void sendAccessRequest(String lastUuid) {
    lastUuid = sanitizeUuid(lastUuid);
    isShimmering = true;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error in WiFi connection");
        isShimmering = false;
        return;
    }

    HTTPClient http;
    http.begin(access_request_url);
    http.addHeader("Content-Type", "application/json");

    String patchData = "{\"mac_address\":\"" + WiFi.macAddress() +
                       "\", \"ip_address\":\"" + WiFi.localIP().toString() +
                       "\", \"serial_number\":\"" + serialNum +
                       "\", \"uuid\":\"" + lastUuid + "\"}";

    int httpCode = http.POST(patchData);
    String payload = http.getString();

    Serial.println(httpCode);
    Serial.println(payload);
    http.end();

    if (httpCode == 200) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            bool access = doc["access"];
            String access_ip = doc["access_ip"] | "";
            Serial.printf("Access: %d | IP: %s\n", access, access_ip.c_str());

            if (access) {
                HTTPClient unlockClient;
                String unlockUrl = "http://" + access_ip + "/unlock";
                unlockClient.begin(unlockUrl);
                int unlockCode = unlockClient.GET();
                String unlockResponse = unlockClient.getString();
                Serial.printf("Unlock: %d | %s\n", unlockCode, unlockResponse.c_str());
                unlockClient.end();
            }
        } else {
            Serial.print("JSON parse failed: ");
            Serial.println(error.c_str());
        }
    }

    setStripWhite(); // fallback color
    isShimmering = false;
}

#endif // FLAG_CAP_REQUEST_H
