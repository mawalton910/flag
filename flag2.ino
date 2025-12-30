#include <WiFi.h>
#include <Arduino.h>
#include <string>
#include "rfid.h"
#include "UidFormatter.h"
#include "NeoPixelControl.h"
#include "initSequence.h"
#include "SecondCoreTask.h"
#include "FlagCapRequest.h"
#include "OTAUpdate.h"

RFID rfid;
TaskHandle_t secondCoreTaskHandle = NULL;

char currentMode[16] = "";

unsigned long lastBadgeScan = 0;
const unsigned long debounceInterval = 1000;
String lastSeenUid = "";

bool isShimmering = false;
unsigned long shimmerLastUpdate = 0;
const unsigned long shimmerInterval = 75; // ms

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Device Starting...");

    initializeMutexes();
    initializeRFID(rfid);
    initializeNeoPixels();
    initializeWiFiByRssi();
    //initializeWiFiSingle();
    postDeviceInit();
    postGameInit();

    xTaskCreatePinnedToCore(
        secondCoreTask,
        "SecondCoreTask",
        10000,
        NULL,
        1,
        &secondCoreTaskHandle,
        0
    );
}
void loop() {
    char tempMode[16];
    unsigned long now = millis();
    strncpy(tempMode, mode.c_str(), sizeof(tempMode) - 1);

    // shimmer animation frame
    if (isShimmering && (now - shimmerLastUpdate > 30)) {
        shimmerLastUpdate = now;
        shimmerFrame();
    }

    if (strcmp(currentMode, tempMode) != 0) {
        strncpy(currentMode, tempMode, sizeof(currentMode) - 1);
        Serial.print("Updated Mode: "); Serial.println(currentMode);
        Serial.print("Current Minigame Id: "); Serial.println(currentMinigameId);
        Serial.print("Current Minigame Type: "); Serial.println(currentMinigameType);
        Serial.print("Owning Faction Id: "); Serial.println(owningFactionId);
    }

    if (rfid.isCardPresent()) {
        String uidStr = rfid.readCardUID();

        if ((now - lastBadgeScan < debounceInterval) && uidStr == lastSeenUid) {
            return;
        }

        lastBadgeScan = now;
        lastSeenUid = uidStr;

        Serial.println("Scanned UID: " + uidStr);
        
        // Check for OTA update trigger
        if (isOTAUpdateTag(uidStr)) {
            Serial.println("OTA trigger detected!");
            triggerGitHubUpdate();
            return;  // Will restart after update, so no need to continue
        }
        
        badgeSwipeBlink();

        if (strcmp(currentMode, "Flag") == 0) {
            sendFlagCaptureRequest(uidStr);
        } else if (strcmp(currentMode, "Relay") == 0) {
            sendPatchRequest(uidStr);
        } else if (strcmp(currentMode, "Access") == 0) {
            sendAccessRequest(uidStr);
        } else {
            setRandomPixelColors();
        }

        // Only update color if not shimmering (ownership change handled by background or request)
        if (!isShimmering && !owningFactionId.isEmpty()) {
            for (int i = 0; i < factionsList.size(); i++) {
                Faction* faction = factionsList.get(i);
                if (faction->id == owningFactionId) {
                    setStripColor(faction);
                    Serial.println("Setting flag to owning faction color.");
                    return;
                }
            }
        }

        if (!isShimmering) {
            setStripWhite();
            Serial.println("Setting flag to white.");
        }
    }
}