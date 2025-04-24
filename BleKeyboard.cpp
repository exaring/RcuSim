#ifndef BLE_KEYBOARD_CONFIG_H
#define BLE_KEYBOARD_CONFIG_H

#include <Arduino.h>
#include "BleKeyboard.h"

// ================ CONFIGURATION PARAMETERS ================
// Customize these values to change the device properties
#define DEVICE_NAME "ESP32 BLE Keyboard"  // Device name shown in Bluetooth settings
#define MANUFACTURER_NAME "Espressif"     // Manufacturer name
#define INITIAL_BATTERY_LEVEL 100         // Initial battery level (0-100)

// USB HID parameters
#define VENDOR_ID 0x05ac                 // Vendor ID (Apple in this example)
#define PRODUCT_ID 0x820a                // Product ID
#define VERSION_ID 0x0210                // Version number
// =========================================================

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

// Structure for key mapping
struct KeyMapping {
  const char* name;
  uint8_t keyCode;
};

// Structure for media key mapping
struct MediaKeyMapping {
  const char* name;
  uint16_t keyCode;  // Using uint16_t for media keys
};

// Mapping from string names to regular keycodes
const KeyMapping keyMappings[] = {
  {"up", KEY_UP_ARROW},
  {"down", KEY_DOWN_ARROW},
  {"left", KEY_LEFT_ARROW},
  {"right", KEY_RIGHT_ARROW},
  {"enter", KEY_RETURN},
  {"return", KEY_RETURN},
  {"esc", KEY_ESC},
  {"escape", KEY_ESC},
  {"backspace", KEY_BACKSPACE},
  {"tab", KEY_TAB},
  {"space", ' '},
  {"ctrl", KEY_LEFT_CTRL},
  {"alt", KEY_LEFT_ALT},
  {"shift", KEY_LEFT_SHIFT},
  {"win", KEY_LEFT_GUI},
  {"gui", KEY_LEFT_GUI},
  {"insert", KEY_INSERT},
  {"delete", KEY_DELETE},
  {"del", KEY_DELETE},
  {"home", KEY_HOME},
  {"end", KEY_END},
  {"pageup", KEY_PAGE_UP},
  {"pagedown", KEY_PAGE_DOWN},
  {"capslock", KEY_CAPS_LOCK},
  {"f1", KEY_F1},
  {"f2", KEY_F2},
  {"f3", KEY_F3},
  {"f4", KEY_F4},
  {"f5", KEY_F5},
  {"f6", KEY_F6},
  {"f7", KEY_F7},
  {"f8", KEY_F8},
  {"f9", KEY_F9},
  {"f10", KEY_F10},
  {"f11", KEY_F11},
  {"f12", KEY_F12},
  {"printscreen", KEY_PRINT_SCREEN},
  {"scrolllock", KEY_SCROLL_LOCK},
  {"pause", KEY_PAUSE}
};

// Mapping from string names to media keycodes
const MediaKeyMapping mediaKeyMappings[] = {
  {"playpause", KEY_MEDIA_PLAY_PAUSE},
  {"play", KEY_MEDIA_PLAY_PAUSE},
  {"pause", KEY_MEDIA_PLAY_PAUSE},
  {"nexttrack", KEY_MEDIA_NEXT_TRACK},
  {"next", KEY_MEDIA_NEXT_TRACK},
  {"prevtrack", KEY_MEDIA_PREVIOUS_TRACK},
  {"previous", KEY_MEDIA_PREVIOUS_TRACK},
  {"prev", KEY_MEDIA_PREVIOUS_TRACK},
  {"stop", KEY_MEDIA_STOP},
  {"mute", KEY_MEDIA_MUTE},
  {"volumeup", KEY_MEDIA_VOLUME_UP},
  {"volup", KEY_MEDIA_VOLUME_UP},
  {"volumedown", KEY_MEDIA_VOLUME_DOWN},
  {"voldown", KEY_MEDIA_VOLUME_DOWN},
  {"home", KEY_MEDIA_WWW_HOME},
  {"computer", KEY_MEDIA_LOCAL_MACHINE_BROWSER},
  {"mypc", KEY_MEDIA_LOCAL_MACHINE_BROWSER},
  {"calc", KEY_MEDIA_CALCULATOR},
  {"calculator", KEY_MEDIA_CALCULATOR},
  {"bookmarks", KEY_MEDIA_WWW_BOOKMARKS},
  {"favorites", KEY_MEDIA_WWW_BOOKMARKS},
  {"search", KEY_MEDIA_WWW_SEARCH},
  {"browserstop", KEY_MEDIA_WWW_STOP},
  {"back", KEY_MEDIA_WWW_BACK},
  {"mediaselect", KEY_MEDIA_MEDIA_SELECT},
  {"mail", KEY_MEDIA_MAIL},
  {"email", KEY_MEDIA_MAIL},
  {"power", 0x30}  // Consumer Control Power
};

const int NUM_KEY_MAPPINGS = sizeof(keyMappings) / sizeof(KeyMapping);
const int NUM_MEDIA_KEY_MAPPINGS = sizeof(mediaKeyMappings) / sizeof(MediaKeyMapping);

// Function declarations
void processCommand(String command);
void printError(int errorCode, String message);
void printStatus(int statusCode, String message);
void printHelp();
uint8_t getKeyCode(String key);
uint16_t getMediaKeyCode(String key);
bool isMediaKey(String key);
void startAdvertising();
void stopAdvertising();
void printDiagnostics();
void printConfiguration();

#endif // BLE_KEYBOARD_CONFIG_H