#ifndef HID_CONSTANTS_H
#define HID_CONSTANTS_H

#include <Arduino.h>
#include <map>

// HID Report Types
#define HID_REPORT_TYPE_INPUT   1
#define HID_REPORT_TYPE_OUTPUT  2
#define HID_REPORT_TYPE_FEATURE 3

// HID Usage Pages
#define HID_USAGE_PAGE_GENERIC_DESKTOP  0x01
#define HID_USAGE_PAGE_KEYBOARD         0x07
#define HID_USAGE_PAGE_LED              0x08
#define HID_USAGE_PAGE_CONSUMER         0x0C

// HID Usage IDs for Generic Desktop
#define HID_USAGE_KEYBOARD              0x06
#define HID_USAGE_MOUSE                 0x02
#define HID_USAGE_POINTER               0x01

// HID Usage IDs for Consumer Control
#define HID_USAGE_CONSUMER_CONTROL      0x01

// Keyboard Modifier Keys
#define KEY_MOD_LCTRL   0x01
#define KEY_MOD_LSHIFT  0x02
#define KEY_MOD_LALT    0x04
#define KEY_MOD_LGUI    0x08
#define KEY_MOD_RCTRL   0x10
#define KEY_MOD_RSHIFT  0x20
#define KEY_MOD_RALT    0x40
#define KEY_MOD_RGUI    0x80

// Consumer Control Keys (from analyzed remote)
#define CONSUMER_MUTE               0x00E2
#define CONSUMER_VOL_UP            0x00E9
#define CONSUMER_VOL_DOWN          0x00EA
#define CONSUMER_PLAY_PAUSE        0x00CD
#define CONSUMER_SCAN_NEXT         0x00B5
#define CONSUMER_SCAN_PREVIOUS     0x00B6
#define CONSUMER_STOP              0x00B7
#define CONSUMER_FAST_FORWARD      0x00B3
#define CONSUMER_REWIND            0x00B4
#define CONSUMER_RECORD            0x00B2
#define CONSUMER_MENU              0x0040
#define CONSUMER_HOME              0x0223
#define CONSUMER_BACK              0x0224
#define CONSUMER_AC_SEARCH         0x0221
#define CONSUMER_CHANNEL_UP        0x009C
#define CONSUMER_CHANNEL_DOWN      0x009D
#define CONSUMER_POWER             0x0030

// HID Report Descriptor Item Tags
#define HID_TAG_USAGE_PAGE         0x05
#define HID_TAG_USAGE              0x09
#define HID_TAG_USAGE_MINIMUM      0x19
#define HID_TAG_USAGE_MAXIMUM      0x29
#define HID_TAG_COLLECTION         0xA1
#define HID_TAG_END_COLLECTION     0xC0
#define HID_TAG_REPORT_ID          0x85
#define HID_TAG_REPORT_SIZE        0x75
#define HID_TAG_REPORT_COUNT       0x95
#define HID_TAG_INPUT              0x81
#define HID_TAG_OUTPUT             0x91
#define HID_TAG_FEATURE            0xB1
#define HID_TAG_LOGICAL_MINIMUM    0x15
#define HID_TAG_LOGICAL_MAXIMUM    0x25

// Collection Types
#define HID_COLLECTION_APPLICATION 0x01
#define HID_COLLECTION_LOGICAL     0x02
#define HID_COLLECTION_PHYSICAL    0x00

// Input/Output/Feature Data Types
#define HID_DATA_CONSTANT          0x01
#define HID_DATA_VARIABLE          0x02
#define HID_DATA_RELATIVE          0x04
#define HID_DATA_WRAP              0x08
#define HID_DATA_NON_LINEAR        0x10
#define HID_DATA_NO_PREFERRED      0x20
#define HID_DATA_NULL_STATE        0x40
#define HID_DATA_VOLATILE          0x80

// Key code mappings for common keys
static const std::map<uint8_t, String> KEYBOARD_KEYS = {
    {0x04, "A"}, {0x05, "B"}, {0x06, "C"}, {0x07, "D"}, {0x08, "E"}, {0x09, "F"},
    {0x0A, "G"}, {0x0B, "H"}, {0x0C, "I"}, {0x0D, "J"}, {0x0E, "K"}, {0x0F, "L"},
    {0x10, "M"}, {0x11, "N"}, {0x12, "O"}, {0x13, "P"}, {0x14, "Q"}, {0x15, "R"},
    {0x16, "S"}, {0x17, "T"}, {0x18, "U"}, {0x19, "V"}, {0x1A, "W"}, {0x1B, "X"},
    {0x1C, "Y"}, {0x1D, "Z"},
    {0x1E, "1"}, {0x1F, "2"}, {0x20, "3"}, {0x21, "4"}, {0x22, "5"},
    {0x23, "6"}, {0x24, "7"}, {0x25, "8"}, {0x26, "9"}, {0x27, "0"},
    {0x28, "ENTER"}, {0x29, "ESC"}, {0x2A, "BACKSPACE"}, {0x2B, "TAB"},
    {0x2C, "SPACE"}, {0x2D, "-"}, {0x2E, "="}, {0x2F, "["}, {0x30, "]"},
    {0x39, "CAPS"}, {0x3A, "F1"}, {0x3B, "F2"}, {0x3C, "F3"}, {0x3D, "F4"},
    {0x3E, "F5"}, {0x3F, "F6"}, {0x40, "F7"}, {0x41, "F8"}, {0x42, "F9"},
    {0x43, "F10"}, {0x44, "F11"}, {0x45, "F12"},
    {0x4F, "RIGHT"}, {0x50, "LEFT"}, {0x51, "DOWN"}, {0x52, "UP"}
};

// Consumer control key mappings
static const std::map<uint16_t, String> CONSUMER_KEYS = {
    {CONSUMER_MUTE, "MUTE"},
    {CONSUMER_VOL_UP, "VOL_UP"},
    {CONSUMER_VOL_DOWN, "VOL_DOWN"},
    {CONSUMER_PLAY_PAUSE, "PLAY_PAUSE"},
    {CONSUMER_SCAN_NEXT, "NEXT"},
    {CONSUMER_SCAN_PREVIOUS, "PREVIOUS"},
    {CONSUMER_STOP, "STOP"},
    {CONSUMER_FAST_FORWARD, "FF"},
    {CONSUMER_REWIND, "REWIND"},
    {CONSUMER_RECORD, "RECORD"},
    {CONSUMER_MENU, "MENU"},
    {CONSUMER_HOME, "HOME"},
    {CONSUMER_BACK, "BACK"},
    {CONSUMER_AC_SEARCH, "SEARCH"},
    {CONSUMER_CHANNEL_UP, "CH_UP"},
    {CONSUMER_CHANNEL_DOWN, "CH_DOWN"},
    {CONSUMER_POWER, "POWER"}
};

// Modifier key mappings
static const std::map<uint8_t, String> MODIFIER_KEYS = {
    {KEY_MOD_LCTRL, "L_CTRL"},
    {KEY_MOD_LSHIFT, "L_SHIFT"},
    {KEY_MOD_LALT, "L_ALT"},
    {KEY_MOD_LGUI, "L_GUI"},
    {KEY_MOD_RCTRL, "R_CTRL"},
    {KEY_MOD_RSHIFT, "R_SHIFT"},
    {KEY_MOD_RALT, "R_ALT"},
    {KEY_MOD_RGUI, "R_GUI"}
};

#endif // HID_CONSTANTS_H