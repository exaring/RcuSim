#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <vector>
#include <functional>
#include "ble_host_config.h"
#include "device_types.h"
#include "hid_constants.h"

class BLEScanner : public BLEAdvertisedDeviceCallbacks {
private:
    BLEScan* pBLEScan;
    std::vector<ScannedDevice> scannedDevices;
    ScanFilter currentFilter;
    bool isScanning;
    uint32_t scanDuration;
    std::function<void(const ScannedDevice&)> deviceFoundCallback;
    std::function<void()> scanCompleteCallback;

    DeviceType determineDeviceType(const String& name, const String& manufacturer, const std::vector<String>& serviceUUIDs);
    bool passesFilter(const ScannedDevice& device);
    bool deviceExists(const String& address);
    void updateDevice(const ScannedDevice& device);

public:
    BLEScanner();
    ~BLEScanner();
    
    bool initialize();
    bool startScan(uint32_t duration = BLE_SCAN_TIME_DEFAULT);
    void stopScan();
    bool isCurrentlyScanning() const { return isScanning; }
    
    void setFilter(const ScanFilter& filter);
    void clearFilter();
    ScanFilter getFilter() const { return currentFilter; }
    
    std::vector<ScannedDevice> getScannedDevices() const { return scannedDevices; }
    ScannedDevice getDevice(size_t index) const;
    ScannedDevice getDeviceByAddress(const String& address) const;
    size_t getDeviceCount() const { return scannedDevices.size(); }
    void clearDevices();
    
    void setDeviceFoundCallback(std::function<void(const ScannedDevice&)> callback);
    void setScanCompleteCallback(std::function<void()> callback);
    
    // BLEAdvertisedDeviceCallbacks implementation
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
    
    void printScanResults() const;
    void printDevice(const ScannedDevice& device) const;
};

#endif // BLE_SCANNER_H