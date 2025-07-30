#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "webserver.h"
#include "wifimanager.h"
#include "BleRemoteControl.h"

#define USE_DISPLAY // Define this to enable display functionality

// Error codes
#define ERR_PREFIX "ERROR:"
#define ERR_UNKNOWN_COMMAND 1001
#define ERR_INVALID_PARAMETER 1002
#define ERR_COMMAND_FAILED 1003
#define ERR_NOT_CONNECTED 1004
#define ERR_ALREADY_ADVERTISING 1005
#define ERR_NOT_ADVERTISING 1006
#define ERR_KEY_NOT_FOUND 1007

// Status
#define STATUS_PREFIX "STATUS:"
#define STATUS_OK 2000
#define STATUS_CONNECTED 2001
#define STATUS_DISCONNECTED 2002
#define STATUS_ADVERTISING 2003
#define STATUS_PAIRING 2004
#define STATUS_PAIRED 2005

// Global Variables
extern bool isConfigMode;
extern WiFiManager wifiManager;
extern BleRemoteControl bleRemoteControl;
extern Preferences preferences;

extern unsigned long startTime;
extern unsigned long bootCount;
  
#endif