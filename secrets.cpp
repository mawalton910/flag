#include "secrets.h"
#include <WiFi.h>

String serialNum = "";
const char* ssid = "";
const char* password = "";

// Endpoint URLs
const String item_incrementer_url = "http://iot.rke.world/iot/itemIncrementer";
const String flag_cap_url = "https://iot.rke.world/iot/flagCaptureByUuid";
const String game_init_url = "https://iot.rke.world/iot/getGameFactions";
const String device_init_url = "https://iot.rke.world/iot/initDevice";
const String active_minigame_url = "https://iot.rke.world/iot/getActiveMinigame";
const String update_uuid_url = "https://iot.rke.world/iot/updateUUID";
const String access_request_url = "https://iot.rke.world/iot/accessControl";
const String flag_cap_url_cached = "https://iot.rke.world/iotCache/flagCache";
const String active_minigame_url_cached = "https://iot.rke.world/iotCache/minigame";

struct DeviceEntry {
  const char* name;
  const char* mac_address;
  const char* prod_ssid;
};

// Master device database
DeviceEntry devices[] = {
  // ==== FLAGS ====
  { "GuruGames1", "10:06:1C:81:34:A4", "ggflag1" },
  { "GuruGames2", "10:06:1C:82:D2:24", "ggflag2" },
  { "GuruGames3", "10:06:1C:82:E1:3C", "ggflag3" },
  { "GuruGames4", "30:C9:22:39:13:E4", "ggflag4" },
  { "GuruGames5", "10:06:1C:80:B8:34", "ggflag5" },
  { "GuruGames6", "C8:2E:18:F7:BE:C0", "ggflag6" },
  { "GuruGames7", "10:06:1C:82:A5:00", "ggflag7" },

  // ==== BUY STATIONS ====
  { "gurugamesbuy1", "10:06:1C:81:34:14", "buystation1" },
  { "gurugamesbuy2", "10:06:1C:82:AD:24", "buystation2" },
  { "gurugamesbuy3", "C8:2E:18:F7:E4:48", "buystation3" },
  { "gurugamesbuy4", "30:C9:22:38:FC:AC", "buystation4" },
  { "gurugamesbuy5", "10:06:1C:80:B6:4C", "buystation5" },
  { "gurugamesbuy6", "08:B6:1F:BE:40:00", "buystation6" },
  { "gurugamesbuy7", "10:06:1C:81:20:6C", "buystation7" },
  { "gurugamesbuy8", "10:06:1C:81:44:70", "buystation8" },
  { "gurugamesbuy9", "30:C9:22:39:10:98", "buystation9" },

  // ==== SECONDARY DEVICES ====
  { "mutantwarfare2secondary1",  "10:06:1C:82:9A:A4", "secondary1" },
  { "mutantwarfare2secondary2",  "C8:2E:18:F7:E0:B4", "secondary2" },
  { "mutantwarfare2secondary3",  "10:06:1C:82:33:40", "secondary3" },
  { "mutantwarfare2secondary4",  "10:06:1C:81:29:44", "secondary4" },
  { "mutantwarfare2secondary5",  "C8:2E:18:F7:76:0C", "secondary5" },
  { "mutantwarfare2secondary6",  "C8:2E:18:F7:DB:F4", "secondary6" },
  { "mutantwarfare2secondary7",  "10:06:1C:82:FF:44", "secondary7" },
  { "mutantwarfare2secondary8",  "C8:2E:18:F7:EC:54", "secondary8" },
  { "mutantwarfare2secondary9",  "10:06:1C:82:87:3C", "secondary9" },
  { "mutantwarfare2secondary10", "10:06:1C:82:C8:B4", "secondary10" },
  { "mutantwarfare2secondary11", "C8:2E:18:F7:F4:A8", "secondary11" },
  { "mutantwarfare2secondary12", "10:06:1C:82:28:B0", "secondary12" },
  { "mutantwarfare2secondary13", "10:06:1C:81:CD:A4", "secondary13" },
  { "mutantwarfare2secondary14", "10:06:1C:82:CD:B8", "secondary14" },
  { "mutantwarfare2secondary15", "C8:2E:18:F8:25:3C", "secondary15" },
  { "mutantwarfare2secondary16", "08:B6:1F:BC:E1:40", "secondary16" },
  { "mutantwarfare2secondary17", "10:06:1C:81:4D:68", "secondary17" },
  { "mutantwarfare2secondary18", "10:06:1C:82:DE:14", "secondary18" },
  { "mutantwarfare2secondary19", "10:06:1C:82:A5:4C", "secondary19" },
  { "mutantwarfare2secondary20", "10:06:1C:82:31:3C", "secondary20" }
};

void configureEnvironment() {
  String targetDevice = ENV_FLAG;
  bool deviceFound = false;
  
  // Find the device in our database
  int deviceCount = sizeof(devices) / sizeof(devices[0]);
  for (int i = 0; i < deviceCount; i++) {
    if (targetDevice.equalsIgnoreCase(devices[i].name)) {
      serialNum = devices[i].name;
      deviceFound = true;
      
      #ifdef ENV_HOME
        // Home environment - use home WiFi for all devices
        ssid = "ESPN2G";
        password = "Alexander1985";
        Serial.println("=== HOME ENVIRONMENT ===");
        Serial.println("Device: " + serialNum);
        Serial.println("WiFi: " + String(ssid));
      #else
        // Production environment - use device-specific SSID
        ssid = devices[i].prod_ssid;
        password = "thisismywifipassword";
        Serial.println("=== PRODUCTION ENVIRONMENT ===");
        Serial.println("Device: " + serialNum);
        Serial.println("WiFi: " + String(ssid));
      #endif
      
      break;
    }
  }
  
  if (!deviceFound) {
    Serial.println("ERROR: Device '" + targetDevice + "' not found in database!");
    serialNum = "UNKNOWN_" + targetDevice;
    
    #ifdef ENV_HOME
      ssid = "ESPN2G";
      password = "Alexander1985";
    #else
      ssid = "ESPN2G";  // Fallback
      password = "thisismywifipassword";
    #endif
    
    Serial.println("Using fallback configuration");
  }
  
  Serial.println("Serial Number: " + serialNum);
  Serial.println("WiFi SSID: " + String(ssid));
}