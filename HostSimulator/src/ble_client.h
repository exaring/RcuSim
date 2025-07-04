#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLE2902.h>
#include <map>
#include <functional>
#include "ble_host_config.h"
#include "device_types.h"
#include "hid_constants.h"

class BLEHostClient : public BLEClientCallbacks {
private:
    BLEClient* pClient;
    BLERemoteService* pHIDService;
    BLERemoteService* pDeviceInfoService;
    BLERemoteService* pBatteryService;
    
    std::map<uint8_t, BLERemoteCharacteristic*> inputReportChars;
    std::map<uint8_t, BLERemoteCharacteristic*> outputReportChars;
    std::map<uint8_t, BLERemoteCharacteristic*> featureReportChars;
    
    BLERemoteCharacteristic* pReportMapChar;
    BLERemoteCharacteristic* pHIDInfoChar;
    BLERemoteCharacteristic* pControlPointChar;
    
    String connectedDeviceAddress;
    ConnectionState connectionState;
    DeviceInfo deviceInfo;
    HIDInformation hidInfo;
    
    std::function<void(const ReportData&)> reportCallback;
    std::function<void(ConnectionState)> connectionCallback;
    
    bool discoverServices();
    bool discoverHIDService();
    bool discoverDeviceInfoService();
    bool discoverBatteryService();
    bool readDeviceInformation();
    bool readHIDInformation();
    bool setupReportNotifications();
    
    static void reportNotifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);
    void processReportData(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length);
    
public:
    BLEHostClient();
    ~BLEHostClient();
    
    bool initialize();
    bool connectToDevice(const String& address, uint32_t timeout = BLE_CONNECTION_TIMEOUT);
    bool disconnect();
    bool isConnected() const { return connectionState == ConnectionState::CONNECTED; }
    ConnectionState getConnectionState() const { return connectionState; }
    String getConnectedDeviceAddress() const { return connectedDeviceAddress; }
    
    DeviceInfo getDeviceInfo() const { return deviceInfo; }
    HIDInformation getHIDInformation() const { return hidInfo; }
    std::vector<ServiceInfo> getServices() const;
    
    bool subscribeToReports();
    bool unsubscribeFromReports();
    
    bool sendOutputReport(uint8_t reportId, const uint8_t* data, size_t length);
    bool sendFeatureReport(uint8_t reportId, const uint8_t* data, size_t length);
    bool getFeatureReport(uint8_t reportId, uint8_t* data, size_t& length);
    
    void setReportCallback(std::function<void(const ReportData&)> callback);
    void setConnectionCallback(std::function<void(ConnectionState)> callback);
    
    // BLEClientCallbacks implementation
    void onConnect(BLEClient* pClient) override;
    void onDisconnect(BLEClient* pClient) override;
    
    void printDeviceInfo() const;
    void printHIDInformation() const;
    void printServices() const;
};

#endif // BLE_CLIENT_H