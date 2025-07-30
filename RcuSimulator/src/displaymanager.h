#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "globals.h"

#ifdef USE_DISPLAY
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address - typical for 128x64 OLED
// Function to update display with current status
#endif

class DisplayManager {
    public:
        DisplayManager();
        bool begin();
        bool hasDisplay() const {return displayInitialized; } 
        void setHeadline(const String& text);
        void setLine(uint8_t lineNumber, const String& text);
        void setLinesAndRender(const String& line0, const String& line1 = "", const String& line2 = "", const String& line3 = "");
        void clearDisplay();
        void render();

    private:
#ifdef USE_DISPLAY
        Adafruit_SSD1306 display;
#endif
        bool displayInitialized = false;
        String headline;
        std::vector<String> lines;
        const uint8_t lineHeight = 10;
        const uint8_t maxLines = 2;

        void clearLineArea(uint8_t lineNumber);
};

#endif // DISPLAY_H