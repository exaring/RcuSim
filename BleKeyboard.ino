#include <functional>
/**
 * Extended BLE Keyboard with Serial Command Interface
 * Enables control via serial interface for BLE functions, key inputs and diagnostics
 */
#include "BleKeyboard.h"

// Error codes
#define ERR_PREFIX "ERROR:"
#define ERR_UNKNOWN_COMMAND 1001
#define ERR_INVALID_PARAMETER 1002
#define ERR_COMMAND_FAILED 1003
#define ERR_NOT_CONNECTED 1004
#define ERR_ALREADY_ADVERTISING 1005
#define ERR_NOT_ADVERTISING 1006
#define ERR_KEY_NOT_FOUND 1007

// Status
#define STATUS_PREFIX "STATUS:"
#define STATUS_OK 2000
#define STATUS_CONNECTED 2001
#define STATUS_DISCONNECTED 2002
#define STATUS_ADVERTISING 2003
#define STATUS_PAIRING 2004
#define STATUS_PAIRED 2005

BleKeyboard bleKeyboard;
bool isAdvertising = false;

// Struktur für Tastenmapping
struct KeyMapping {
  const char* name;
  uint8_t keyCode;
};

// Mapping von String-Namen zu Keycodes
const KeyMapping keyMappings[] = {
  {"up", KEY_UP_ARROW},
  {"down", KEY_DOWN_ARROW},
  {"left", KEY_LEFT_ARROW},
  {"right", KEY_RIGHT_ARROW},
  {"enter", KEY_RETURN},
  {"return", KEY_RETURN},
  {"esc", KEY_ESC},
  {"escape", KEY_ESC},
  {"backspace", KEY_BACKSPACE},
  {"tab", KEY_TAB},
  {"space", ' '},
  {"ctrl", KEY_LEFT_CTRL},
  {"alt", KEY_LEFT_ALT},
  {"shift", KEY_LEFT_SHIFT},
  {"win", KEY_LEFT_GUI},
  {"gui", KEY_LEFT_GUI},
  {"insert", KEY_INSERT},
  {"delete", KEY_DELETE},
  {"del", KEY_DELETE},
  {"home", KEY_HOME},
  {"end", KEY_END},
  {"pageup", KEY_PAGE_UP},
  {"pagedown", KEY_PAGE_DOWN},
  {"capslock", KEY_CAPS_LOCK},
  {"f1", KEY_F1},
  {"f2", KEY_F2},
  {"f3", KEY_F3},
  {"f4", KEY_F4},
  {"f5", KEY_F5},
  {"f6", KEY_F6},
  {"f7", KEY_F7},
  {"f8", KEY_F8},
  {"f9", KEY_F9},
  {"f10", KEY_F10},
  {"f11", KEY_F11},
  {"f12", KEY_F12},
  {"printscreen", KEY_PRINT_SCREEN},
  {"scrolllock", KEY_SCROLL_LOCK},
  {"pause", KEY_PAUSE},
  {"playpause", KEY_MEDIA_PLAY_PAUSE},
  {"nexttrack", KEY_MEDIA_NEXT_TRACK},
  {"prevtrack", KEY_MEDIA_PREVIOUS_TRACK},
  {"stop", KEY_MEDIA_STOP},
  {"mute", KEY_MEDIA_MUTE},
  {"volumeup", KEY_MEDIA_VOLUME_UP},
  {"volumedown", KEY_MEDIA_VOLUME_DOWN}
};

const int NUM_KEY_MAPPINGS = sizeof(keyMappings) / sizeof(KeyMapping);

// Funktionsdeklarationen
void processCommand(String command);
void printError(int errorCode, String message);
void printStatus(int statusCode, String message);
void printHelp();
uint8_t getKeyCode(String key);
void startAdvertising();
void stopAdvertising();
void printDiagnostics();

void setup() {
  Serial.begin(115200);
  Serial.println("BLE Keyboard with Command Interface");
  Serial.println("Enter 'help' to see available commands");
  
  // Register connection callback
  bleKeyboard.setConnectionCallback([](String address) {
    // This function is called when a BLE connection is established
    printStatus(STATUS_PAIRED, "Connected to device: " + address);
  });
  
  // Initialize BLE functionality, but don't start yet
  bleKeyboard.begin();
  
  // Wait for initialization
  delay(500);
  
  Serial.println("System ready. Waiting for commands.");
}

void loop() {
  // Serielle Befehle verarbeiten
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      processCommand(command);
    }
  }
  
  // Periodisch Status prüfen und melden (optional)
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
    if (keyName.length() == 1) {
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
    
    if (parameter.length() == 1) {
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
  
  if (parameter.length() == 1) {
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
  } else if (baseCommand == "type") {
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
  Serial.println("press <key>           - Presses a key");
  Serial.println("release <key>         - Releases a key");
  Serial.println("key <key> [delay]     - Presses a key and releases it after delay ms (default: 50ms)");
  Serial.println("type <text>           - Sends a text");
  Serial.println("releaseall            - Releases all pressed keys");
  Serial.println("disconnect            - Terminates the current connection");
  Serial.println("diag                  - Shows diagnostic information");
  Serial.println("\n=== Available Special Keys ===");
  
  // Alle 5 Sondertasten pro Zeile
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
  
  return 0; // Nicht gefunden
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
    
    // Display MAC address of connected device
    String peerAddress = bleKeyboard.getPeerAddress();
    if (peerAddress.length() > 0) {
      Serial.print("Connected to device: ");
      Serial.println(peerAddress);
    }
  } else {
    Serial.println("Not connected");
  }
  
  Serial.print("Advertising status: ");
  Serial.println(isAdvertising ? "Active" : "Inactive");
  
  // Display battery status
  Serial.print("Battery status: ");
  Serial.println("100%"); // Actual battery status could be queried here
  
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