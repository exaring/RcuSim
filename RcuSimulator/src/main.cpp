#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "globals.h"
#include "webserver.h"
#include "wifimanager.h"
#include "BleRemoteControl.h"
#include "main.h"
#include "utils.h"

BleRemoteControl bleRemoteControl(BLE_DEVICE_NAME, BLE_MANUFACTURER_NAME, BLE_INITIAL_BATTERY_LEVEL);

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
  } else if (bleRemoteControl.isAdvertising()) {
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
Preferences preferences;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool isBleAdvertising = false;
unsigned long startTime = 0;
unsigned long bootCount = 0;

// Status update interval for display refresh
unsigned long lastStatusUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 2000; // Update every 2 seconds

// Command mapping table with help texts
const CommandHandler commandHandlers[] = {
  // WiFi Configuration
  {"setssid",     cmdSetSSID,           true,  "setssid <ssid>               - Set the SSID of the WiFi network",                    "WiFi Configuration"},
  {"setpwd",      cmdSetPwd,            true,  "setpwd <password>            - Set the password of the WiFi network",               "WiFi Configuration"},
  {"setip",       cmdSetIP,             true,  "setip <ip>                   - Set the static IP address (format: xxx.xxx.xxx.xxx)", "WiFi Configuration"},
  {"setgateway",  cmdSetGateway,        true,  "setgateway <ip>              - Set the gateway address (format: xxx.xxx.xxx.xxx)",  "WiFi Configuration"},
  {"save",        cmdSaveConfig,        false, "save                         - Save the current WiFi configuration to NVM",          "WiFi Configuration"},
  {"connect",     cmdConnectWiFi,       false, "connect                      - Connect to the WiFi network with the current config", "WiFi Configuration"},
  {"config",      cmdShowConfig,        false, "config                       - Shows the current WiFi configuration",               "WiFi Configuration"},
  
  // BLE Remote Control
  {"pair",        cmdStartPairing,      false, "pair                         - Start BLE advertising for pairing",                   "BLE Remote Control"},
  {"ble-pair",    cmdStartPairing,      false, "ble-pair                     - Start BLE advertising for pairing",                   "BLE Remote Control"},
  {"stoppair",    cmdStopPairing,       false, "stoppair                     - Stop BLE advertising",                                "BLE Remote Control"},
  {"ble-stoppair", cmdStopPairing,      false, "ble-stoppair                 - Stop BLE advertising",                                "BLE Remote Control"},
  {"unpair",      cmdUnpair,            false, "unpair                       - Remove all stored BLE pairings",                      "BLE Remote Control"},
  {"ble-unpair",  cmdUnpair,            false, "ble-unpair                   - Remove all stored BLE pairings",                      "BLE Remote Control"},
  {"key",         cmdSendKey,           true,  "key <keyname> [delay]        - Press and release a key with optional delay (ms)",   "BLE Remote Control"},
  {"press",       cmdPressKey,          true,  "press <keyname>              - Press a key without releasing",                       "BLE Remote Control"},
  {"release",     cmdReleaseKey,        true,  "release <keyname>            - Release a previously pressed key",                    "BLE Remote Control"},
  {"releaseall",  cmdReleaseAllKeys,    false, "releaseall                   - Release all currently pressed keys",                  "BLE Remote Control"},
  {"battery",     cmdSetBatteryLevel,   true,  "battery <level>              - Set the reported battery level (0-100)",             "BLE Remote Control"},
  {"ble-status",  cmdShowBleStatus,     false, "ble-status                   - Show current BLE connection status",                  "BLE Remote Control"},
  {"seq",         cmdSendSequence,      true,  "seq <start> <end> <delay>    - Send sequence of hex values as keys",                "BLE Remote Control"},
  {"hex",         cmdSendHex,           true,  "hex <hex1> <hex2> [delay]    - Send hex key pair for custom controls",              "BLE Remote Control"},
  {"hex1",        cmdSendHex1,          true,  "hex1 <hex> [delay]           - Send 1-byte hex key",                                 "BLE Remote Control"},
  {"hex2",        cmdSendHex2,          true,  "hex2 <hex> [delay]           - Send 2-byte hex key",                                 "BLE Remote Control"},
  
  // System Commands
  {"help",        printHelp,            false, "help                         - Shows this help",                                      "System Commands"},
  {"reboot",      cmdReboot,            false, "reboot                       - Restarts the device",                                 "System Commands"},
  {"diag",        cmdDiag,              false, "diag                         - Shows diagnostic information",                        "System Commands"},
  
  {nullptr,       nullptr,              false, nullptr,                                                                               nullptr} // End marker
};

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

// Simplified processCommand function using command handler table
void processCommand(String command) {
  ParsedCommand parsed = parseCommand(command);
  
  // Find command handler
  for (const CommandHandler* handler = commandHandlers; handler->command != nullptr; handler++) {
    if (parsed.baseCommand.equalsIgnoreCase(handler->command)) {
      if (handler->needsParameter && parsed.firstParam.isEmpty()) {
        printParameterError("Command '" + parsed.baseCommand + "' requires a parameter");
        return;
      }
      handler->handler(parsed.firstParam);
      return;
    }
  }
  
  printUnknownCommandError(command);
}

// Generic function for hex commands
bool executeHexCommand(unsigned long param1, unsigned long param2, int delay, const String& commandName) {
  if (bleRemoteControl.sendMediaKey(param1, param2, delay)) {
    printSuccessMessage(commandName + " executed successfully (delay: " + String(delay) + "ms)");
    return true;
  } else {
    printErrorMessage("Failed to execute " + commandName);
    return false;
  }
}

#pragma region WiFi Commands
void cmdSetSSID(String ssid) {
  if (!validateNonEmpty(ssid, "Invalid SSID")) return;
  
  Serial.print("Set SSID to: ");
  Serial.println(ssid);
  wifiManager.setSSID(ssid);
}

void cmdSetPwd(String password) {
  if (!validateNonEmpty(password, "Invalid Password")) return;
  
  Serial.print("Set Password to: ");
  Serial.println(password);
  wifiManager.setPassword(password);
}

void cmdSetIP(String ip) {
  if (!validateNonEmpty(ip, "Missing IP address") ||
      !wifiManager.isValidIPAddress(ip)) {
    printParameterError("Invalid IP address");
    return;
  }
  
  Serial.print("Set IP to: ");
  Serial.println(ip);
  wifiManager.setStaticIP(IPAddress().fromString(ip));
}

void cmdSetGateway(String gateway) {
  if (!validateNonEmpty(gateway, "Missing Gateway address") ||
      !wifiManager.isValidIPAddress(gateway)) {
    printParameterError("Invalid Gateway address");
    return;
  }
  
  Serial.print("Set Gateway to: ");
  Serial.println(gateway);
  wifiManager.setGateway(IPAddress().fromString(gateway));
}

void cmdSaveConfig(String parameter) {
  if (wifiManager.hasUnsavedChanges()) {
    wifiManager.saveConfig();
    printSuccessMessage("Configuration saved!");
  } else {
    Serial.println("No changes to save.");
  }
}

void cmdConnectWiFi(String parameter) {
  Serial.println("Trying to establish WiFi connection...");
  wifiManager.setup();
  
  if (wifiManager.isConnected()) {
    printSuccessMessage("Connected to WiFi!");
    setupWebServer();
  } else {
    printErrorMessage("Failed to connect to WiFi.");
  }
}

void cmdShowConfig(String parameter) {
  wifiManager.printConfig();
}

void cmdReboot(String parameter) {
  Serial.println("ESP32 is restarting...");
  delay(1000);
  ESP.restart();
}

void cmdDiag(String parameter) {
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
#pragma endregion

#pragma region BLE Commands
void cmdStartPairing(String parameter) {
  if (isBleAdvertising) {
    printGenericError(1001, "BLE is already advertising");
    return;
  }
  
  bleRemoteControl.startAdvertising();
  isBleAdvertising = true;
  printStatusMessage(200, "BLE advertising started for pairing");
}

void cmdStopPairing(String parameter) {
  if (!isBleAdvertising) {
    printGenericError(1002, "BLE is not advertising");
    return;
  }
  
  bleRemoteControl.stopAdvertising();
  isBleAdvertising = false;
  printStatusMessage(200, "BLE advertising stopped");
}

void cmdUnpair(String parameter) {
  if (bleRemoteControl.removeBonding()) {
    printSuccessMessage("Pairing information removed successfully");
  } else {
    printGenericError(1003, "Failed to remove pairing information");
  }
}

void cmdSendHex(String parameter) {
  ParsedCommand parsed = parseTwoHexCommand(parameter);
  if (parsed.baseCommand.isEmpty()) {
    printParameterError("Missing parameters. Usage: hex <hex1> <hex2> [delay_ms]");
    return;
  }
  
  unsigned long startValue, endValue;
  if (!validateHexAndParse(parsed.firstParam, startValue, "Invalid start hex value") ||
      !validateHexAndParse(parsed.secondParam, endValue, "Invalid end hex value")) {
    return;
  }
  
  executeHexCommand(startValue, endValue, parsed.delayMs, "hex");
}

void cmdSendHex1(String parameter) {
  ParsedCommand parsed = parseHexCommand(parameter);
  if (parsed.baseCommand.isEmpty()) {
    printParameterError("Missing parameter. Usage: hex1 <hex> [delay_ms]");
    return;
  }
  
  unsigned long keyCode;
  if (!validateHexAndParse(parsed.firstParam, keyCode, "Invalid hex value")) {
    return;
  }
  
  executeHexCommand(keyCode, 0, parsed.delayMs, "hex1");
}

void cmdSendHex2(String parameter) {
  ParsedCommand parsed = parseHexCommand(parameter);
  if (parsed.baseCommand.isEmpty()) {
    printParameterError("Missing parameter. Usage: hex2 <hex> [delay_ms]");
    return;
  }
  
  unsigned long keyCode;
  if (!validateHexAndParse(parsed.firstParam, keyCode, "Invalid hex value")) {
    return;
  }
  
  executeHexCommand(0, keyCode, parsed.delayMs, "hex2");
}

void cmdSendKey(String parameter) {
  ParsedCommand parsed = parseKeyCommand(parameter);
  if (parsed.baseCommand.isEmpty()) {
    printParameterError("Missing parameter. Usage: key <keyname> [delay_ms]");
    return;
  }
  
  if (!checkBleConnection()) return;
  
  if (bleRemoteControl.sendKey(parsed.firstParam, parsed.delayMs)) {
    printSuccessMessage("Key '" + parsed.firstParam + "' executed (delay: " + String(parsed.delayMs) + "ms)");
  } else {
    printErrorMessage("Failed to process key: " + parsed.firstParam);
  }
}

void cmdPressKey(String parameter) {
  if (!validateNonEmpty(parameter, "Missing key parameter. Usage: press <keyname>") ||
      !checkBleConnection()) return;
  
  if (bleRemoteControl.sendPress(parameter)) {
    printSuccessMessage("Key pressed: " + parameter);
  } else {
    printErrorMessage("Failed to press key: " + parameter);
  }
}

void cmdReleaseKey(String parameter) {
  if (!validateNonEmpty(parameter, "Missing key parameter. Usage: release <keyname>") ||
      !checkBleConnection()) return;
  
  if (bleRemoteControl.sendRelease(parameter)) {
    printSuccessMessage("Key released: " + parameter);
  } else {
    printErrorMessage("Failed to release key: " + parameter);
  }
}

void cmdReleaseAllKeys(String parameter) {
  if (!checkBleConnection()) return;
  
  bleRemoteControl.releaseAll();
  printSuccessMessage("All keys released");
}

void cmdSetBatteryLevel(String parameter) {
  if (!validateNonEmpty(parameter, "Missing level parameter. Usage: battery <level>")) return;
  
  int level = parameter.toInt();
  if (!validateRange(level, 0, 100, "Invalid battery level. Must be between 0 and 100")) return;
  
  bleRemoteControl.setBatteryLevel(level);
  printSuccessMessage("Battery level set to " + String(level) + "%");
}

void cmdShowBleStatus(String parameter) {
  Serial.println("BLE Status:");
  Serial.print("  Device name: ");
  Serial.println(BLE_DEVICE_NAME);
  Serial.print("  Manufacturer: ");
  Serial.println(BLE_MANUFACTURER_NAME);
  Serial.print("  Connected: ");
  Serial.println(bleRemoteControl.isConnected() ? "Yes" : "No");
  Serial.print("  Advertising: ");
  Serial.println(isBleAdvertising ? "Yes" : "No");
  Serial.print("  Battery level: ");
  Serial.print(bleRemoteControl.getBatteryLevel());
  Serial.println("%");
}

void cmdSendSequence(String parameter) {
  if (parameter.isEmpty()) {
    printParameterError("Missing parameters. Usage: seq <start_hex> <end_hex> <delay_ms>\nExample: seq 0x20 0x7E 100");
    return;
  }
  
  if (!checkBleConnection()) return;
  
  // Parse parameters: start_hex end_hex delay_ms
  int firstSpace = parameter.indexOf(' ');
  if (firstSpace == -1) {
    printParameterError("Missing parameters. Usage: seq <start_hex> <end_hex> <delay_ms>");
    return;
  }
  
  String startHexStr = parameter.substring(0, firstSpace);
  String remaining = parameter.substring(firstSpace + 1);
  remaining.trim();
  
  int secondSpace = remaining.indexOf(' ');
  if (secondSpace == -1) {
    printParameterError("Missing delay parameter. Usage: seq <start_hex> <end_hex> <delay_ms>");
    return;
  }
  
  String endHexStr = remaining.substring(0, secondSpace);
  String delayStr = remaining.substring(secondSpace + 1);
  delayStr.trim();
  
  // Validate hex strings
  unsigned long startValue, endValue;
  if (!validateHexAndParse(startHexStr, startValue, "Invalid start hex format") ||
      !validateHexAndParse(endHexStr, endValue, "Invalid end hex format")) {
    return;
  }
  
  int delayMs = delayStr.toInt();
  
  // Validate parameters
  if (!validateRange(delayMs, 101, 10000, "Invalid delay. Must be greater than 100") ||
      startValue >= endValue) {
    printParameterError("Start value must be less than end value");
    return;
  }
  
  if (endValue > 0xFFFF) {
    printParameterError("Values must be within 16-bit range (0x0000-0xFFFF)");
    return;
  }
  
  // Execute sequence
  Serial.print(STATUS_PREFIX);
  Serial.print(" ");
  Serial.println("Starting key sequence...");
  Serial.print("Range: ");
  Serial.print(formatHex16((uint16_t)startValue));
  Serial.print(" to ");
  Serial.print(formatHex16((uint16_t)endValue));
  Serial.print(", Delay: ");
  Serial.print(delayMs);
  Serial.println("ms");
  
  unsigned long totalKeys = endValue - startValue + 1;
  unsigned long sentKeys = 0;
  
  for (unsigned long value = startValue; value <= endValue; value++) {
    // Create hex string with 0x prefix using utility function
    String hexKey = formatHex16((uint16_t)value);
    
    Serial.print("Key: ");
    Serial.println(hexKey);
    
    // Send the key
    if (bleRemoteControl.sendMediaKeyHex(hexKey, 1, 100)) {
      sentKeys++;
      
      // Progress reporting every 10% or every 100 keys, whichever is less frequent
      unsigned long progressInterval = max(1UL, min(100UL, totalKeys / 10));
      if (sentKeys % progressInterval == 0 || sentKeys == totalKeys) {
        Serial.print("Progress: ");
        Serial.print(sentKeys);
        Serial.print("/");
        Serial.print(totalKeys);
        Serial.print(" (");
        Serial.print((sentKeys * 100) / totalKeys);
        Serial.println("%)");
      }
    } else {
      Serial.print("Warning: Failed to send key ");
      Serial.println(hexKey);
    }
    
    // Wait specified delay between keys
    delay(delayMs);
  }
  
  printSuccessMessage("Sequence completed. Sent " + String(sentKeys) + " of " + String(totalKeys) + " keys");
}
#pragma endregion

// Generate help text from command handler table
void printHelp(String parameter) {
  Serial.println("\n=== BLE Remote Control Console Commands ===");
  
  String currentCategory = "";
  for (const CommandHandler* handler = commandHandlers; handler->command != nullptr; handler++) {
    // Skip duplicate commands (like ble-pair, ble-stoppair, etc.)
    if (String(handler->command).startsWith("ble-") && 
        String(handler->command) != "ble-status" && 
        String(handler->command) != "ble-pair" && 
        String(handler->command) != "ble-stoppair" && 
        String(handler->command) != "ble-unpair") {
      continue;
    }
    
    // Print category header
    if (currentCategory != String(handler->category)) {
      currentCategory = String(handler->category);
      Serial.println("\n--- " + currentCategory + " ---");
    }
    
    // Print command help
    Serial.println(handler->helpText);
  }
  
  Serial.println("\nNote: Commands are case-insensitive.");
  Serial.println("Hex values can be 8-bit (0x00-0xFF) or 16-bit (0x0000-0xFFFF)");
  Serial.println("Example: seq 0x20 0x7E 100");
  Serial.println("=========================================");
}

// BLE setup
void setupBLE() {
  // Initialize BLE functionality, but don't start yet
  bleRemoteControl.begin();
  
  // Wait for initialization
  delay(500);
}