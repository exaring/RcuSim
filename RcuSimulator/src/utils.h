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
bool parseHexValue16(const String& hexStr, uint16_t& result);
bool parseHexValue8(const String& hexStr, uint8_t& result);

#endif // UTILS_H