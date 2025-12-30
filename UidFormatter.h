#ifndef UID_FORMATTER_H
#define UID_FORMATTER_H

#include <Arduino.h>

inline String formatUid(byte *uid, byte uidLength) {
    String result = "";
    for (byte i = 0; i < uidLength; i++) {
        if (uid[i] < 0x10) result += "0";  // Zero-padding
        result += String(uid[i], HEX);
    }
    result.toUpperCase(); // Ensure consistent uppercase output
    return result;
}

#endif // UID_FORMATTER_H
