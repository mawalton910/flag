// initSequence.h
#ifndef INIT_SEQUENCE_H
#define INIT_SEQUENCE_H

#include <WiFi.h>
#include <Arduino.h>
#include "rfid.h"
#include "NeoPixelControl.h"
#include "secrets.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LinkedList.h>

String mac = "";
String ip = "";
String postData;
int httpCode;
String payload;
DynamicJsonDocument doc(2048);
String mode = "";
String currentMinigameId = "";
String currentMinigameType = "";
String owningFactionId = "";
LinkedList<Faction*> factionsList;
TaskHandle_t activeMinigameTaskHandle = NULL;

void initializeRFID(RFID &rfid) {
    Serial.println("Initializing RFID system...");    
    rfid.initialize();
}

void initializeNeoPixels() {
    Serial.println("Initializing RGB Strip");
    initNeoPixels();
    newRoundBlink();
    setStripOff();
}

void initializeWiFiSingle() {
    Serial.println("Beginning Wifi Connection...");
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        setRandomPixelColors();
    }
    Serial.println("Connected to WiFi");  
    mac = WiFi.macAddress();
    Serial.println("MAC: " + mac);
    Serial.println("Serial Number: " + serialNum);
    ip = WiFi.localIP().toString();
    Serial.println("IP: " + ip);  
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    newRoundBlink();
    setStripOff();
}

/*
void initializeWiFiByRssi() {
    Serial.println("Scanning for WiFi networks...");
    int n = WiFi.scanNetworks();

    if (n == 0) {
        Serial.println("No networks found. Retrying...");
        delay(1000);
        initializeWiFiByRssi(); // Recursive retry
        return;
    }

    int bestRSSI = -1000;
    int bestNetworkIndex = -1;

    for (int i = 0; i < n; ++i) {
        String ssidFound = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);

        Serial.printf("Found SSID: %s, RSSI: %d\n", ssidFound.c_str(), rssi);

        if (ssidFound == ssid && rssi > bestRSSI) {
            bestRSSI = rssi;
            bestNetworkIndex = i;
        }
    }

    if (bestNetworkIndex == -1) {
        Serial.println("Desired SSID not found. Retrying...");
        delay(1000);
        initializeWiFiByRssi(); // Retry
        return;
    }

    Serial.printf("Connecting to best AP with RSSI %d...\n", bestRSSI);
    WiFi.begin(ssid, password, WiFi.channel(bestNetworkIndex), WiFi.BSSID(bestNetworkIndex));

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(250);
        setRandomPixelColors();
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect. Retrying...");
        delay(1000);
        initializeWiFiByRssi(); // Retry
        return;
    }

    Serial.println("Connected to WiFi");  
    mac = WiFi.macAddress();
    Serial.println("MAC: " + mac);
    Serial.println("Serial Number: " + serialNum);
    ip = WiFi.localIP().toString();
    Serial.println("IP: " + ip);  
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    newRoundBlink();
    setStripOff();
}*/

void initializeWiFiByRssi() {
  Serial.println("WiFi: starting scan/connect");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                // no power-save
  WiFi.disconnect(true, true);         // drop old connection & erase from RAM
  delay(100);

  const uint8_t MAX_SCAN_RETRIES = 8;  // ~8 * (scan time + backoff)
  uint8_t scanTry = 0;

  while (scanTry < MAX_SCAN_RETRIES) {
    Serial.printf("WiFi: scanning (try %u/%u)...\n", scanTry + 1, MAX_SCAN_RETRIES);

    int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
    if (n <= 0) {
      Serial.println("WiFi: no networks found");
    } else {
      int bestIdx = -1;
      int bestRSSI = -1000;

      for (int i = 0; i < n; ++i) {
        String ssidFound = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        int ch   = WiFi.channel(i);
        uint8_t* bssidPtr = WiFi.BSSID(i);

        char bssidStr[20];
        if (bssidPtr) sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                              bssidPtr[0], bssidPtr[1], bssidPtr[2],
                              bssidPtr[3], bssidPtr[4], bssidPtr[5]);
        else strcpy(bssidStr, "??:??:??:??:??:??");

        Serial.printf("  SSID: %s  RSSI: %d dBm  CH: %d  BSSID: %s\n",
                      ssidFound.c_str(), rssi, ch, bssidStr);

        // pick strongest AP that matches your target SSID (from secrets.h)
        if (ssidFound == ssid && rssi > bestRSSI) {
          bestRSSI = rssi;
          bestIdx  = i;
        }
      }

      // Prefer BSSID-specific connect to the best AP. If not seen, fallback.
      if (bestIdx >= 0) {
        int ch = WiFi.channel(bestIdx);
        uint8_t* bssid = WiFi.BSSID(bestIdx);
        Serial.printf("WiFi: connecting to '%s' via best AP (RSSI %d, CH %d)...\n",
                      ssid, bestRSSI, ch);
        WiFi.begin(ssid, password, ch, bssid);
      } else {
        Serial.println("WiFi: desired SSID not found; falling back to generic connect");
        WiFi.begin(ssid, password);
      }

      // Wait up to ~7 seconds total
      const uint32_t tStart = millis();
      while (WiFi.status() != WL_CONNECTED && (millis() - tStart) < 7000) {
        delay(100);
        // keep LEDs modest here to avoid flooding CPU
        shimmerFrame(); // if you want a tiny animation; keep it light
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi: connected!");
        Serial.print("MAC: "); Serial.println(WiFi.macAddress());
        Serial.print("IP : "); Serial.println(WiFi.localIP());
        WiFi.setAutoReconnect(true);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        // Optional deep radio diag:
        WiFi.printDiag(Serial);
        // esp_wifi_set_max_tx_power(84); // 84 -> ~20dBm, if you want explicit
        mac = WiFi.macAddress();
        ip = WiFi.localIP().toString();
        newRoundBlink();
        setStripOff();
        return; // success
      } else {
        Serial.println("WiFi: connect timeout");
        WiFi.disconnect(true, true);
      }
    }

    // Exponential-ish backoff between scans. Keep CPU free.
    uint16_t backoffMs = 400 + (scanTry * 250);
    Serial.printf("WiFi: retrying in %u ms...\n", backoffMs);
    uint32_t until = millis() + backoffMs;
    while (millis() < until) {
      delay(50);
      // tiny idle animation if you like
    }
    scanTry++;
  }

  Serial.println("WiFi: FAILED after retries");
  // Show a clear LED failure state and keep running offline  
}

void handleDeviceInitError() {
    pixelFailBlink();
    for (int i = 0; i < NUMPIXELS / 2; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    }
    for (int i = NUMPIXELS / 2; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 0));
    }
    pixels.show();
}

void handleInitializationSuccess() {
    pixelSuccessBlink();
}

void postDeviceInit() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        http.begin(device_init_url);
        http.addHeader("Content-Type", "application/json");

        postData = "{\"mac_address\":\"" + mac + "\", \"ip_address\":\"" + ip + "\", \"serial_number\":\"" + serialNum + "\"}";

        httpCode = http.POST(postData);
        payload = http.getString();
        Serial.println(payload);

        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            String message = doc["message"].as<String>();
            JsonVariant result = doc["result"];

            if (!result.isNull() && result.containsKey("keywords")) {
                JsonArray keywords = result["keywords"].as<JsonArray>();
                for (JsonVariant v : keywords) {
                    String keyword = v.as<String>();
                    if (keyword == "Flag") {
                        mode = keyword;
                        Serial.println("Mode set to: " + mode);
                        break;
                    }
                    if (keyword == "Relay") {
                        mode = keyword;
                        Serial.println("Mode set to: " + mode);
                        break;
                    }
                    if (keyword == "Access") {
                        mode = keyword;
                        Serial.println("Mode set to: " + mode);
                        break;
                    }
                    if (keyword == "Lock") {
                        mode = keyword;
                        Serial.println("Mode set to: " + mode);
                        break;
                    }                                                            
                }
            }

            JsonVariant widget = doc["widget"];
            if (httpCode == 404 || message == "Error finding associated widget" || widget.isNull()) {
                handleDeviceInitError();                
            } else {
                handleInitializationSuccess();
            }
        } else {
            handleDeviceInitError();
        }

        http.end();
    } else {
        Serial.println("Error in WiFi connection");
    }
}

void postGameInit() {
    if (WiFi.status() == WL_CONNECTED) {    
        HTTPClient http;
        http.begin(game_init_url);
        http.addHeader("Content-Type", "application/json");
        String patchData = "{\"mac_address\":\"" + WiFi.macAddress() + "\", \"ip_address\":\"" + WiFi.localIP().toString() + "\", \"serial_number\":\"" + serialNum + "\"}";
        httpCode = http.POST(patchData);
        payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
        http.end();
        setStripWhite();

        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            pixelFailBlink();
            return;
        }

        // Parse and store the minigame _id and type name
        if (doc.containsKey("minigames") && doc["minigames"].is<JsonArray>()) {
            JsonArray minigamesArray = doc["minigames"];
            if (!minigamesArray.isNull() && minigamesArray.size() > 0) {
                JsonObject firstMinigame = minigamesArray[0];
                if (firstMinigame.containsKey("_id")) {
                    currentMinigameId = firstMinigame["_id"].as<String>();
                }
                if (firstMinigame.containsKey("minigameType") && firstMinigame["minigameType"].is<JsonObject>()) {
                    JsonObject minigameType = firstMinigame["minigameType"];
                    if (minigameType.containsKey("name")) {
                        currentMinigameType = minigameType["name"].as<String>();
                    }
                }
            }
        }
        Serial.println("Minigame ID: " + currentMinigameId);
        Serial.println("Minigame Type Name: " + currentMinigameType);

        // Parse and store the fac_owner _id from POI or default to null
        if (doc.containsKey("poi") && doc["poi"].is<JsonObject>()) {
            JsonObject poiObject = doc["poi"];
            if (poiObject.containsKey("fac_owner") && !poiObject["fac_owner"].isNull()) {
                owningFactionId = poiObject["fac_owner"].as<String>();
            }
        }
        Serial.println("Fac Owner ID: " + owningFactionId);

        // Parse and display factions, keeping track of the owning faction
        Faction* owningFaction = nullptr;
        if (doc.containsKey("factions") && doc["factions"].is<JsonArray>()) {
            JsonArray factionsArray = doc["factions"];
            factionsList.clear();
            for (JsonVariant v : factionsArray) {
                Faction* faction = new Faction();
                faction->id = v["_id"].as<String>();
                faction->colorCode = v["colorCode"].as<String>();
                factionsList.add(faction);
                Serial.println("Faction ID: " + faction->id);
                Serial.println("Faction Color: " + faction->colorCode);
                fadeToFaction(faction);
                delay(250);

                // Check if this is the owning faction
                if (faction->id == owningFactionId) {
                    owningFaction = faction;
                }
            }
            badgeSwipeBlink(); 
        }

        // Display the owning faction's color at the end of the sequence
        if (owningFaction != nullptr) {
            Serial.println("Displaying owning faction color");
            setStripColor(owningFaction);
        } else {
            setStripWhite();
            Serial.println("No owning faction found");
        }
        Serial.println("Game Initialization Complete");
    } else {
        Serial.println("Error in WiFi connection");
    }
}

#endif // INIT_SEQUENCE_H
