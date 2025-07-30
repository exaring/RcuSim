#include "displaymanager.h"

DisplayManager::DisplayManager(){
    lines.resize(maxLines);
}

bool DisplayManager::begin() {
    #ifdef USE_DISPLAY
    // Initialize the OLED display
    if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        displayInitialized = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.display();
        return true;
    }
    #endif
    return false;
}

void DisplayManager::setHeadline(const String& text) {
    headline = text;
}

void DisplayManager::setLine(uint8_t lineNumber, const String& text) {
    if (lineNumber >= maxLines) return;

    if (lines[lineNumber] != text) {
        lines[lineNumber] = text;
    }
}

void DisplayManager::clearLineArea(uint8_t lineNumber) {
#ifdef USE_DISPLAY
    uint8_t y = lineHeight * (lineNumber + 1);
    display.fillRect(0, y, display.width(), lineHeight, SSD1306_BLACK);
#endif
}

void DisplayManager::render() {
#ifdef USE_DISPLAY    
    if (!displayInitialized) return;
    display.clearDisplay();

    // Headline
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(headline);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(headline, 0, 0, &x1, &y1, &w, &h);
    display.drawLine(0, h , w, h , SSD1306_WHITE);

    // Textzeilen
    for (uint8_t i = 0; i < maxLines; i++) {
        if (lines[i].length() > 0) {
            display.setTextSize(1);
            display.setCursor(0, lineHeight * (i + 1));
            display.print(lines[i]);
        }
    }

    display.display();
#endif
}

void DisplayManager::setLinesAndRender(const String& line0, const String& line1, const String& line2, const String& line3) {
    setLine(0, line0);
    setLine(1, line1);
    setLine(2, line2);
    setLine(3, line3);
    render();
}

void DisplayManager::clearDisplay() {
    #ifdef USE_DISPLAY
    if (displayInitialized) {
        display.clearDisplay();
        display.display();
    }
    #endif
}