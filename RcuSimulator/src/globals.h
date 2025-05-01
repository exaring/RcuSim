#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include <WiFi.h>

// ================ CONFIGURATION PARAMETERS ================
// Customize these values to change the BLE device properties
#define BLE_DEVICE_NAME "Examote One"  // Device name shown in Bluetooth settings
#define BLE_MANUFACTURER_NAME "Exaring"     // Manufacturer name
#define BLE_INITIAL_BATTERY_LEVEL 100         // Initial battery level (0-100)

// USB HID parameters
#define VENDOR_ID 0x05ac                 // Vendor ID (Apple in this example)
#define PRODUCT_ID 0x820a                // Product ID
#define VERSION_ID 0x0210                // Version number

// SYSTEM PARAMETERS
#define DEVICE_NAME "ESP32 BLE Remote Control" // Device name for the web server
#define DEVICE_MANUFACTURER "Exaring"
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

#endif