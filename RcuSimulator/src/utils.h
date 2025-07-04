#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "globals.h"
#include "BleRemoteControl.h"

// External reference to BLE remote control object
extern BleRemoteControl bleRemoteControl;

// Parameter parsing utilities
struct ParsedCommand {
  String baseCommand;
  String firstParam;
  String secondParam;
  int delayMs;
  bool hasDelay;
};

// Parameter parsing functions
ParsedCommand parseCommand(const String& command);
ParsedCommand parseKeyCommand(const String& parameter, int defaultDelay = 100);
ParsedCommand parseHexCommand(const String& parameter, int defaultDelay = 100);
ParsedCommand parseTwoHexCommand(const String& parameter, int defaultDelay = 100);

// Hex parsing utilities
unsigned long parseHexValue(const String& hexStr);
bool isValidHexString(const String& hexStr);
String formatHex16(uint16_t value);

// Validation utilities
bool validateNonEmpty(const String& value, const String& errorMessage);
bool validateRange(int value, int min, int max, const String& errorMessage);
bool validateHexAndParse(const String& hexStr, unsigned long& result, const String& errorMessage);

// BLE connection utilities
bool checkBleConnection();

// Message printing utilities
void printParameterError(const String& usage);
void printSuccessMessage(const String& message);
void printErrorMessage(const String& message);
void printGenericError(int errorCode, const String& message);
void printStatusMessage(int statusCode, const String& message);
void printUnknownCommandError(const String& command);

#endif // UTILS_H