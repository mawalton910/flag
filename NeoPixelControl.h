#ifndef NEOPIXELCONTROL_H
#define NEOPIXELCONTROL_H

#include <Adafruit_NeoPixel.h>
#include "Faction.h"

#define NEOPIXEL_PIN 14
#define NUMPIXELS    30

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
extern bool isShimmering; // <-- add this line

void initNeoPixels() {
    pixels.begin();
    pixels.setBrightness(255);
    pixels.show();
}

void setRandomPixelColors() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(rand() % 256, rand() % 256, rand() % 256));
    }
    pixels.show();
}

void setStripColor(Faction* faction) {
    int red = strtol(faction->colorCode.substring(1, 3).c_str(), NULL, 16);
    int green = strtol(faction->colorCode.substring(3, 5).c_str(), NULL, 16);
    int blue = strtol(faction->colorCode.substring(5, 7).c_str(), NULL, 16);
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(red, green, blue));
    }
    pixels.show();
}

void setStripOff() {
    pixels.clear();
    pixels.show();
}

void setStripWhite() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
    pixels.show();
}

void setStripRed() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    }
    pixels.show();
}

void setStripYellow() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 0));
    }
    pixels.show();
}

void setStripGreen() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    }
    pixels.show();
}

// ~~~~~ Optimized Animations Below ~~~~~

void pixelSuccessBlink() {
    for (int i = 0; i < 2; i++) {
        setStripOff();
        delay(75);
        setStripGreen();
        delay(75);
    }
    setStripOff();
}

void pixelFailBlink() {
    for (int i = 0; i < 2; i++) {
        setStripOff();
        delay(100);
        setStripRed();
        delay(100);
    }
    setStripOff();
}

void newRoundBlink() {
    for (int i = 0; i < 6; i++) {
        setRandomPixelColors();
        delay(50);
        setStripOff();
        delay(20);
    }    
}

void badgeSwipeBlink() {
    for (int i = 0; i < 4; i++) {
        setRandomPixelColors();
        delay(30);
        setStripOff();
        delay(20);
        setRandomPixelColors();        
    }
}

void setStripFactionColors(LinkedList<Faction*> factions) {
    if (factions.size() == 0) {
        setStripOff();
        return;
    }

    int ledsPerFaction = NUMPIXELS / factions.size();
    int remainder = NUMPIXELS % factions.size();
    int currentLED = 0;

    for (int i = 0; i < factions.size(); i++) {
        Faction* faction = factions.get(i);
        int red = strtol(faction->colorCode.substring(1, 3).c_str(), NULL, 16);
        int green = strtol(faction->colorCode.substring(3, 5).c_str(), NULL, 16);
        int blue = strtol(faction->colorCode.substring(5, 7).c_str(), NULL, 16);
        int count = ledsPerFaction + (remainder > 0 ? 1 : 0);
        if (remainder > 0) remainder--;

        for (int j = 0; j < count && currentLED < NUMPIXELS; j++, currentLED++) {
            pixels.setPixelColor(currentLED, pixels.Color(red, green, blue));
        }
    }

    pixels.show();
}

void setStripRedWhiteBlue() {
    int sectionSize = NUMPIXELS / 3;

    for (int i = 0; i < sectionSize; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    }
    for (int i = sectionSize; i < sectionSize * 2; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
    for (int i = sectionSize * 2; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    }

    pixels.show();
}

void setStripBlueWhiteRed() {
    int sectionSize = NUMPIXELS / 3;

    for (int i = 0; i < sectionSize; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    }
    for (int i = sectionSize; i < sectionSize * 2; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
    for (int i = sectionSize * 2; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    }

    pixels.show();
}


// Shimmer animation (simple quick feedback)
void shimmerEffect() {
    static uint8_t phase = 0;
    phase = (phase + 1) % 256;

    for (int i = 0; i < NUMPIXELS; i++) {
        int brightness = 32 + (sin((i * 16 + phase) * 0.1) * 96);
        pixels.setPixelColor(i, pixels.Color(brightness, brightness, brightness));
    }
    pixels.show();
}

void shimmerFrame() {
    if (!isShimmering) return;  // only shimmer if active

    static uint8_t hue = 0;
    hue += 4; // shift the hue every frame

    for (int i = 0; i < NUMPIXELS; i++) {
        uint32_t color = pixels.ColorHSV(hue * 256, 255, 80); // dim HSV shimmer
        pixels.setPixelColor(i, color);
    }
    pixels.show();
}

void fadeToColor(uint8_t targetR, uint8_t targetG, uint8_t targetB, int steps = 20, int delayMs = 20) {
    for (int i = 0; i < NUMPIXELS; i++) {
        uint32_t current = pixels.getPixelColor(i);
        uint8_t currR = (current >> 16) & 0xFF;
        uint8_t currG = (current >> 8) & 0xFF;
        uint8_t currB = current & 0xFF;

        // Store start color for pixel
        for (int step = 0; step <= steps; step++) {
            uint8_t newR = currR + (targetR - currR) * step / steps;
            uint8_t newG = currG + (targetG - currG) * step / steps;
            uint8_t newB = currB + (targetB - currB) * step / steps;

            pixels.setPixelColor(i, pixels.Color(newR, newG, newB));
        }
    }

    for (int step = 0; step <= steps; step++) {
        for (int i = 0; i < NUMPIXELS; i++) {
            uint32_t current = pixels.getPixelColor(i);
            uint8_t currR = (current >> 16) & 0xFF;
            uint8_t currG = (current >> 8) & 0xFF;
            uint8_t currB = current & 0xFF;

            uint8_t newR = currR + (targetR - currR) * step / steps;
            uint8_t newG = currG + (targetG - currG) * step / steps;
            uint8_t newB = currB + (targetB - currB) * step / steps;

            pixels.setPixelColor(i, pixels.Color(newR, newG, newB));
        }
        pixels.show();
        delay(delayMs);
    }
}

void fadeToFaction(Faction* faction) {
    String colorCode = faction->colorCode;
    uint8_t r = strtol(colorCode.substring(1, 3).c_str(), NULL, 16);
    uint8_t g = strtol(colorCode.substring(3, 5).c_str(), NULL, 16);
    uint8_t b = strtol(colorCode.substring(5, 7).c_str(), NULL, 16);
    fadeToColor(r, g, b);
}

void flashWhiteThenFadeToFaction(Faction* faction, int whiteMs = 150, int fadeSteps = 20, int fadeDelay = 20) {
    // Step 1: Flash white briefly
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
    pixels.show();
    delay(whiteMs);

    // Step 2: Fade into faction color
    fadeToFaction(faction);
}

void setStripInactive() {
    // Inactive (not in hardpoint list): dim purple
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(40, 0, 60));
    }
    pixels.show();
}

void setStripLocked() {
    // Locked: amber/orange
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 80, 0));
    }
    pixels.show();
}


#endif
