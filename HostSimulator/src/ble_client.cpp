#include "ble_client.h"
#include "hid_constants.h"
#include "ble_host_config.h"

// Static instance for callback access
static BLEHostClient* clientInstance = nullptr;

BLEHostClient::BLEHostClient() : 
    pClient(nullptr),
    pHIDService(nullptr),
    pDeviceInfoService(nullptr),
    pBatteryService(nullptr),
    pReportMapChar(nullptr),
    pHIDInfoChar(nullptr),
    pControlPointChar(nullptr),
    connectionState(ConnectionState::DISCONNECTED) {
    clientInstance = this;
}

BLEHostClient::~BLEHostClient() {
    disconnect();
    clientInstance = nullptr;
}

bool BLEHostClient::initialize() {
    try {
        pClient = BLEDevice::createClient();
        if (!pClient) {
            ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to create BLE client");
            return false;
        }
        
        pClient->setClientCallbacks(this);
        ESP_LOGI(BLE_HOST_LOG_TAG, "BLE Client initialized successfully");
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception during BLE client initialization: %s", e.what());
        return false;
    }
}

bool BLEHostClient::connectToDevice(const String& address, uint32_t timeout) {
    if (connectionState == ConnectionState::CONNECTED || 
        connectionState == ConnectionState::CONNECTING) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "Already connected or connecting");
        return false;
    }
    
    if (!pClient) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "BLE Client not initialized");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Connecting to device: %s", address.c_str());
    Serial.printf("Connecting to device: %s\n", address.c_str());
    
    connectionState = ConnectionState::CONNECTING;
    if (connectionCallback) {
        connectionCallback(connectionState);
    }
    
    try {
        BLEAddress bleAddress(address.c_str());
        
        if (!pClient->connect(bleAddress)) {
            ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to connect to device");
            connectionState = ConnectionState::ERROR;
            if (connectionCallback) {
                connectionCallback(connectionState);
            }
            return false;
        }
        
        connectedDeviceAddress = address;
        ESP_LOGI(BLE_HOST_LOG_TAG, "Connected to device successfully");
        
        // Discover services and characteristics
        if (!discoverServices()) {
            ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to discover services");
            disconnect();
            return false;
        }
        
        connectionState = ConnectionState::CONNECTED;
        if (connectionCallback) {
            connectionCallback(connectionState);
        }
        
        Serial.println("Successfully connected and services discovered");
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception during connection: %s", e.what());
        connectionState = ConnectionState::ERROR;
        if (connectionCallback) {
            connectionCallback(connectionState);
        }
        return false;
    }
}

bool BLEHostClient::disconnect() {
    if (connectionState == ConnectionState::DISCONNECTED) {
        return true;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Disconnecting from device");
    connectionState = ConnectionState::DISCONNECTING;
    
    if (connectionCallback) {
        connectionCallback(connectionState);
    }
    
    try {
        // Unsubscribe from notifications
        unsubscribeFromReports();
        
        // Clear service pointers
        pHIDService = nullptr;
        pDeviceInfoService = nullptr;
        pBatteryService = nullptr;
        pReportMapChar = nullptr;
        pHIDInfoChar = nullptr;
        pControlPointChar = nullptr;
        
        // Clear report characteristics
        inputReportChars.clear();
        outputReportChars.clear();
        featureReportChars.clear();
        
        // Disconnect client
        if (pClient) {
            pClient->disconnect();
        }
        
        connectionState = ConnectionState::DISCONNECTED;
        connectedDeviceAddress = "";
        
        if (connectionCallback) {
            connectionCallback(connectionState);
        }
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "Disconnected successfully");
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception during disconnection: %s", e.what());
        connectionState = ConnectionState::ERROR;
        if (connectionCallback) {
            connectionCallback(connectionState);
        }
        return false;
    }
}

bool BLEHostClient::discoverServices() {
    if (!pClient || !pClient->isConnected()) {
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Discovering services...");
    
    // Get all services
    std::map<std::string, BLERemoteService*>* serviceMap = pClient->getServices();
    if (!serviceMap) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to get services");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Found %d services", serviceMap->size());
    
    // Discover specific services
    bool hidServiceFound = discoverHIDService();
    bool deviceInfoServiceFound = discoverDeviceInfoService();
    bool batteryServiceFound = discoverBatteryService();
    
    if (!hidServiceFound) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "HID service not found");
    }
    
    // Read device information
    readDeviceInformation();
    readHIDInformation();
    
    return true; // Continue even if some services are missing
}

bool BLEHostClient::discoverHIDService() {
    pHIDService = pClient->getService(BLEUUID(HID_SERVICE_UUID));
    if (!pHIDService) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "HID service not found");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "HID service found");
    deviceInfo.hasHIDService = true;
    
    // Get HID characteristics
    std::map<std::string, BLERemoteCharacteristic*>* charMap = pHIDService->getCharacteristics();
    if (!charMap) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to get HID characteristics");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Found %d HID characteristics", charMap->size());
    
    // Find specific characteristics
    pReportMapChar = pHIDService->getCharacteristic(BLEUUID(HID_REPORT_MAP_CHAR_UUID));
    pHIDInfoChar = pHIDService->getCharacteristic(BLEUUID(HID_INFORMATION_CHAR_UUID));
    pControlPointChar = pHIDService->getCharacteristic(BLEUUID(HID_CONTROL_POINT_CHAR_UUID));
    
    // Find report characteristics
    for (auto& pair : *charMap) {
        BLERemoteCharacteristic* pChar = pair.second;
        if (pChar->getUUID().equals(BLEUUID(HID_REPORT_CHAR_UUID))) {
            // Get report reference descriptor to determine report ID and type
            BLERemoteDescriptor* pReportRefDesc = pChar->getDescriptor(BLEUUID((uint16_t)0x2908));
            if (pReportRefDesc) {
                try {
                    std::string refData = pReportRefDesc->readValue();
                    if (refData.length() >= 2) {
                        uint8_t reportId = refData[0];
                        uint8_t reportType = refData[1];
                        
                        switch (reportType) {
                            case HID_REPORT_TYPE_INPUT:
                                inputReportChars[reportId] = pChar;
                                ESP_LOGD(BLE_HOST_LOG_TAG, "Input report %d found", reportId);
                                break;
                            case HID_REPORT_TYPE_OUTPUT:
                                outputReportChars[reportId] = pChar;
                                ESP_LOGD(BLE_HOST_LOG_TAG, "Output report %d found", reportId);
                                break;
                            case HID_REPORT_TYPE_FEATURE:
                                featureReportChars[reportId] = pChar;
                                ESP_LOGD(BLE_HOST_LOG_TAG, "Feature report %d found", reportId);
                                break;
                        }
                    }
                } catch (const std::exception& e) {
                    ESP_LOGW(BLE_HOST_LOG_TAG, "Failed to read report reference for characteristic");
                    // If no report reference, assume it's an input report with ID 0
                    inputReportChars[0] = pChar;
                }
            } else {
                ESP_LOGW(BLE_HOST_LOG_TAG, "No report reference descriptor found, assuming input report");
                // If no report reference descriptor, assume it's an input report
                inputReportChars[0] = pChar;
            }
        }
    }
    
    return true;
}

bool BLEHostClient::discoverDeviceInfoService() {
    pDeviceInfoService = pClient->getService(BLEUUID(DEVICE_INFORMATION_SERVICE_UUID));
    if (!pDeviceInfoService) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "Device Information service not found");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Device Information service found");
    deviceInfo.hasDeviceInfoService = true;
    return true;
}

bool BLEHostClient::discoverBatteryService() {
    pBatteryService = pClient->getService(BLEUUID(BATTERY_SERVICE_UUID));
    if (!pBatteryService) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "Battery service not found");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Battery service found");
    deviceInfo.hasBatteryService = true;
    return true;
}

bool BLEHostClient::readDeviceInformation() {
    if (!pDeviceInfoService) {
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Reading device information...");
    
    try {
        // Read manufacturer name
        BLERemoteCharacteristic* pChar = pDeviceInfoService->getCharacteristic(BLEUUID(MANUFACTURER_NAME_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            deviceInfo.manufacturerName = String(value.c_str());
        }
        
        // Read model number
        pChar = pDeviceInfoService->getCharacteristic(BLEUUID(MODEL_NUMBER_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            deviceInfo.modelNumber = String(value.c_str());
        }
        
        // Read serial number
        pChar = pDeviceInfoService->getCharacteristic(BLEUUID(SERIAL_NUMBER_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            deviceInfo.serialNumber = String(value.c_str());
        }
        
        // Read firmware revision
        pChar = pDeviceInfoService->getCharacteristic(BLEUUID(FIRMWARE_REVISION_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            deviceInfo.firmwareRevision = String(value.c_str());
        }
        
        // Read hardware revision
        pChar = pDeviceInfoService->getCharacteristic(BLEUUID(HARDWARE_REVISION_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            deviceInfo.hardwareRevision = String(value.c_str());
        }
        
        // Read software revision
        pChar = pDeviceInfoService->getCharacteristic(BLEUUID(SOFTWARE_REVISION_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            deviceInfo.softwareRevision = String(value.c_str());
        }
        
        // Read PnP ID
        pChar = pDeviceInfoService->getCharacteristic(BLEUUID(PNP_ID_CHAR_UUID));
        if (pChar && pChar->canRead()) {
            std::string value = pChar->readValue();
            if (value.length() >= 7) {
                deviceInfo.vendorIdSource = value[0];
                deviceInfo.vendorId = (value[2] << 8) | value[1];
                deviceInfo.productId = (value[4] << 8) | value[3];
                deviceInfo.version = (value[6] << 8) | value[5];
            }
        }
        
        // Read battery level if available
        if (pBatteryService) {
            pChar = pBatteryService->getCharacteristic(BLEUUID(BATTERY_LEVEL_CHAR_UUID));
            if (pChar && pChar->canRead()) {
                std::string value = pChar->readValue();
                if (!value.empty()) {
                    deviceInfo.batteryLevel = (uint8_t)value[0];
                }
            }
        }
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "Device information read successfully");
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception reading device information: %s", e.what());
        return false;
    }
}

bool BLEHostClient::readHIDInformation() {
    if (!pHIDInfoChar) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "HID Information characteristic not found");
        return false;
    }
    
    try {
        // Read HID Information
        if (pHIDInfoChar->canRead()) {
            std::string value = pHIDInfoChar->readValue();
            if (value.length() >= 4) {
                hidInfo.bcdHID = (value[1] << 8) | value[0];
                hidInfo.bCountryCode = value[2];
                hidInfo.flags = value[3];
            }
        }
        
        // Read Report Map
        if (pReportMapChar && pReportMapChar->canRead()) {
            std::string value = pReportMapChar->readValue();
            hidInfo.reportDescriptor.assign(value.begin(), value.end());
            ESP_LOGI(BLE_HOST_LOG_TAG, "Report descriptor size: %d bytes", hidInfo.reportDescriptor.size());
        }
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "HID information read successfully");
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception reading HID information: %s", e.what());
        return false;
    }
}

bool BLEHostClient::subscribeToReports() {
    if (inputReportChars.empty()) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "No input report characteristics found");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Subscribing to input reports...");
    
    try {
        for (auto& pair : inputReportChars) {
            BLERemoteCharacteristic* pChar = pair.second;
            if (pChar->canNotify()) {
                pChar->registerForNotify(reportNotifyCallback);
                ESP_LOGD(BLE_HOST_LOG_TAG, "Subscribed to input report %d", pair.first);
            }
        }
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "Successfully subscribed to %d input reports", inputReportChars.size());
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception subscribing to reports: %s", e.what());
        return false;
    }
}

bool BLEHostClient::unsubscribeFromReports() {
    try {
        for (auto& pair : inputReportChars) {
            BLERemoteCharacteristic* pChar = pair.second;
            if (pChar->canNotify()) {
                pChar->registerForNotify(nullptr);
            }
        }
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "Unsubscribed from input reports");
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception unsubscribing from reports: %s", e.what());
        return false;
    }
}

bool BLEHostClient::sendOutputReport(uint8_t reportId, const uint8_t* data, size_t length) {
    auto it = outputReportChars.find(reportId);
    if (it == outputReportChars.end()) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Output report %d not found", reportId);
        return false;
    }
    
    try {
        it->second->writeValue((uint8_t*)data, length);
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception sending output report: %s", e.what());
        return false;
    }
}

bool BLEHostClient::sendFeatureReport(uint8_t reportId, const uint8_t* data, size_t length) {
    auto it = featureReportChars.find(reportId);
    if (it == featureReportChars.end()) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Feature report %d not found", reportId);
        return false;
    }
    
    try {
        it->second->writeValue((uint8_t*)data, length);
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception sending feature report: %s", e.what());
        return false;
    }
}

bool BLEHostClient::getFeatureReport(uint8_t reportId, uint8_t* data, size_t& length) {
    auto it = featureReportChars.find(reportId);
    if (it == featureReportChars.end()) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Feature report %d not found", reportId);
        return false;
    }
    
    try {
        std::string value = it->second->readValue();
        if (value.length() > length) {
            ESP_LOGE(BLE_HOST_LOG_TAG, "Buffer too small for feature report");
            return false;
        }
        
        memcpy(data, value.data(), value.length());
        length = value.length();
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception reading feature report: %s", e.what());
        return false;
    }
}

void BLEHostClient::reportNotifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    if (clientInstance) {
        clientInstance->processReportData(pChar, pData, length);
    }
}

void BLEHostClient::processReportData(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length) {
    // Find report ID for this characteristic
    uint8_t reportId = 0;
    for (const auto& pair : inputReportChars) {
        if (pair.second == pChar) {
            reportId = pair.first;
            break;
        }
    }
    
    ReportData report(reportId, pData, length);
    
    ESP_LOGD(BLE_HOST_LOG_TAG, "Report received: ID=%d, Length=%d", reportId, length);
    
    if (reportCallback) {
        reportCallback(report);
    }
}

std::vector<ServiceInfo> BLEHostClient::getServices() const {
    std::vector<ServiceInfo> services;
    
    if (!pClient || !pClient->isConnected()) {
        return services;
    }
    
    std::map<std::string, BLERemoteService*>* serviceMap = pClient->getServices();
    if (!serviceMap) {
        return services;
    }
    
    for (const auto& pair : *serviceMap) {
        BLERemoteService* pService = pair.second;
        String uuid = pService->getUUID().toString().c_str();
        String name = "Unknown Service";
        
        // Map common service UUIDs to names
        if (uuid.equalsIgnoreCase(HID_SERVICE_UUID)) {
            name = "Human Interface Device";
        } else if (uuid.equalsIgnoreCase(DEVICE_INFORMATION_SERVICE_UUID)) {
            name = "Device Information";
        } else if (uuid.equalsIgnoreCase(BATTERY_SERVICE_UUID)) {
            name = "Battery Service";
        }
        
        ServiceInfo serviceInfo(uuid, name);
        
        // Get characteristics for this service
        std::map<std::string, BLERemoteCharacteristic*>* charMap = pService->getCharacteristics();
        if (charMap) {
            for (const auto& charPair : *charMap) {
                serviceInfo.characteristicUUIDs.push_back(charPair.second->getUUID().toString().c_str());
            }
        }
        
        services.push_back(serviceInfo);
    }
    
    return services;
}

void BLEHostClient::setReportCallback(std::function<void(const ReportData&)> callback) {
    reportCallback = callback;
}

void BLEHostClient::setConnectionCallback(std::function<void(ConnectionState)> callback) {
    connectionCallback = callback;
}

void BLEHostClient::onConnect(BLEClient* pClient) {
    ESP_LOGI(BLE_HOST_LOG_TAG, "BLE Client connected");
}

void BLEHostClient::onDisconnect(BLEClient* pClient) {
    ESP_LOGI(BLE_HOST_LOG_TAG, "BLE Client disconnected");
    connectionState = ConnectionState::DISCONNECTED;
    if (connectionCallback) {
        connectionCallback(connectionState);
    }
}

void BLEHostClient::printDeviceInfo() const {
    Serial.println("\n=== Device Information ===");
    Serial.printf("Address: %s\n", connectedDeviceAddress.c_str());
    Serial.printf("Manufacturer: %s\n", deviceInfo.manufacturerName.c_str());
    Serial.printf("Model: %s\n", deviceInfo.modelNumber.c_str());
    Serial.printf("Serial: %s\n", deviceInfo.serialNumber.c_str());
    Serial.printf("Firmware: %s\n", deviceInfo.firmwareRevision.c_str());
    Serial.printf("Hardware: %s\n", deviceInfo.hardwareRevision.c_str());
    Serial.printf("Software: %s\n", deviceInfo.softwareRevision.c_str());
    
    if (deviceInfo.vendorId != 0 || deviceInfo.productId != 0) {
        Serial.printf("Vendor ID: 0x%04X\n", deviceInfo.vendorId);
        Serial.printf("Product ID: 0x%04X\n", deviceInfo.productId);
        Serial.printf("Version: 0x%04X\n", deviceInfo.version);
    }
    
    if (deviceInfo.hasBatteryService) {
        Serial.printf("Battery Level: %d%%\n", deviceInfo.batteryLevel);
    }
    
    Serial.println("========================");
}

void BLEHostClient::printHIDInformation() const {
    Serial.println("\n=== HID Information ===");
    Serial.printf("HID Version: 0x%04X\n", hidInfo.bcdHID);
    Serial.printf("Country Code: %d\n", hidInfo.bCountryCode);
    Serial.printf("Flags: 0x%02X\n", hidInfo.flags);
    Serial.printf("Report Descriptor Size: %d bytes\n", hidInfo.reportDescriptor.size());
    
    Serial.println("\nReport Map (hex):");
    for (size_t i = 0; i < hidInfo.reportDescriptor.size(); i++) {
        if (i % 16 == 0) {
            Serial.printf("\n%04X: ", i);
        }
        Serial.printf("%02X ", hidInfo.reportDescriptor[i]);
    }
    Serial.println();
    
    if (!hidInfo.reportMap.empty()) {
        Serial.println("\nReport Structure:");
        for (const auto& pair : hidInfo.reportMap) {
            const HIDReportInfo& info = pair.second;
            String typeStr = (info.reportType == HID_REPORT_TYPE_INPUT) ? "Input" :
                           (info.reportType == HID_REPORT_TYPE_OUTPUT) ? "Output" : "Feature";
            Serial.printf("  Report ID %d: %s, Size: %d bits, %s\n", 
                         info.reportId, typeStr.c_str(), info.reportSize, info.description.c_str());
        }
    }
    
    Serial.println("=======================");
}

void BLEHostClient::printServices() const {
    auto services = getServices();
    
    Serial.println("\n=== Available Services ===");
    for (const auto& service : services) {
        Serial.printf("Service: %s\n", service.name.c_str());
        Serial.printf("  UUID: %s\n", service.uuid.c_str());
        Serial.printf("  Characteristics (%d):\n", service.characteristicUUIDs.size());
        for (const auto& charUuid : service.characteristicUUIDs) {
            Serial.printf("    %s\n", charUuid.c_str());
        }
        Serial.println();
    }
    Serial.println("===========================");
}