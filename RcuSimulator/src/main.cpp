#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "globals.h"
#include "webserver.h"
#include "wifimanager.h"
#include "BleRemoteControl.h"
#include "main.h"

// Optional OLED Display Support
// Uncomment the next line to enable OLED display support
#define USE_DISPLAY

#ifdef USE_DISPLAY
// Function to update display with current status
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Title
  display.println("ESP32 BLE Remote");
  display.drawLine(0, 8, display.width(), 8, SSD1306_WHITE);
  
  // WiFi Status
  display.setCursor(0, 10);
  if (wifiManager.isConnected()) {
    display.print("WiFi: ");
    display.println(wifiManager.ssid());
    display.print("> IP: ");
    display.println(wifiManager.localIp().toString());
    display.print("> RSSI: ");
    display.print(wifiManager.RSSI());
    display.println(" dBm");
  } else {
    display.println("WiFi: Disconnected");
    display.println("> Config: Use serial");
    display.println("> Baud: 115200,8,N,1");
  }  
  
  // BLE Status
  display.setCursor(0, 40);
  display.print("BLE: ");
  if (deviceConnected) {
    display.println("Connected");
  } else if (isBleAdvertising) {
    display.println("Advertising...");
  } else {
    display.println("Ready to connect");
  }

  // Show battery level if BLE is connected
  if (deviceConnected) {
    display.setCursor(0, 50);
    display.print("Battery: ");
    display.print(bleRemoteControl.getBatteryLevel());
    display.println("%");
  }  
  display.display();
}
#endif

// Global variables
bool isConfigMode = false;
WiFiManager wifiManager;
BleRemoteControl bleRemoteControl(BLE_DEVICE_NAME, BLE_MANUFACTURER_NAME, BLE_INITIAL_BATTERY_LEVEL);
Preferences preferences;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool isBleAdvertising = false;
unsigned long startTime = 0;
unsigned long bootCount = 0;

// Status update interval for display refresh
unsigned long lastStatusUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 2000; // Update every 2 seconds

// The main setup routine executed once at bootup
void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE Remote Control");

  // Initialize EEPROM
  preferences.begin("rcu-config", false);
  
  // Read boot count
  bootCount = preferences.getUInt("bootCount", 0);
  bootCount++;
  preferences.putUInt("bootCount", bootCount);
  Serial.printf("Boot count: %u\n", bootCount);
  
  // Save startup time
  startTime = millis();
  
#ifdef USE_DISPLAY
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    // Don't return, just continue without display
  } else {
    Serial.println("OLED display initialized");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32 BLE Remote");
    display.println("Initializing...");
    display.display();
  }
#endif
  
  setupSerial();
  wifiManager.setup();
  setupBLE();
  
  // If WiFi is connected, start web server
  if (wifiManager.isConnected()) {
    setupWebServer();
  }
  
#ifdef USE_DISPLAY
  // Initial display update
  updateDisplay();
#endif

  Serial.println("Setup completed");
}

// The main loop routine runs over and over again
void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      processCommand(command);
    }
  }
  
  // Handle WiFi manager updates
  wifiManager.loop();
  
#ifdef USE_DISPLAY
  // Update display periodically
  unsigned long currentTime = millis();
  if (currentTime - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
    updateDisplay();
    lastStatusUpdate = currentTime;
  }
#endif
  
  // Periodically check status and report (optional)
  delay(100);
}

void setupSerial() {
  // Initialize serial interface
  Serial.begin(115200);
  Serial.println("ESP32 BLE Remote Control - Startup");
}

void updateBootCounter() {
  bootCount = preferences.getULong("bootCount", 0);
  bootCount++;
  preferences.putULong("bootCount", bootCount);
}

void processCommand(String command) {
  // Split command into parts (base command and parameter)
  int spaceIndex = command.indexOf(' ');
  String baseCommand;
  String parameter = "";
  
  if (spaceIndex != -1) {
    baseCommand = command.substring(0, spaceIndex);
    parameter = command.substring(spaceIndex + 1);
    parameter.trim();
  } else {
    baseCommand = command;
  }
  
  baseCommand.toLowerCase();
  
  if (baseCommand == "help") {
    printHelp();
  } else if (baseCommand == "setssid") {
    cmdSetSSID(parameter);
  } else if (baseCommand == "setpwd") {
    cmdSetPwd(parameter);
  } else if (baseCommand == "setip") {
    cmdSetIP(parameter);
  } else if (baseCommand == "setgateway") {
    cmdSetGateway(parameter);
  } else if (baseCommand == "save") {
    cmdSaveConfig();
  } else if (baseCommand == "connect") {
    cmdConnectWiFi();
  } else if (baseCommand == "config") {
    cmdShowConfig();
  } else if (baseCommand == "reboot") {
    cmdReboot();
  } else if (baseCommand == "diag") {
    cmdDiag();
  } else {
    Serial.print("Unknown command: ");
    Serial.println(command);
  }
}

#pragma region WiFiManager Commands
void cmdSetSSID(String ssid) {
  if (ssid.length() > 0) {
    Serial.print("Set SSID to: ");
    Serial.println(ssid);
    wifiManager.setSSID(ssid);
  } else {
    Serial.println("Invalid SSID");
  }
}

void cmdSetPwd(String password) {
  if (password.length() > 0) {
    Serial.print("Set Password to: ");
    Serial.println(password);
    wifiManager.setPassword(password);
  } else {
    Serial.println("Invalid Password");
  }
}

void cmdSetIP(String ip) {
  if (wifiManager.isValidIPAddress(ip)) {
    Serial.print("Set IP to: ");
    Serial.println(ip);
    wifiManager.setStaticIP(IPAddress().fromString(ip));
  } else {
    Serial.println("Invalid IP address");
  }
}

void cmdSetGateway(String gateway) {
  if (wifiManager.isValidIPAddress(gateway)) {
    Serial.print("Set Gateway to: ");
    Serial.println(gateway);
    wifiManager.setGateway(IPAddress().fromString(gateway));
  } else {
    Serial.println("Invalid Gateway address");
  }
}

void cmdSaveConfig() {
  if (wifiManager.hasUnsavedChanges()) {
    wifiManager.saveConfig();
    Serial.println("Configuration saved!");
  } else {
    Serial.println("No changes to save.");
  }
}

void cmdConnectWiFi() {
  Serial.println("Trying to establish WiFi connection...");
  wifiManager.setup();
  if (wifiManager.isConnected()) {
    Serial.println("Connected to WiFi!");
    setupWebServer();
  } else {
    Serial.println("Failed to connect to WiFi.");
  }
}
#pragma endregion

void cmdReboot() {
  Serial.println("ESP32 is restarting...");
  delay(1000);
  ESP.restart();
}

void cmdShowConfig() {
  wifiManager.printConfig();
} 

void cmdDiag() {
  Serial.println("Diagnostic information:");
  Serial.print("  Boot counter: ");
  Serial.println(bootCount);
  Serial.print("  Uptime: ");
  Serial.print((millis() - startTime) / 1000);
  Serial.println(" seconds");
  Serial.print("  WiFi status: ");
  Serial.println(wifiManager.isConnected() ? "Connected" : "Not connected");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("  Current IP: ");
    Serial.println(wifiManager.localIp());
  }
}

void printHelp() {
  Serial.println("\n=== BLE Remote Control Console Commands ===");
  Serial.println("help                  - Shows this help");
  Serial.println("setssid               - set the ssid of the WiFi network");
  Serial.println("setpwd                - set the password of the WiFi network");
  Serial.println("setpwd                - set the password of the WiFi network");
  Serial.println("setip                 - set the static IP address (format: xxx.xxx.xxx.xxx)");
  Serial.println("setgateway            - set the gateway address (format: xxx.xxx.xxx.xxx)");
  Serial.println("save                  - save the current WiFi configuration to NVM");
  Serial.println("connect               - connect to the WiFi network with the current configuration");
  Serial.println("config                - shows the current WiFi configuration");
  Serial.println("reboot                - Restarts the device");
  Serial.println("diag                  - Shows diagnostic information");
  Serial.println("=========================================");
  Serial.println("Note: Commands are case-insensitive.");
  Serial.println("=========================================");
}

// BLE setup
void setupBLE() {
  bleRemoteControl.setConnectionCallback([](String message) {
    // This function is called when a BLE connection is established
    Serial.println("BLE - Device connected");
  });
  
  // Set USB HID device properties
  bleRemoteControl.set_vendor_id(VENDOR_ID);
  bleRemoteControl.set_product_id(PRODUCT_ID);
  bleRemoteControl.set_version(VERSION_ID);
  
  // Initialize BLE functionality, but don't start yet
  bleRemoteControl.begin();
  
  // Wait for initialization
  delay(500);
}