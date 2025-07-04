#include <Arduino.h>
#include <BLEDevice.h>
#include "ble_host_config.h"
#include "ble_scanner.h"
#include "ble_client.h"
#include "hid_parser.h"
#include "report_monitor.h"
#include "cli_handler.h"

// Global instances
BLEScanner scanner;
BLEHostClient client;
HIDReportParser parser;
ReportMonitor monitor;
CLIHandler cli;

// System status
bool systemInitialized = false;
unsigned long bootTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    
    bootTime = millis();
    
    Serial.println("\n" + String("==========================================="));
    Serial.println("ESP32 BLE Host Simulator");
    Serial.println("Phase 1 & 2 Implementation");
    Serial.println("Version 1.0.0");
    Serial.println("===========================================");
    
    // Initialize BLE
    Serial.print("Initializing BLE Device... ");
    BLEDevice::init(BLE_HOST_DEVICE_NAME);
    Serial.println("OK");
    
    // Initialize components
    Serial.print("Initializing Scanner... ");
    if (!scanner.initialize()) {
        Serial.println("FAILED");
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to initialize BLE scanner");
        return;
    }
    Serial.println("OK");
    
    Serial.print("Initializing Client... ");
    if (!client.initialize()) {
        Serial.println("FAILED");
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to initialize BLE client");
        return;
    }
    Serial.println("OK");
    
    Serial.print("Initializing Parser... ");
    // Parser doesn't need explicit initialization
    Serial.println("OK");
    
    Serial.print("Initializing Monitor... ");
    if (!monitor.initialize(&parser)) {
        Serial.println("FAILED");
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to initialize report monitor");
        return;
    }
    Serial.println("OK");
    
    Serial.print("Initializing CLI... ");
    if (!cli.initialize(&scanner, &client, &parser, &monitor)) {
        Serial.println("FAILED");
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to initialize CLI handler");
        return;
    }
    Serial.println("OK");
    
    // Setup callbacks
    scanner.setDeviceFoundCallback([](const ScannedDevice& device) {
        cli.onDeviceFound(device);
    });
    
    scanner.setScanCompleteCallback([]() {
        cli.onScanComplete();
    });
    
    client.setConnectionCallback([](ConnectionState state) {
        cli.onConnectionStateChanged(state);
    });
    
    client.setReportCallback([](const ReportData& report) {
        monitor.onReportReceived(report);
        cli.onReportReceived(report);
    });
    
    monitor.setReportCallback([](const ReportData& report) {
        // Additional report processing if needed
    });
    
    systemInitialized = true;
    
    Serial.println("\nSystem initialized successfully!");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Boot time: %lu ms\n", millis() - bootTime);
    
    cli.printWelcome();
}

void loop() {
    if (!systemInitialized) {
        delay(1000);
        return;
    }
    
    // Process CLI input
    cli.processInput();
    
    // Small delay to prevent watchdog issues
    delay(10);
}

// Error handler for critical errors
void handleCriticalError(const String& error) {
    Serial.println("\nCRITICAL ERROR: " + error);
    Serial.println("System halted. Please restart the device.");
    
    while (true) {
        delay(1000);
        ESP.restart(); // Auto-restart after 1 second
    }
}