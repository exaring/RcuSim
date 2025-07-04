#ifndef BLE_HOST_CONFIG_H
#define BLE_HOST_CONFIG_H

#include <Arduino.h>

// BLE Configuration
#define BLE_HOST_DEVICE_NAME "ESP32 BLE Host"
#define BLE_SCAN_TIME_DEFAULT 10
#define BLE_CONNECTION_TIMEOUT 30000
#define BLE_MAX_DEVICES 20

// CLI Configuration
#define CLI_PROMPT "ble-host> "
#define CLI_MAX_COMMAND_LENGTH 256
#define CLI_MAX_ARGS 10

// Logging Configuration
#define LOG_BUFFER_SIZE 1024
#define LOG_FILE_MAX_SIZE (1024 * 1024) // 1MB
#define LOG_TIMESTAMP_FORMAT "%02d:%02d:%02d.%03d"

// Report Monitoring Configuration
#define REPORT_MONITOR_BUFFER_SIZE 256
#define MAX_REPORT_SIZE 64

// Output Formats
enum class OutputFormat {
    HEX_ONLY,
    DECODED_ONLY,
    BOTH
};

// Scan Filter Options
struct ScanFilter {
    String nameFilter;
    String manufacturerFilter;
    int16_t minRSSI = -100;
    bool filterByName = false;
    bool filterByManufacturer = false;
    bool filterByRSSI = false;
};

// Error Codes
#define ERR_SUCCESS 0
#define ERR_INVALID_COMMAND 1001
#define ERR_INVALID_PARAMETER 1002
#define ERR_DEVICE_NOT_FOUND 1003
#define ERR_CONNECTION_FAILED 1004
#define ERR_NOT_CONNECTED 1005
#define ERR_SCAN_FAILED 1006
#define ERR_PARSE_FAILED 1007

// Status Messages
#define STATUS_SCANNING "Scanning for BLE devices..."
#define STATUS_SCAN_COMPLETE "Scan completed"
#define STATUS_CONNECTING "Connecting to device..."
#define STATUS_CONNECTED "Successfully connected"
#define STATUS_DISCONNECTED "Device disconnected"
#define STATUS_MONITORING "Report monitoring started"
#define STATUS_MONITORING_STOPPED "Report monitoring stopped"

// Debug Configuration
#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define BLE_HOST_LOG_TAG "BLE_HOST"
#else
  #include "esp_log.h"
  static const char* BLE_HOST_LOG_TAG = "BLE_HOST";
#endif

#endif // BLE_HOST_CONFIG_H