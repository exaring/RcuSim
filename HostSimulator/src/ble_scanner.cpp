#include "ble_scanner.h"
#include "ble_host_config.h"
#include <regex>

// Helper function to match regex patterns
static bool matchesPattern(const String& text, const String& pattern) {
    // Convert regex pattern to simple string matching
    String simplifiedPattern = pattern;
    
    // Remove common regex wildcards
    simplifiedPattern.replace(".*", "");
    simplifiedPattern.replace("*", "");
    simplifiedPattern.replace("^", "");
    simplifiedPattern.replace("$", "");
    
    // If pattern is empty after removing wildcards, it's a catch-all
    if (simplifiedPattern.isEmpty()) {
        return true;
    }
    
    // Case-insensitive matching
    String lowerText = text;
    String lowerPattern = simplifiedPattern;
    lowerText.toLowerCase();
    lowerPattern.toLowerCase();
    
    return lowerText.indexOf(lowerPattern) >= 0;
}

BLEScanner::BLEScanner() : 
    pBLEScan(nullptr), 
    isScanning(false), 
    scanDuration(BLE_SCAN_TIME_DEFAULT) {
}

BLEScanner::~BLEScanner() {
    if (isScanning) {
        stopScan();
    }
}

bool BLEScanner::initialize() {
    try {
        pBLEScan = BLEDevice::getScan();
        if (!pBLEScan) {
            ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to create BLE scan object");
            return false;
        }
        
        pBLEScan->setAdvertisedDeviceCallbacks(this);
        pBLEScan->setActiveScan(true); // Active scan for more information
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99);
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "BLE Scanner initialized successfully");
        return true;
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception during BLE scanner initialization: %s", e.what());
        return false;
    }
}

bool BLEScanner::startScan(uint32_t duration) {
    if (isScanning) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "Scan already in progress");
        return false;
    }
    
    if (!pBLEScan) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "BLE Scanner not initialized");
        return false;
    }
    
    scanDuration = duration;
    clearDevices(); // Clear previous results
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Starting BLE scan for %d seconds", duration);
    Serial.printf("Scanning for BLE devices (duration: %ds)...\n", duration);
    
    isScanning = true;
    
    // Start synchronous scan and handle completion manually
    BLEScanResults results = pBLEScan->start(duration, false);
    
    isScanning = false;
    ESP_LOGI(BLE_HOST_LOG_TAG, "Scan completed. Found %d devices", scannedDevices.size());
    Serial.printf("Scan completed. Found %d devices\n", scannedDevices.size());
    
    if (scanCompleteCallback) {
        scanCompleteCallback();
    }
    
    return true;
}

void BLEScanner::stopScan() {
    if (!isScanning || !pBLEScan) {
        return;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Stopping BLE scan");
    pBLEScan->stop();
    isScanning = false;
}

void BLEScanner::setFilter(const ScanFilter& filter) {
    currentFilter = filter;
    ESP_LOGI(BLE_HOST_LOG_TAG, "Scan filter updated");
}

void BLEScanner::clearFilter() {
    currentFilter = ScanFilter();
    ESP_LOGI(BLE_HOST_LOG_TAG, "Scan filter cleared");
}

ScannedDevice BLEScanner::getDevice(size_t index) const {
    if (index >= scannedDevices.size()) {
        return ScannedDevice(); // Return invalid device
    }
    return scannedDevices[index];
}

ScannedDevice BLEScanner::getDeviceByAddress(const String& address) const {
    for (const auto& device : scannedDevices) {
        if (device.address.equalsIgnoreCase(address)) {
            return device;
        }
    }
    return ScannedDevice(); // Return invalid device
}

void BLEScanner::clearDevices() {
    scannedDevices.clear();
}

void BLEScanner::setDeviceFoundCallback(std::function<void(const ScannedDevice&)> callback) {
    deviceFoundCallback = callback;
}

void BLEScanner::setScanCompleteCallback(std::function<void()> callback) {
    scanCompleteCallback = callback;
}

void BLEScanner::onResult(BLEAdvertisedDevice advertisedDevice) {
    ScannedDevice device;
    device.address = advertisedDevice.getAddress().toString().c_str();
    device.name = advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "Unknown";
    device.rssi = advertisedDevice.getRSSI();
    device.scanTimestamp = millis();
    
    // Get manufacturer data if available
    if (advertisedDevice.haveManufacturerData()) {
        std::string manufacturerData = advertisedDevice.getManufacturerData();
        if (manufacturerData.length() >= 2) {
            uint16_t manufacturerId = (manufacturerData[1] << 8) | manufacturerData[0];
            device.manufacturer = "ID: 0x" + String(manufacturerId, HEX);
        }
    }
    
    // Check for services in advertisement
    if (advertisedDevice.haveServiceUUID()) {
        BLEUUID serviceUUID = advertisedDevice.getServiceUUID();
        String uuidStr = serviceUUID.toString().c_str();
        device.serviceUUIDs.push_back(uuidStr);
        
        if (uuidStr.equalsIgnoreCase(HID_SERVICE_UUID)) {
            device.hasHIDService = true;
        }
        if (uuidStr.equalsIgnoreCase(DEVICE_INFORMATION_SERVICE_UUID)) {
            device.hasDeviceInfoService = true;
        }
        if (uuidStr.equalsIgnoreCase(BATTERY_SERVICE_UUID)) {
            device.hasBatteryService = true;
        }
    }
    
    // Check for service data and complete service list
    if (advertisedDevice.haveServiceData()) {
        BLEUUID serviceDataUUID = advertisedDevice.getServiceDataUUID();
        std::string serviceData = advertisedDevice.getServiceData();
        
        String serviceUuid = serviceDataUUID.toString().c_str();
        device.serviceUUIDs.push_back(serviceUuid);
        
        if (serviceUuid.equalsIgnoreCase(HID_SERVICE_UUID)) {
            device.hasHIDService = true;
        }
        if (serviceUuid.equalsIgnoreCase(DEVICE_INFORMATION_SERVICE_UUID)) {
            device.hasDeviceInfoService = true;
        }
        if (serviceUuid.equalsIgnoreCase(BATTERY_SERVICE_UUID)) {
            device.hasBatteryService = true;
        }
    }
    
    // Determine device type
    device.deviceType = determineDeviceType(device.name, device.manufacturer, device.serviceUUIDs);
    
    // Apply filter
    if (!passesFilter(device)) {
        return;
    }
    
    // Check if device already exists and update or add
    if (deviceExists(device.address)) {
        updateDevice(device);
    } else {
        scannedDevices.push_back(device);
        ESP_LOGD(BLE_HOST_LOG_TAG, "New device found: %s (%s) RSSI: %d", 
                 device.address.c_str(), device.name.c_str(), device.rssi);
    }
    
    // Call callback if set
    if (deviceFoundCallback) {
        deviceFoundCallback(device);
    }
}

DeviceType BLEScanner::determineDeviceType(const String& name, const String& manufacturer, 
                                          const std::vector<String>& serviceUUIDs) {
    // Check for HID service first
    bool hasHID = false;
    for (const auto& uuid : serviceUUIDs) {
        if (uuid.equalsIgnoreCase(HID_SERVICE_UUID)) {
            hasHID = true;
            break;
        }
    }
    
    if (!hasHID) {
        // Also check by name patterns even without HID service detected
        for (const auto& pattern : KNOWN_DEVICE_PATTERNS) {
            if (matchesPattern(name, pattern.namePattern) || 
                (!manufacturer.isEmpty() && matchesPattern(manufacturer, pattern.manufacturerPattern))) {
                return pattern.deviceType;
            }
        }
        return DeviceType::UNKNOWN;
    }
    
    // Check against known patterns
    for (const auto& pattern : KNOWN_DEVICE_PATTERNS) {
        if (matchesPattern(name, pattern.namePattern) || 
            (!manufacturer.isEmpty() && matchesPattern(manufacturer, pattern.manufacturerPattern))) {
            return pattern.deviceType;
        }
    }
    
    // Default to remote control if HID service is present
    return DeviceType::REMOTE_CONTROL;
}

bool BLEScanner::passesFilter(const ScannedDevice& device) {
    if (currentFilter.filterByName && !currentFilter.nameFilter.isEmpty()) {
        if (device.name.indexOf(currentFilter.nameFilter) < 0) {
            return false;
        }
    }
    
    if (currentFilter.filterByManufacturer && !currentFilter.manufacturerFilter.isEmpty()) {
        if (device.manufacturer.indexOf(currentFilter.manufacturerFilter) < 0) {
            return false;
        }
    }
    
    if (currentFilter.filterByRSSI) {
        if (device.rssi < currentFilter.minRSSI) {
            return false;
        }
    }
    
    return true;
}

bool BLEScanner::deviceExists(const String& address) {
    for (const auto& device : scannedDevices) {
        if (device.address.equalsIgnoreCase(address)) {
            return true;
        }
    }
    return false;
}

void BLEScanner::updateDevice(const ScannedDevice& device) {
    for (auto& existingDevice : scannedDevices) {
        if (existingDevice.address.equalsIgnoreCase(device.address)) {
            // Update RSSI and timestamp, keep other info
            existingDevice.rssi = device.rssi;
            existingDevice.scanTimestamp = device.scanTimestamp;
            break;
        }
    }
}

void BLEScanner::printScanResults() const {
    if (scannedDevices.empty()) {
        Serial.println("No devices found");
        return;
    }
    
    Serial.printf("\nFound %d device(s):\n", scannedDevices.size());
    Serial.println("Index | Address           | Name                 | RSSI | Type");
    Serial.println("------|-------------------|----------------------|------|----------------");
    
    for (size_t i = 0; i < scannedDevices.size(); i++) {
        const auto& device = scannedDevices[i];
        String deviceTypeName = "Unknown";
        switch (device.deviceType) {
            case DeviceType::KEYBOARD: deviceTypeName = "Keyboard"; break;
            case DeviceType::MOUSE: deviceTypeName = "Mouse"; break;
            case DeviceType::REMOTE_CONTROL: deviceTypeName = "Remote"; break;
            case DeviceType::GAME_CONTROLLER: deviceTypeName = "Controller"; break;
            case DeviceType::MULTIMEDIA_REMOTE: deviceTypeName = "Media Remote"; break;
            default: deviceTypeName = "Unknown"; break;
        }
        
        Serial.printf("%5d | %17s | %-20s | %4d | %s\n",
                     i, device.address.c_str(), 
                     device.name.length() > 20 ? (device.name.substring(0, 17) + "...").c_str() : device.name.c_str(),
                     device.rssi, deviceTypeName.c_str());
    }
    Serial.println();
}

void BLEScanner::printDevice(const ScannedDevice& device) const {
    Serial.println("Device Details:");
    Serial.printf("  Address: %s\n", device.address.c_str());
    Serial.printf("  Name: %s\n", device.name.c_str());
    Serial.printf("  Manufacturer: %s\n", device.manufacturer.c_str());
    Serial.printf("  RSSI: %d dBm\n", device.rssi);
    Serial.printf("  Device Type: ");
    
    switch (device.deviceType) {
        case DeviceType::KEYBOARD: Serial.println("Keyboard"); break;
        case DeviceType::MOUSE: Serial.println("Mouse"); break;
        case DeviceType::REMOTE_CONTROL: Serial.println("Remote Control"); break;
        case DeviceType::GAME_CONTROLLER: Serial.println("Game Controller"); break;
        case DeviceType::MULTIMEDIA_REMOTE: Serial.println("Multimedia Remote"); break;
        default: Serial.println("Unknown"); break;
    }
    
    Serial.printf("  Services:\n");
    Serial.printf("    HID Service: %s\n", device.hasHIDService ? "Yes" : "No");
    Serial.printf("    Device Info Service: %s\n", device.hasDeviceInfoService ? "Yes" : "No");
    Serial.printf("    Battery Service: %s\n", device.hasBatteryService ? "Yes" : "No");
    
    if (!device.serviceUUIDs.empty()) {
        Serial.println("  Service UUIDs:");
        for (const auto& uuid : device.serviceUUIDs) {
            Serial.printf("    %s\n", uuid.c_str());
        }
    }
    
    uint32_t ageSeconds = (millis() - device.scanTimestamp) / 1000;
    Serial.printf("  Last seen: %d seconds ago\n", ageSeconds);
}