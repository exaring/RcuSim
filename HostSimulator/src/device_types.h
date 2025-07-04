#ifndef DEVICE_TYPES_H
#define DEVICE_TYPES_H

#include <Arduino.h>
#include <vector>
#include <map>

// BLE Service UUIDs
#define HID_SERVICE_UUID                "00001812-0000-1000-8000-00805F9B34FB"
#define DEVICE_INFORMATION_SERVICE_UUID "0000180A-0000-1000-8000-00805F9B34FB"
#define BATTERY_SERVICE_UUID            "0000180F-0000-1000-8000-00805F9B34FB"

// HID Characteristic UUIDs
#define HID_REPORT_CHAR_UUID            "00002A4D-0000-1000-8000-00805F9B34FB"
#define HID_REPORT_MAP_CHAR_UUID        "00002A4B-0000-1000-8000-00805F9B34FB"
#define HID_INFORMATION_CHAR_UUID       "00002A4A-0000-1000-8000-00805F9B34FB"
#define HID_CONTROL_POINT_CHAR_UUID     "00002A4C-0000-1000-8000-00805F9B34FB"

// Device Information Characteristic UUIDs
#define MANUFACTURER_NAME_CHAR_UUID     "00002A29-0000-1000-8000-00805F9B34FB"
#define MODEL_NUMBER_CHAR_UUID          "00002A24-0000-1000-8000-00805F9B34FB"
#define SERIAL_NUMBER_CHAR_UUID         "00002A25-0000-1000-8000-00805F9B34FB"
#define FIRMWARE_REVISION_CHAR_UUID     "00002A26-0000-1000-8000-00805F9B34FB"
#define HARDWARE_REVISION_CHAR_UUID     "00002A27-0000-1000-8000-00805F9B34FB"
#define SOFTWARE_REVISION_CHAR_UUID     "00002A28-0000-1000-8000-00805F9B34FB"
#define PNP_ID_CHAR_UUID                "00002A50-0000-1000-8000-00805F9B34FB"

// Battery Level Characteristic UUID
#define BATTERY_LEVEL_CHAR_UUID         "00002A19-0000-1000-8000-00805F9B34FB"

// Device Types
enum class DeviceType {
    UNKNOWN,
    KEYBOARD,
    MOUSE,
    REMOTE_CONTROL,
    GAME_CONTROLLER,
    MULTIMEDIA_REMOTE
};

// Connection State
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    ERROR
};

// Scanned Device Information
struct ScannedDevice {
    String address;
    String name;
    String manufacturer;
    int16_t rssi;
    DeviceType deviceType;
    bool hasHIDService;
    bool hasDeviceInfoService;
    bool hasBatteryService;
    std::vector<String> serviceUUIDs;
    uint32_t scanTimestamp;
    
    ScannedDevice() : 
        rssi(-100), 
        deviceType(DeviceType::UNKNOWN),
        hasHIDService(false),
        hasDeviceInfoService(false),
        hasBatteryService(false),
        scanTimestamp(0) {}
        
    bool isValid() const {
        return !address.isEmpty();
    }
};

// Device Information
struct DeviceInfo {
    String manufacturerName;
    String modelNumber;
    String serialNumber;
    String firmwareRevision;
    String hardwareRevision;
    String softwareRevision;
    uint16_t vendorId;
    uint16_t productId;
    uint16_t version;
    uint8_t vendorIdSource;
    bool hasHIDService;
    bool hasDeviceInfoService;
    bool hasBatteryService;
    uint8_t batteryLevel;
    
    DeviceInfo() : 
        vendorId(0), 
        productId(0), 
        version(0),
        vendorIdSource(0),
        hasHIDService(false),
        hasDeviceInfoService(false),
        hasBatteryService(false),
        batteryLevel(0) {}
};

// Service Information
struct ServiceInfo {
    String uuid;
    String name;
    std::vector<String> characteristicUUIDs;
    
    ServiceInfo(const String& serviceUuid, const String& serviceName) : 
        uuid(serviceUuid), name(serviceName) {}
};

// HID Report Information
struct HIDReportInfo {
    uint8_t reportId;
    uint8_t reportType; // 1=Input, 2=Output, 3=Feature
    uint16_t reportSize;
    String description;
    
    HIDReportInfo() : reportId(0), reportType(0), reportSize(0) {}
    HIDReportInfo(uint8_t id, uint8_t type, uint16_t size, const String& desc) :
        reportId(id), reportType(type), reportSize(size), description(desc) {}
};

// HID Information
struct HIDInformation {
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t flags;
    std::vector<uint8_t> reportDescriptor;
    std::map<uint8_t, HIDReportInfo> reportMap;
    
    HIDInformation() : bcdHID(0), bCountryCode(0), flags(0) {}
};

// Report Data
struct ReportData {
    uint8_t reportId;
    std::vector<uint8_t> data;
    uint32_t timestamp;
    String decodedData;
    
    ReportData() : reportId(0), timestamp(0) {}
    ReportData(uint8_t id, const uint8_t* reportData, size_t length) :
        reportId(id), timestamp(millis()) {
        data.assign(reportData, reportData + length);
    }
};

// Known Device Patterns (for device type detection)
struct DevicePattern {
    String namePattern;
    String manufacturerPattern;
    DeviceType deviceType;
    String description;
};

// Static list of known device patterns
static const std::vector<DevicePattern> KNOWN_DEVICE_PATTERNS = {
    {"Remote", ".*", DeviceType::REMOTE_CONTROL, "Generic Remote Control"},
    {"Keyboard", ".*", DeviceType::KEYBOARD, "Generic Keyboard"},
    {"Mouse", ".*", DeviceType::MOUSE, "Generic Mouse"},
    {"Gamepad", ".*", DeviceType::GAME_CONTROLLER, "Generic Game Controller"},
    {"Controller", ".*", DeviceType::GAME_CONTROLLER, "Generic Controller"},
    {"Examote", "Exaring", DeviceType::REMOTE_CONTROL, "Exaring Remote Control"},
    {"ESP32", ".*", DeviceType::REMOTE_CONTROL, "ESP32 Based Remote"}
};

#endif // DEVICE_TYPES_H