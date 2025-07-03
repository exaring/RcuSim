#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "globals.h"
#include "webserver.h"
#include "wifimanager.h"
#include "BleRemoteControl.h"

// Optional OLED Display Support
// Uncomment the next line to enable OLED display support
// #define USE_DISPLAY

#ifdef USE_DISPLAY
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address - typical for 128x64 OLED

// Initialize the OLED display
extern Adafruit_SSD1306 display;

// Function to update display with current status
void updateDisplay();
#endif

// Status update interval for display refresh
extern unsigned long lastStatusUpdate;
extern const unsigned long STATUS_UPDATE_INTERVAL;

// Function prototypes
void setupSerial();
void handleSerialCommands();
void setupBLE();
void processCommand(String command);
void cmdSetSSID(String ssid);
void cmdSetPwd(String password);
void cmdSetIP(String ip);
void cmdSetGateway(String gateway);
void cmdSaveConfig();
void cmdConnectWiFi();
void cmdReboot();
void cmdShowConfig();
void cmdDiag();
void printHelp();
void updateBootCounter();
void cmdStartPairing();
void cmdStopPairing();
void cmdUnpair();
void cmdSendKey(String key);
void cmdPressKey(String key);
void cmdReleaseKey(String key);
void cmdSendSequence(String key);
void cmdSendHex(String parameter);
void cmdSendHex1(String parameter);
void cmdSendHex2(String parameter);
void cmdReleaseAllKeys();
void cmdSetBatteryLevel(String key);
void cmdShowBleStatus();

#ifdef USE_DISPLAY
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address - typical for 128x64 OLED

// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void updateDisplay();

#endif // USE_DISPLAY

#endif // MAIN_H