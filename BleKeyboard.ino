#include <functional>
/**
 * Extended BLE Keyboard with Serial Command Interface
 * Enables control via serial interface for BLE functions, key inputs and diagnostics
 */
#include "BleKeyboard.h"
#include "BleKeyboardConfig.h"

// Create BLE Keyboard with the configured parameters
BleKeyboard bleKeyboard(DEVICE_NAME, MANUFACTURER_NAME, INITIAL_BATTERY_LEVEL);
bool isAdvertising = false;

void setup() {
  Serial.begin(115200);
  Serial.println("BLE Keyboard with Command Interface");
  Serial.println("Enter 'help' to see available commands");
  
  // Register connection callback
  bleKeyboard.setConnectionCallback([](String message) {
    // This function is called when a BLE connection is established
    printStatus(STATUS_PAIRED, "Device connected");
  });
  
  // Set USB HID device properties
  bleKeyboard.set_vendor_id(VENDOR_ID);
  bleKeyboard.set_product_id(PRODUCT_ID);
  bleKeyboard.set_version(VERSION_ID);
  
  // Initialize BLE functionality, but don't start yet
  bleKeyboard.begin();
  
  // Wait for initialization
  delay(500);
  
  Serial.println("System ready. Waiting for commands.");
  
  // Print device configuration
  printConfiguration();
}

void loop() {
  // Process serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      processCommand(command);
    }
  }
  
  // Periodically check status and report (optional)
  delay(100);
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
  
  // Process commands
  if (baseCommand == "help") {
    printHelp();
  }
  else if (baseCommand == "pair") {
    startAdvertising();
  }
  else if (baseCommand == "stoppair" || baseCommand == "stop") {
    stopAdvertising();
  }
  else if (baseCommand == "unpair") {
    if (bleKeyboard.removeBonding()) {
      printStatus(STATUS_OK, "All pairing information removed");
    } else {
      printError(ERR_COMMAND_FAILED, "Could not remove pairing information");
    }
  }
  else if (baseCommand == "battery") {
    // Set battery level: battery <percentage>
    if (parameter.length() == 0) {
      printError(ERR_INVALID_PARAMETER, "No battery level specified");
      return;
    }
    
    int batteryLevel = parameter.toInt();
    if (batteryLevel < 0 || batteryLevel > 100) {
      printError(ERR_INVALID_PARAMETER, "Battery level must be between 0 and 100");
      return;
    }
    
    bleKeyboard.setBatteryLevel(batteryLevel);
    printStatus(STATUS_OK, "Battery level set to " + String(batteryLevel) + "%");
  }
  else if (baseCommand == "reboot") {
    printStatus(STATUS_OK, "Rebooting device...");
    delay(500);  // Short delay to ensure message is sent
    ESP.restart();
  }
  else if (baseCommand == "key") {
    // Format: key <keyname> [delay]
    if (parameter.length() == 0) {
      printError(ERR_INVALID_PARAMETER, "No key specified");
      return;
    }
    
    if (!bleKeyboard.isConnected()) {
      printError(ERR_NOT_CONNECTED, "Not connected to a host");
      return;
    }
    
    // Split parameter into key and optional delay
    int delayIndex = parameter.indexOf(' ');
    String keyName;
    int keyDelay = 50; // Default value: 50ms
    
    if (delayIndex != -1) {
      keyName = parameter.substring(0, delayIndex);
      String delayStr = parameter.substring(delayIndex + 1);
      delayStr.trim();
      keyDelay = delayStr.toInt();
      if (keyDelay <= 0) {
        keyDelay = 50; // Default if invalid value
      }
    } else {
      keyName = parameter;
    }
    
    // Determine key and press
    uint8_t keyCode = 0;
    
    // Check if keyName is a hex value (format: 0xXX)
    if (keyName.startsWith("0x") && keyName.length() > 2) {
      // Convert hex string to integer
      char* endPtr;
      keyCode = (uint8_t)strtol(keyName.c_str(), &endPtr, 16);
      if (*endPtr != '\0') {
        // Conversion failed
        printError(ERR_INVALID_PARAMETER, "Invalid hex key code: " + keyName);
        return;
      }
    } else if (keyName.length() == 1) {
      // Single character
      keyCode = keyName.charAt(0);
    } else {
      // Special key via mapping
      keyCode = getKeyCode(keyName);
      if (keyCode == 0) {
        printError(ERR_KEY_NOT_FOUND, "Unknown key: " + keyName);
        return;
      }
    }
    
    // Press key, wait, and release
    bleKeyboard.press(keyCode);
    delay(keyDelay);
    bleKeyboard.release(keyCode);
    
    printStatus(STATUS_OK, "Key '" + keyName + "' pressed and released after " + String(keyDelay) + "ms");
  }
  else if (baseCommand == "press") {
    if (parameter.length() == 0) {
      printError(ERR_INVALID_PARAMETER, "No key specified");
      return;
    }
    
    if (!bleKeyboard.isConnected()) {
      printError(ERR_NOT_CONNECTED, "Not connected to a host");
      return;
    }
    
    // Check if parameter is a hex value (format: 0xXX)
    if (parameter.startsWith("0x") && parameter.length() > 2) {
      // Convert hex string to integer
      char* endPtr;
      uint8_t keyCode = (uint8_t)strtol(parameter.c_str(), &endPtr, 16);
      if (*endPtr != '\0') {
        // Conversion failed
        printError(ERR_INVALID_PARAMETER, "Invalid hex key code: " + parameter);
        return;
      }
      
      bleKeyboard.press(keyCode);
      printStatus(STATUS_OK, "Key pressed: 0x" + String(keyCode, HEX));
    } else if (parameter.length() == 1) {
      // Single character
      bleKeyboard.press(parameter.charAt(0));
      printStatus(STATUS_OK, "Key pressed: " + parameter);
    } else {
      // Special key via mapping
      uint8_t keyCode = getKeyCode(parameter);
      if (keyCode != 0) {
        bleKeyboard.press(keyCode);
        printStatus(STATUS_OK, "Key pressed: " + parameter);
      } else {
        printError(ERR_KEY_NOT_FOUND, "Unknown key: " + parameter);
      }
    }
  }
  else if (baseCommand == "release") {
    if (parameter.length() == 0) {
      printError(ERR_INVALID_PARAMETER, "No key specified");
      return;
    }
    
    if (!bleKeyboard.isConnected()) {
      printError(ERR_NOT_CONNECTED, "Not connected to a host");
      return;
    }
    
    // Check if parameter is a hex value (format: 0xXX)
    if (parameter.startsWith("0x") && parameter.length() > 2) {
      // Convert hex string to integer
      char* endPtr;
      uint8_t keyCode = (uint8_t)strtol(parameter.c_str(), &endPtr, 16);
      if (*endPtr != '\0') {
        // Conversion failed
        printError(ERR_INVALID_PARAMETER, "Invalid hex key code: " + parameter);
        return;
      }
      
      bleKeyboard.release(keyCode);
      printStatus(STATUS_OK, "Key released: 0x" + String(keyCode, HEX));
    } else if (parameter.length() == 1) {
      // Single character
      bleKeyboard.release(parameter.charAt(0));
      printStatus(STATUS_OK, "Key released: " + parameter);
    } else {
      // Special key via mapping
      uint8_t keyCode = getKeyCode(parameter);
      if (keyCode != 0) {
        bleKeyboard.release(keyCode);
        printStatus(STATUS_OK, "Key released: " + parameter);
      } else {
        printError(ERR_KEY_NOT_FOUND, "Unknown key: " + parameter);
      }
    }
  }
  else if (baseCommand == "type") {
    if (parameter.length() == 0) {
      printError(ERR_INVALID_PARAMETER, "No text specified");
      return;
    }
    
    if (!bleKeyboard.isConnected()) {
      printError(ERR_NOT_CONNECTED, "Not connected to a host");
      return;
    }
    
    bleKeyboard.print(parameter);
    printStatus(STATUS_OK, "Text sent: " + parameter);
  }
  else if (baseCommand == "releaseall") {
    if (!bleKeyboard.isConnected()) {
      printError(ERR_NOT_CONNECTED, "Not connected to a host");
      return;
    }
    
    bleKeyboard.releaseAll();
    printStatus(STATUS_OK, "All keys released");
  }
  else if (baseCommand == "diag") {
    printDiagnostics();
  }
  else if (baseCommand == "config") {
    printConfiguration();
  }
  else if (baseCommand == "disconnect") {
    if (!bleKeyboard.isConnected()) {
      printError(ERR_NOT_CONNECTED, "Not connected to a host");
      return;
    }
    
    if (bleKeyboard.disconnect()) {
      printStatus(STATUS_DISCONNECTED, "Connection terminated");
    } else {
      printError(ERR_COMMAND_FAILED, "Could not terminate connection");
    }
  }
  else {
    printError(ERR_UNKNOWN_COMMAND, "Unknown command: " + baseCommand);
  }
}

void printError(int errorCode, String message) {
  Serial.print(ERR_PREFIX);
  Serial.print(" [");
  Serial.print(errorCode);
  Serial.print("] ");
  Serial.println(message);
}

void printStatus(int statusCode, String message) {
  Serial.print(STATUS_PREFIX);
  Serial.print(" [");
  Serial.print(statusCode);
  Serial.print("] ");
  Serial.println(message);
}

void printHelp() {
  Serial.println("\n=== BLE Keyboard Commands ===");
  Serial.println("help                  - Shows this help");
  Serial.println("pair                  - Activates pairing mode and starts BLE advertising");
  Serial.println("stoppair / stop       - Stops BLE advertising");
  Serial.println("unpair                - Removes all stored pairings");
  Serial.println("press <key>           - Presses a key (can use name, character, or 0xXX format)");
  Serial.println("release <key>         - Releases a key (can use name, character, or 0xXX format)");
  Serial.println("key <key> [delay]     - Presses a key and releases it after delay ms (default: 50ms)");
  Serial.println("type <text>           - Sends a text");
  Serial.println("releaseall            - Releases all pressed keys");
  Serial.println("disconnect            - Terminates the current connection");
  Serial.println("battery <percentage>  - Sets the reported battery level (0-100)");
  Serial.println("reboot                - Restarts the device");
  Serial.println("diag                  - Shows diagnostic information");
  Serial.println("config                - Shows current device configuration");
  Serial.println("\n=== Key Formats ===");
  Serial.println("- Single character    - Example: a, B, 7");
  Serial.println("- Named key           - Example: enter, f1, up, space");
  Serial.println("- Hex value           - Example: 0x28, 0xE2, 0x1A");
  Serial.println("\n=== Available Named Keys ===");
  
  // Show 5 special keys per line
  int keysPerLine = 5;
  for (int i = 0; i < NUM_KEY_MAPPINGS; i++) {
    Serial.print(keyMappings[i].name);
    if ((i + 1) % keysPerLine != 0 && i < NUM_KEY_MAPPINGS - 1) {
      Serial.print(", ");
    } else {
      Serial.println();
    }
  }
  Serial.println();
}

uint8_t getKeyCode(String key) {
  key.toLowerCase();
  
  for (int i = 0; i < NUM_KEY_MAPPINGS; i++) {
    if (key.equals(keyMappings[i].name)) {
      return keyMappings[i].keyCode;
    }
  }
  
  return 0; // Not found
}

void startAdvertising() {
  if (isAdvertising) {
    printError(ERR_ALREADY_ADVERTISING, "BLE advertising is already running");
    return;
  }
  
  // Direct call to the extended BleKeyboard method
  bleKeyboard.startAdvertising();
  isAdvertising = true;
  printStatus(STATUS_PAIRING, "Pairing mode activated");
}

void stopAdvertising() {
  if (!isAdvertising) {
    printError(ERR_NOT_ADVERTISING, "BLE advertising is not active");
    return;
  }
  
  // Direct call to the extended BleKeyboard method
  bleKeyboard.stopAdvertising();
  isAdvertising = false;
  printStatus(STATUS_OK, "BLE advertising stopped");
}

void printDiagnostics() {
  Serial.println("\n=== BLE Keyboard Diagnostics ===");
  Serial.print("Connection status: ");
  if (bleKeyboard.isConnected()) {
    Serial.println("Connected");
  } else {
    Serial.println("Not connected");
  }
  
  Serial.print("Advertising status: ");
  Serial.println(isAdvertising ? "Active" : "Inactive");
  
  // Display battery status
  Serial.print("Battery level: ");
  // Try to get the battery level - we can't directly access the battery level value from here
  // So we'll show a placeholder message
  Serial.println("Current configured value"); // This doesn't show the actual value
  
  // Display memory info
  Serial.print("Free memory: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" Bytes");
  
  // Additional ESP32-specific information
  Serial.print("ESP32 Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  
  Serial.print("ESP32 SDK Version: ");
  Serial.println(ESP.getSdkVersion());
  
  Serial.print("ESP32 CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  
  Serial.println("=== End of Diagnostics ===\n");
}

void printConfiguration() {
  Serial.println("\n=== Device Configuration ===");
  Serial.print("Device Name: ");
  Serial.println(DEVICE_NAME);
  Serial.print("Manufacturer: ");
  Serial.println(MANUFACTURER_NAME);
  Serial.print("Vendor ID: 0x");
  Serial.println(VENDOR_ID, HEX);
  Serial.print("Product ID: 0x");
  Serial.println(PRODUCT_ID, HEX);
  Serial.print("Version: 0x");
  Serial.println(VERSION_ID, HEX);
  Serial.print("Initial Battery Level: ");
  Serial.print(INITIAL_BATTERY_LEVEL);
  Serial.println("%");
  Serial.println("============================\n");
}