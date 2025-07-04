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

// Command handler structure
struct CommandHandler {
  const char* command;
  void (*handler)(String);
  bool needsParameter;
  const char* helpText;
  const char* category;
};

// Function prototypes - Core functions
void setupSerial();
void handleSerialCommands();
void setupBLE();
void processCommand(String command);
void updateBootCounter();
bool executeHexCommand(unsigned long param1, unsigned long param2, int delay, const String& commandName);

// Function prototypes - WiFi commands
void cmdSetSSID(String parameter);
void cmdSetPwd(String parameter);
void cmdSetIP(String parameter);
void cmdSetGateway(String parameter);
void cmdSaveConfig(String parameter);
void cmdConnectWiFi(String parameter);
void cmdShowConfig(String parameter);

// Function prototypes - BLE commands
void cmdStartPairing(String parameter);
void cmdStopPairing(String parameter);
void cmdUnpair(String parameter);
void cmdSendKey(String parameter);
void cmdPressKey(String parameter);
void cmdReleaseKey(String parameter);
void cmdReleaseAllKeys(String parameter);
void cmdSetBatteryLevel(String parameter);
void cmdShowBleStatus(String parameter);
void cmdSendSequence(String parameter);
void cmdSendHex(String parameter);
void cmdSendHex1(String parameter);
void cmdSendHex2(String parameter);

// Function prototypes - System commands
void cmdReboot(String parameter);
void cmdDiag(String parameter);
void printHelp(String parameter);

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