#include "utils.h"

// Parameter parsing utilities
ParsedCommand parseCommand(const String& command) {
  ParsedCommand result;
  result.delayMs = 0;
  result.hasDelay = false;
  
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex != -1) {
    result.baseCommand = command.substring(0, spaceIndex);
    result.firstParam = command.substring(spaceIndex + 1);
    result.firstParam.trim();
  } else {
    result.baseCommand = command;
  }
  
  result.baseCommand.toLowerCase();
  return result;
}

ParsedCommand parseKeyCommand(const String& parameter, int defaultDelay) {
  ParsedCommand result;
  result.delayMs = defaultDelay;
  result.hasDelay = false;
  
  if (parameter.isEmpty()) {
    return result;
  }
  
  int spaceIndex = parameter.indexOf(' ');
  if (spaceIndex != -1) {
    result.firstParam = parameter.substring(0, spaceIndex);
    String delayStr = parameter.substring(spaceIndex + 1);
    delayStr.trim();
    result.delayMs = delayStr.toInt();
    result.hasDelay = true;
  } else {
    result.firstParam = parameter;
  }
  
  result.baseCommand = "parsed";
  return result;
}

ParsedCommand parseHexCommand(const String& parameter, int defaultDelay) {
  return parseKeyCommand(parameter, defaultDelay);
}

ParsedCommand parseTwoHexCommand(const String& parameter, int defaultDelay) {
  ParsedCommand result;
  result.delayMs = defaultDelay;
  result.hasDelay = false;
  
  if (parameter.isEmpty()) {
    return result;
  }
  
  int firstSpace = parameter.indexOf(' ');
  if (firstSpace == -1) {
    return result;
  }
  
  result.firstParam = parameter.substring(0, firstSpace);
  String remaining = parameter.substring(firstSpace + 1);
  remaining.trim();
  
  int secondSpace = remaining.indexOf(' ');
  if (secondSpace != -1) {
    result.secondParam = remaining.substring(0, secondSpace);
    String delayStr = remaining.substring(secondSpace + 1);
    delayStr.trim();
    result.delayMs = delayStr.toInt();
    result.hasDelay = true;
  } else {
    result.secondParam = remaining;
  }
  
  result.baseCommand = "parsed";
  return result;
}

// Utility function to parse hex string to unsigned long
unsigned long parseHexValue(const String& hexStr) {
  String cleanHex = hexStr;
  cleanHex.trim();
  
  // Remove 0x or 0X prefix if present
  if (cleanHex.startsWith("0x") || cleanHex.startsWith("0X")) {
    cleanHex = cleanHex.substring(2);
  }
  
  // Convert to unsigned long
  return strtoul(cleanHex.c_str(), NULL, 16);
}

// Utility function to validate hex string format
bool isValidHexString(const String& hexStr) {
  String cleanHex = hexStr;
  cleanHex.trim();
  
  // Remove 0x or 0X prefix if present
  if (cleanHex.startsWith("0x") || cleanHex.startsWith("0X")) {
    cleanHex = cleanHex.substring(2);
  }
  
  // Check if empty after removing prefix
  if (cleanHex.length() == 0) {
    return false;
  }
  
  // Check if all characters are valid hex digits
  for (unsigned int i = 0; i < cleanHex.length(); i++) {
    char c = cleanHex.charAt(i);
    if (!((c >= '0' && c <= '9') || 
          (c >= 'A' && c <= 'F') || 
          (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  
  return true;
}

// Utility function to format uint16_t as 4-digit hex string with 0x prefix
String formatHex16(uint16_t value) {
  String result = "0x";
  
  // Convert to hex string and pad to 4 digits
  String hexStr = String(value, HEX);
  hexStr.toUpperCase();
  
  // Pad with leading zeros to make it 4 digits
  while (hexStr.length() < 4) {
    hexStr = "0" + hexStr;
  }
  
  result += hexStr;
  return result;
}

// Validation utilities
bool validateNonEmpty(const String& value, const String& errorMessage) {
  if (value.isEmpty()) {
    printParameterError(errorMessage);
    return false;
  }
  return true;
}

bool validateRange(int value, int min, int max, const String& errorMessage) {
  if (value < min || value > max) {
    printParameterError(errorMessage);
    return false;
  }
  return true;
}

bool validateHexAndParse(const String& hexStr, unsigned long& result, const String& errorMessage) {
  if (!isValidHexString(hexStr)) {
    printParameterError(errorMessage);
    return false;
  }
  result = parseHexValue(hexStr);
  return true;
}

// Utility function to check BLE connection and print error if not connected
bool checkBleConnection() {
  if (!bleRemoteControl.isConnected()) {
    Serial.print(ERR_PREFIX);
    Serial.print(" ");
    Serial.println(ERR_NOT_CONNECTED);
    Serial.println("Not connected to a host device");
    return false;
  }
  return true;
}

// Utility function to print parameter error
void printParameterError(const String& usage) {
  Serial.print(ERR_PREFIX);
  Serial.print(" ");
  Serial.println(ERR_INVALID_PARAMETER);
  Serial.println(usage);
}

// Utility function to print success message
void printSuccessMessage(const String& message) {
  Serial.print(STATUS_PREFIX);
  Serial.print(" ");
  Serial.println(STATUS_OK);
  Serial.println(message);
}

// Utility function to print error message
void printErrorMessage(const String& message) {
  Serial.print(ERR_PREFIX);
  Serial.print(" ");
  Serial.println(ERR_KEY_NOT_FOUND);
  Serial.println(message);
}

// Utility function to print generic error with custom error code
void printGenericError(int errorCode, const String& message) {
  Serial.print(ERR_PREFIX);
  Serial.print(" ");
  Serial.println(errorCode);
  Serial.println(message);
}

// Utility function to print status message with custom status code
void printStatusMessage(int statusCode, const String& message) {
  Serial.print(STATUS_PREFIX);
  Serial.print(" ");
  Serial.println(statusCode);
  Serial.println(message);
}

// Utility function to print unknown command error
void printUnknownCommandError(const String& command) {
  Serial.print(ERR_PREFIX);
  Serial.print(" ");
  Serial.print(ERR_UNKNOWN_COMMAND);
  Serial.println(" Unknown command: " + command);
}