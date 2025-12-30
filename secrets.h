#ifndef SECRETS_H
#define SECRETS_H

#include <Arduino.h>

// Firmware Version
#define FIRMWARE_VERSION "Flag v25.1.0"

// Environment Configuration - Comment out ENV_HOME for production
#define ENV_HOME                     // Uncomment for home environment
#define ENV_FLAG "GuruGames2"           // Change this per device

extern const char* ssid;
extern const char* password;
extern String serialNum;

extern const String item_incrementer_url;
extern const String flag_cap_url;
extern const String game_init_url;
extern const String device_init_url;
extern const String active_minigame_url;
extern const String update_uuid_url;
extern const String access_request_url;
extern const String flag_cap_url_cached;
extern const String active_minigame_url_cached;

void configureEnvironment();

#endif // SECRETS_H
