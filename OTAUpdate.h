// OTAUpdate.h - Remote GitHub OTA Update for ESP32
#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <esp_task_wdt.h>

// ============================================
// OTA UPDATE CONFIGURATION
// ============================================

// Special RFID UIDs that trigger OTA update (add your OTA trigger tags here)
const String OTA_TRIGGER_UIDS[] = {
    "04B6C5D65F6180",      // OTA trigger tag
    // Add more OTA trigger tags as needed
};
const int OTA_TRIGGER_UIDS_COUNT = sizeof(OTA_TRIGGER_UIDS) / sizeof(OTA_TRIGGER_UIDS[0]);

// GitHub OTA settings
const char* OTA_WIFI_SSID = "ESPN2G";           // WiFi SSID for OTA updates
const char* OTA_WIFI_PASSWORD = "Alexander1985"; // WiFi password for OTA updates
String targetBranch = "main";                     // GitHub branch to download from

// GitHub repository info
const char* GITHUB_USER = "mawalton910";
const char* GITHUB_REPO = "flag";
const char* FIRMWARE_FILENAME = "update.ino.bin";

// Build GitHub URL dynamically
String getOTAUrl() {
    return "https://raw.githubusercontent.com/" + 
           String(GITHUB_USER) + "/" + 
           String(GITHUB_REPO) + "/" + 
           targetBranch + "/" + 
           String(FIRMWARE_FILENAME);
}

// ============================================
// OTA PROGRESS DISPLAY (Serial only for ESP32)
// ============================================

void displayOTAProgress(int progress, int total) {
    // Calculate percentage
    int percentage = (progress * 100) / total;
    
    // Display progress on Serial
    Serial.printf("OTA Progress: %d%% (%d KB / %d KB)\n", 
                  percentage, 
                  progress / 1024, 
                  total / 1024);
}

void displayOTAStatus(String message) {
    Serial.println("OTA Status: " + message);
}

// ============================================
// OTA UPDATE CORE FUNCTION
// ============================================

void triggerGitHubUpdate() {
    Serial.println("=== OTA UPDATE TRIGGERED ===");
    
    // Disable watchdog timer during update
    esp_task_wdt_deinit();
    
    // Display connecting message
    displayOTAStatus("CONNECTING TO OTA WIFI...");
    
    // Disconnect from current WiFi
    WiFi.disconnect(true);
    delay(500);
    
    // Connect to OTA WiFi
    WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PASSWORD);
    Serial.println("Connecting to OTA WiFi: " + String(OTA_WIFI_SSID));
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        displayOTAStatus("WIFI FAILED");
        Serial.println("\nOTA WiFi connection failed!");
        delay(3000);
        ESP.restart();
        return;
    }
    
    Serial.println("\nOTA WiFi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    
    // Build firmware URL
    String firmwareUrl = getOTAUrl();
    Serial.println("Firmware URL: " + firmwareUrl);
    
    displayOTAStatus("PREPARING UPDATE...");
    delay(1000);
    
    // Configure WiFiClientSecure
    WiFiClientSecure client;
    client.setInsecure();  // Bypass certificate validation (GitHub SSL)
    
    // Set up progress callback
    httpUpdate.onProgress([](int current, int total) {
        displayOTAProgress(current, total);
    });
    
    // Perform the update
    Serial.println("Starting HTTP Update...");
    displayOTAStatus("DOWNLOADING FIRMWARE...");
    
    t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);
    
    // Handle update result
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("Update FAILED (%d): %s\n", 
                         httpUpdate.getLastError(), 
                         httpUpdate.getLastErrorString().c_str());
            displayOTAStatus("UPDATE FAILED: " + httpUpdate.getLastErrorString());
            delay(5000);
            ESP.restart();
            break;
            
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No updates available");
            displayOTAStatus("NO UPDATE AVAILABLE");
            delay(3000);
            ESP.restart();
            break;
            
        case HTTP_UPDATE_OK:
            Serial.println("Update SUCCESS!");
            displayOTAStatus("UPDATE SUCCESS! REBOOTING...");
            delay(2000);
            ESP.restart();  // This will reboot into the new firmware
            break;
    }
}

// ============================================
// OTA TRIGGER CHECK FUNCTION
// ============================================

bool isOTAUpdateTag(String uuid) {
    // Check if scanned UUID matches any OTA trigger tags
    for (int i = 0; i < OTA_TRIGGER_UIDS_COUNT; i++) {
        if (uuid == OTA_TRIGGER_UIDS[i]) {
            return true;
        }
    }
    return false;
}

// ============================================
// INTEGRATION HELPER
// ============================================
// Add this check to your loop() after reading RFID card:
//
// if (rfid.isCardPresent()) {
//     String uidStr = rfid.readCardUID();
//     if (isOTAUpdateTag(uidStr)) {
//         triggerGitHubUpdate();
//     }
//     // ... rest of your RFID logic
// }

#endif // OTA_UPDATE_H
