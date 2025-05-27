#ifndef ESP32_BLE_REMOTE_CONTROL_H
#define ESP32_BLE_REMOTE_CONTROL_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <BLEHIDDevice.h>
#include <BLECharacteristic.h>

#include "Print.h"

#define BLE_REMOTE_CONTROL_VERSION "0.3.0"

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

//  Media Keys
#define KEY_MEDIA_PROG          0x00000001 // 00 00 01 - Consumer Control Program
#define KEY_MEDIA_POWER         0x00000002 // 00 00 02 - Consumer Control Power
#define KEY_MEDIA_TV            0x00000004 // 00 00 04 - Consumer Control TV
#define KEY_MEDIA_MENU          0x00000008 // 00 00 08 - Consumer Control Menu
#define KEY_MEDIA_OK            0x00000010 // 00 00 10 - Consumer Control OK
#define KEY_MEDIA_UP            0x00000020 // 00 00 20 - Consumer Control Up
#define KEY_MEDIA_DOWN          0x00000040 // 00 00 40 - Consumer Control Down
#define KEY_MEDIA_LEFT          0x00000080 // 00 00 80 - Consumer Control Left
#define KEY_MEDIA_RIGHT         0x00000100 // 00 01 00 - Consumer Control Right
#define KEY_MEDIA_CHANNEL_UP    0x00000042 // 00 02 00 - Consumer Control Channel Up
#define KEY_MEDIA_CHANNEL_DOWN  0x00000043 // 00 04 00 - Consumer Control Channel Down
#define KEY_MEDIA_REWIND        0x00000800 // 00 08 00 - Consumer Control Rewind
#define KEY_MEDIA_RECORD        0x00001000 // 00 10 00 - Consumer Control Record
#define KEY_MEDIA_FAST_FORWARD  0x00002000 // 00 20 00 - Consumer Control Fast Forward
#define KEY_MEDIA_NEXT          0x00004000 // 00 40 00 - Consumer Control Next
#define KEY_MEDIA_PREVIOUS      0x00008000 // 00 80 00 - Consumer Control Previous

//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

//  Media key report: 2 bytes to use for media keys
typedef uint8_t MediaKeyReport[2];

class BleRemoteControl : public Print, public BLEServerCallbacks, public BLECharacteristicCallbacks
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
  uint32_t _delay_ms = 7;
  uint16_t vid = 0x05ac;
  uint16_t pid = 0x820a;
  uint16_t version = 0x0210;
  BLEServer* pServer = nullptr;
  // Callback function for connection events
  typedef std::function<void(String)> ConnectionCallback;
  ConnectionCallback connectCallback = nullptr;

public:
  BleRemoteControl(std::string deviceName = "ESP32 BLE Remote Control", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);
  void end(void);
  void sendReport(KeyReport* keys);
  void sendReport(MediaKeyReport* keys);
  size_t press(uint8_t k);
  size_t press(const MediaKeyReport k);
  size_t release(uint8_t k);
  size_t release(const MediaKeyReport k);
  size_t write(uint8_t c);
  size_t write(const MediaKeyReport c);
  size_t write(const uint8_t *buffer, size_t size);
  void releaseAll(void);
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);
  void setDelay(uint32_t ms);
  void set_vendor_id(uint16_t vid);
  void set_product_id(uint16_t pid);
  void set_version(uint16_t version);
  void delay_ms(uint64_t ms);
  bool disconnect();      // Method to actively disconnect the connection
  bool removeBonding();   // Method to remove all pairings and bondings
  void startAdvertising(); // Method to specifically start advertising
  void stopAdvertising();  // Method to specifically stop advertising
  // Set callback function for connection events
  void setConnectionCallback(ConnectionCallback cb) { connectCallback = cb; }

protected:
  virtual void onStarted(BLEServer *pServer) { };
  virtual void onConnect(BLEServer* pServer);
  virtual void onDisconnect(BLEServer* pServer);
  virtual void onWrite(BLECharacteristic* me);
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_REMOTE_CONTROL_H