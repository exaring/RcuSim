#include "utils.h"

static bool parseHexValueCore(const String& hexStr, unsigned long& result, size_t maxDigits) {
  String cleanHex = hexStr;
  cleanHex.trim();
  cleanHex.toUpperCase();

  if (cleanHex.startsWith("0X")) {
    cleanHex = cleanHex.substring(2);
  }
  
  if (cleanHex.length() == 0 || cleanHex.length() > maxDigits) {
    return false;
  }
  
  for (unsigned int i = 0; i < cleanHex.length(); i++) {
    char c = cleanHex.charAt(i);
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
      return false;
    }
  }
  
  char* endPtr;
  result = strtoul(cleanHex.c_str(), &endPtr, 16);
  return (*endPtr == '\0');
}

unsigned long parseHexValue(const String& hexStr) {
  unsigned long result = 0;
  parseHexValueCore(hexStr, result, 8); 
  return result;
}

bool isValidHexString(const String& hexStr) {
  unsigned long dummy;
  return parseHexValueCore(hexStr, dummy, 8); 
}

bool parseHexValue16(const String& hexStr, uint16_t& result) {
  unsigned long value;
  if (!parseHexValueCore(hexStr, value, 4)) { 
    return false;
  }
  
  if (value > 0xFFFF) {
    return false;
  }
  
  result = (uint16_t)value;
  return true;
}

// Optimized 8-bit hex parsing
bool parseHexValue8(const String& hexStr, uint8_t& result) {
  unsigned long value;
  if (!parseHexValueCore(hexStr, value, 2)) { 
    return false;
  }
  
  if (value > 0xFF) {
    return false;
  }
  
  result = (uint8_t)value;
  return true;
}
