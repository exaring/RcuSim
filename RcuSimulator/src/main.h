#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "globals.h"
#include "webserver.h"
#include "wifimanager.h"
#include "displaymanager.h"
#include "BleRemoteControl.h"
#include "utils.h"
#include "generic_cli.h"
#include "cli_standard_commands.h"

// Command definition structure
struct CLICommandDef {
  const char* name;
  const char* description;
  const char* usage;
  void (*handler)(const CLIArgs& args);
  const char* category;
};
// Status update interval for display refresh
extern unsigned long lastStatusUpdate;
extern const unsigned long STATUS_UPDATE_INTERVAL;

// Function prototypes - Core functions
void setupSerial();
void setupBLE();
void setupCLI();
void updateBootCounter();
void tryConnectWifi();

#endif // MAIN_H