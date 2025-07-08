#ifndef ESP32_BLE_REMOTE_CONTROL_H
#define ESP32_BLE_REMOTE_CONTROL_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <Arduino.h>
#include <functional>
#include "Print.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include <BLECharacteristic.h>
#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

// USB HID parameters
#define VENDOR_ID 0x012d                 // Vendor ID 
#define PRODUCT_ID 0x2ec0                // Product ID
#define VERSION_ID 0x1101                // Version number

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02

// HID Report Descriptor matching the analyzed remote control (a4:c1:38:81:21:05)
static const uint8_t _hidReportDescriptor[] = {
  // Keyboard Report (Report ID 1)
  USAGE_PAGE(1),      0x01,        // Usage Page (Generic Desktop Ctrls)
  USAGE(1),           0x06,        // Usage (Keyboard)
  COLLECTION(1),      0x01,        // Collection (Application)
  REPORT_ID(1),       0x01,        //   Report ID (1)
  USAGE_PAGE(1),      0x07,        //   Usage Page (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,        //   Usage Minimum (0xE0) - Left Control
  USAGE_MAXIMUM(1),   0xE7,        //   Usage Maximum (0xE7) - Right GUI
  LOGICAL_MINIMUM(1), 0x00,        //   Logical Minimum (0)
  LOGICAL_MAXIMUM(1), 0x01,        //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,        //   Report Size (1 bit)
  REPORT_COUNT(1),    0x08,        //   Report Count (8) - 8 modifier keys
  HIDINPUT(1),        0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x01,        //   Report Count (1) - 1 reserved byte
  REPORT_SIZE(1),     0x08,        //   Report Size (8 bits)
  HIDINPUT(1),        0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x05,        //   Report Count (5) - 5 LED bits
  REPORT_SIZE(1),     0x01,        //   Report Size (1 bit)
  USAGE_PAGE(1),      0x08,        //   Usage Page (LEDs)
  USAGE_MINIMUM(1),   0x01,        //   Usage Minimum (Num Lock)
  USAGE_MAXIMUM(1),   0x05,        //   Usage Maximum (Kana)
  HIDOUTPUT(1),       0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,        //   Report Count (1) - 1 padding byte
  REPORT_SIZE(1),     0x03,        //   Report Size (3 bits)
  HIDOUTPUT(1),       0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x06,        //   Report Count (6) - 6 key slots
  REPORT_SIZE(1),     0x08,        //   Report Size (8 bits)
  LOGICAL_MINIMUM(1), 0x00,        //   Logical Minimum (0)
  LOGICAL_MAXIMUM(1), 0xFF,        //   Logical Maximum (255)
  USAGE_PAGE(1),      0x07,        //   Usage Page (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,        //   Usage Minimum (0x00)
  USAGE_MAXIMUM(1),   0xFF,        //   Usage Maximum (0xFF)
  HIDINPUT(1),        0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),               // End Collection
  
  // Consumer Control Report (Report ID 2)
  USAGE_PAGE(1),      0x0C,        // Usage Page (Consumer)
  USAGE(1),           0x01,        // Usage (Consumer Control)
  COLLECTION(1),      0x01,        // Collection (Application)
  REPORT_ID(1),       0x02,        //   Report ID (2)
  REPORT_SIZE(1),     0x10,        //   Report Size (16 bits)
  REPORT_COUNT(1),    0x02,        //   Report Count (2) - 2x 16-bit consumer codes
  LOGICAL_MINIMUM(1), 0x01,        //   Logical Minimum (1)
  LOGICAL_MAXIMUM(2), 0xFF, 0x03,  //   Logical Maximum (1023)
  USAGE_MINIMUM(1),   0x01,        //   Usage Minimum (0x01)
  USAGE_MAXIMUM(2),   0xFF, 0x03,  //   Usage Maximum (0x3FF)
  HIDINPUT(1),        0x60,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,Null State)
  REPORT_COUNT(1),    0x01,        //   Report Count (1) - 1 padding byte
  REPORT_SIZE(1),     0x08,        //   Report Size (8 bits)
  LOGICAL_MINIMUM(1), 0x00,        //   Logical Minimum (0)
  LOGICAL_MAXIMUM(1), 0xFF,        //   Logical Maximum (255)
  HIDINPUT(1),        0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),               // End Collection
};

/**
 * @brief Keyboard report map.
 * 
 * Keyboard report descriptor (using format defined in USB HID specs)
 * https://www.usb.org/sites/default/files/documents/hid1_11.pdf
 */

//  Keyboard
#define KEY_LEFT_CTRL     0x80
#define KEY_LEFT_SHIFT    0x81
#define KEY_LEFT_ALT      0x82
#define KEY_LEFT_GUI      0x83
#define KEY_RIGHT_CTRL    0x84
#define KEY_RIGHT_SHIFT   0x85
#define KEY_RIGHT_ALT     0x86
#define KEY_RIGHT_GUI     0x87

#define KEY_UP_ARROW      0xDA
#define KEY_DOWN_ARROW    0xD9
#define KEY_LEFT_ARROW    0xD8
#define KEY_RIGHT_ARROW   0xD7
#define KEY_BACKSPACE     0xB2
#define KEY_TAB           0xB3
#define KEY_RETURN        0xB0
#define KEY_ESC           0xB1
#define KEY_INSERT        0xD1
#define KEY_DELETE        0xD4
#define KEY_PAGE_UP       0xD3
#define KEY_PAGE_DOWN     0xD6
#define KEY_HOME          0xD2
#define KEY_END           0xD5
#define KEY_CAPS_LOCK     0xC1
#define KEY_F1            0xC2
#define KEY_F2            0xC3
#define KEY_F3            0xC4
#define KEY_F4            0xC5
#define KEY_F5            0xC6
#define KEY_F6            0xC7
#define KEY_F7            0xC8
#define KEY_F8            0xC9
#define KEY_F9            0xCA
#define KEY_F10           0xCB
#define KEY_F11           0xCC
#define KEY_F12           0xCD
#define KEY_F13           0xF0
#define KEY_F14           0xF1
#define KEY_F15           0xF2
#define KEY_F16           0xF3
#define KEY_F17           0xF4
#define KEY_F18           0xF5
#define KEY_F19           0xF6
#define KEY_F20           0xF7
#define KEY_F21           0xF8
#define KEY_F22           0xF9
#define KEY_F23           0xFA
#define KEY_F24           0xFB
#define KEY_PRINT_SCREEN  0xCE
#define KEY_SCROLL_LOCK   0xCF
#define KEY_PAUSE         0xD0

 
// Consumer Control Keys (updated to match analyzed descriptor)
// Values must be within 1-1023 range as per descriptor
#define KEY_MEDIA_PROGRAM           0x0007
#define KEY_MEDIA_PREVIOUS_CHANNEL  0x0201
#define KEY_MEDIA_MUTE              0x00E2
#define KEY_MEDIA_VOL_UP            0x00E9
#define KEY_MEDIA_VOL_DOWN          0x00EA
#define KEY_MEDIA_PLAY_PAUSE        0x00CD
#define KEY_MEDIA_NEXT              0x00B5
#define KEY_MEDIA_PREVIOUS          0x00B6
#define KEY_MEDIA_STOP              0x00B7
#define KEY_MEDIA_FAST_FORWARD      0x00B3
#define KEY_MEDIA_REWIND            0x00B4
#define KEY_MEDIA_RECORD            0x00B2
#define KEY_MEDIA_MENU              0x0040
#define KEY_MEDIA_HOME              0x0223
#define KEY_MEDIA_BACK              0x0224
#define KEY_MEDIA_OK                0x0041
#define KEY_MEDIA_UP                0x0042
#define KEY_MEDIA_DOWN              0x0043
#define KEY_MEDIA_LEFT              0x0044
#define KEY_MEDIA_RIGHT             0x0045
#define KEY_MEDIA_CHANNEL_UP        0x009C
#define KEY_MEDIA_CHANNEL_DOWN      0x009D
#define KEY_MEDIA_POWER             0x0030
#define KEY_MEDIA_TV                0x001C
#define KEY_MEDIA_ASSISTANT         0x2102
#define KEY_MEDIA_APP_NETFLIX       0x000A
#define KEY_MEDIA_APP_WAIPUTHEK     0x00D2

//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

//  Media key report: Updated to match analyzed descriptor (5 bytes total)
typedef struct
{
  uint16_t consumer1;  // First 16-bit consumer code
  uint16_t consumer2;  // Second 16-bit consumer code  
  uint8_t padding;     // Padding byte (constant)
} MediaKeyReport;

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

// Mapping from string names to media keycodes (updated for analyzed descriptor)
const MediaKeyMapping mediaKeyMappings[] = {
  {"program", KEY_MEDIA_PROGRAM},
  {"chprev", KEY_MEDIA_PREVIOUS_CHANNEL},
  {"power", KEY_MEDIA_POWER},
  {"tv", KEY_MEDIA_TV},
  {"menu", KEY_MEDIA_MENU},
  {"ok", KEY_MEDIA_OK},
  {"mkup", KEY_MEDIA_UP},
  {"mkdown", KEY_MEDIA_DOWN},
  {"mkleft", KEY_MEDIA_LEFT},
  {"mkright", KEY_MEDIA_RIGHT},
  {"channelup", KEY_MEDIA_CHANNEL_UP},
  {"chup", KEY_MEDIA_CHANNEL_UP},
  {"chdown", KEY_MEDIA_CHANNEL_DOWN},
  {"rewind", KEY_MEDIA_REWIND},
  {"record", KEY_MEDIA_RECORD},
  {"ff", KEY_MEDIA_FAST_FORWARD},
  {"next", KEY_MEDIA_NEXT},
  {"previous", KEY_MEDIA_PREVIOUS},
  {"playpause", KEY_MEDIA_PLAY_PAUSE},
  {"stop", KEY_MEDIA_STOP},
  {"assistant", KEY_MEDIA_ASSISTANT},
  {"back", KEY_MEDIA_BACK},
  {"home", KEY_MEDIA_HOME},
  {"volup", KEY_MEDIA_VOL_UP},
  {"voldown", KEY_MEDIA_VOL_DOWN},
  {"mute", KEY_MEDIA_MUTE},
  {"netflix", KEY_MEDIA_APP_NETFLIX},
  {"waiputhek", KEY_MEDIA_APP_WAIPUTHEK}
};

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US

	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    // {
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
} PROGMEM;

const int NUM_KEY_MAPPINGS = sizeof(keyMappings) / sizeof(KeyMapping);
const int NUM_MEDIA_KEY_MAPPINGS = sizeof(mediaKeyMappings) / sizeof(MediaKeyMapping);


class BleRemoteControl : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
  BLEHIDDevice* hid;
  BLECharacteristic* inputKeyboard;
  BLECharacteristic* outputKeyboard;
  BLECharacteristic* inputMediaKeys;
  BLEAdvertising*    advertising;
  KeyReport      _keyReport;
  MediaKeyReport _mediaKeyReport;
  std::string deviceName;
  std::string deviceManufacturer;
  uint8_t     batteryLevel;
  bool connected = false;
  bool isAdvertisingMode = false;
  uint32_t _delay_ms = 7;
  uint16_t vid = 0x05ac;
  uint16_t pid = 0x820a;
  uint16_t version = 0x0210;
  BLEServer* pServer = nullptr;
  // Callback function for connection events
  typedef std::function<void(String)> ConnectionCallback;
  ConnectionCallback connectCallback = nullptr;

  bool isMediaKey(String key);
  uint16_t getMediaKeyCode(String key);
  uint8_t getKeyCode(String key);
  size_t press(uint8_t k);
  size_t press(const MediaKeyReport k);
  size_t release(uint8_t k);
  size_t release(const MediaKeyReport k);
  void sendKeyReport(KeyReport* keys);
  void sendMediaReport(MediaKeyReport* keys);
  void sendMediaReport(uint16_t key);
  void sendMediaReport(uint16_t key1, uint16_t key2);
  size_t press(String key);
  size_t release(String key);
  void delay_ms(uint64_t ms);

public:
  BleRemoteControl(std::string deviceName = "ESP32 BLE Remote Control", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);

  bool startAdvertising(); // Method to specifically start advertising
  void stopAdvertising();  // Method to specifically stop advertising
  bool isAdvertising(void) { return this->isAdvertisingMode; } // Method to check if advertising
  bool removeBonding();   // Method to remove all pairings and bondings

  bool disconnect();      // Method to actively disconnect the connection
  bool isConnected(void) { return this->connected; } // Method to check if connected

  bool sendKey(String k, uint32_t delay_ms = 0);
  bool sendMediaKeyHex(String k, uint8_t position, uint32_t delay_ms);
  bool sendMediaKey(uint16_t first, uint16_t second, uint32_t delay_ms);
  bool sendPress(String key);
  bool sendRelease(String key);
  void releaseAll(void);

  void setBatteryLevel(uint8_t level);
  uint8_t getBatteryLevel(void) { return this->batteryLevel; }

  void setDefaultDelay(uint32_t ms);

protected:
  virtual void onStarted(BLEServer *pServer) { };
  virtual void onConnect(BLEServer* pServer);
  virtual void onDisconnect(BLEServer* pServer);
  virtual void onWrite(BLECharacteristic* me);
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_REMOTE_CONTROL_H