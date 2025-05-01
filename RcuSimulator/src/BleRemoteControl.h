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


#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

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
#define KEY_MEDIA_NEXT_TRACK    0x01
#define KEY_MEDIA_PREVIOUS_TRACK 0x02
#define KEY_MEDIA_STOP          0x04
#define KEY_MEDIA_PLAY_PAUSE    0x08
#define KEY_MEDIA_MUTE          0x10
#define KEY_MEDIA_VOLUME_UP     0x20
#define KEY_MEDIA_VOLUME_DOWN   0x40
#define KEY_MEDIA_WWW_HOME      0x80
#define KEY_MEDIA_LOCAL_MACHINE_BROWSER  0x100 // Opens "My Computer" on Windows
#define KEY_MEDIA_CALCULATOR    0x200
#define KEY_MEDIA_WWW_BOOKMARKS 0x400
#define KEY_MEDIA_WWW_SEARCH    0x800
#define KEY_MEDIA_WWW_STOP      0x1000
#define KEY_MEDIA_WWW_BACK      0x2000
#define KEY_MEDIA_MEDIA_SELECT  0x4000
#define KEY_MEDIA_MAIL          0x8000

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