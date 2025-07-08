#include "BleRemoteControl.h"

bool parseHexString(String hexStr, uint16_t* result) {
  if (hexStr.length() == 0 || result == nullptr) {
    return false;
  }
  
  // String trimmen
  hexStr.trim();
  
  // Leerer String nach trim
  if (hexStr.length() == 0) {
    return false;
  }
  
  // Prefix entfernen falls vorhanden
  String cleanHex = hexStr;
  if (hexStr.startsWith("0x") || hexStr.startsWith("0X")) {
    cleanHex = hexStr.substring(2);
  }
  
  // Prüfen ob noch Zeichen übrig sind
  if (cleanHex.length() == 0) {
    return false;
  }
  
  // Maximale Länge für 16-bit prüfen (4 Hex-Zeichen = FFFF)
  if (cleanHex.length() > 4) {
    return false;
  }
  
  // Prüfen ob alle Zeichen gültige Hex-Zeichen sind
  for (unsigned int i = 0; i < cleanHex.length(); i++) {
    char c = cleanHex.charAt(i);
    if (!((c >= '0' && c <= '9') || 
          (c >= 'A' && c <= 'F') || 
          (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  
  // String zu uppercase konvertieren für einheitliche Verarbeitung
  cleanHex.toUpperCase();
  
  // Hex-String in Zahl konvertieren
  char* endPtr;
  unsigned long value = strtoul(cleanHex.c_str(), &endPtr, 16);
  
  // Prüfen ob Konvertierung erfolgreich war
  if (*endPtr != '\0') {
    return false;
  }
  
  // Prüfen ob Wert im 16-bit Bereich liegt
  if (value > 0xFFFF) {
    return false;
  }
  
  // Ergebnis setzen
  *result = (uint16_t)value;
  return true;
}

/**
 * Überladene Version die das Ergebnis direkt zurückgibt
 * Gibt 0 zurück bei Parsing-Fehlern (verwende die Pointer-Version für Fehlerbehandlung)
 */
uint16_t parseHexString(String hexStr) {
  uint16_t result = 0;
  parseHexString(hexStr, &result);
  return result;
}


BleRemoteControl::BleRemoteControl(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) {
    // Initialize media key report struct
    _mediaKeyReport.consumer1 = 0;
    _mediaKeyReport.consumer2 = 0;
    _mediaKeyReport.padding = 0;
}

void BleRemoteControl::begin(void)
{
  // Set USB HID device properties
  this->vid = VENDOR_ID;
  this->pid = PRODUCT_ID;
  this->version = VERSION_ID;

  BLEDevice::init(deviceName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  hid = new BLEHIDDevice(pServer);
  inputKeyboard = hid->inputReport(KEYBOARD_ID);  // <-- input REPORTID from report map
  outputKeyboard = hid->outputReport(KEYBOARD_ID);
  inputMediaKeys = hid->inputReport(MEDIA_KEYS_ID);

  outputKeyboard->setCallbacks(this);

  hid->manufacturer()->setValue(deviceManufacturer);

  hid->pnp(0x02, vid, pid, version);
  hid->hidInfo(0x00, 0x00);

  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();

  onStarted(pServer);

  advertising = pServer->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(false);
  
  // Don't start advertising automatically
  // Call startAdvertising() explicitly instead
  
  hid->setBatteryLevel(batteryLevel);

  ESP_LOGD(LOG_TAG, "BLE HID device initialized!");
}

bool BleRemoteControl::startAdvertising() {
	if (advertising != nullptr && !isAdvertisingMode) {
	  advertising->start();
	  isAdvertisingMode = true;
	  ESP_LOGD(LOG_TAG, "Advertising started!");
	  return true;
	}
	return false;
  }
  
void BleRemoteControl::stopAdvertising() {
	if (advertising != nullptr) {
	  advertising->stop();
	  isAdvertisingMode = false;
	  ESP_LOGD(LOG_TAG, "Advertising stopped!");
	}
  }

/**
 * @brief Removes all stored pairings and bondings
 * 
 * @return bool - true if the operation was successful
 */
bool BleRemoteControl::removeBonding() {
	// For standard BLE version (esp32-arduino BLE)
	// Direct access to the ESP-BLE-API for removing bondings
	
	// Remove all bonded devices
	int dev_num = esp_ble_get_bond_device_num();
	if (dev_num > 0) {
	  esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
	  esp_ble_get_bond_device_list(&dev_num, dev_list);
	  for (int i = 0; i < dev_num; i++) {
		esp_ble_remove_bond_device(dev_list[i].bd_addr);
	  }
	  free(dev_list);
	  return true;
	}
	
	return false;
  }
/**
 * @brief Actively disconnects from the connected device
 * 
 * @return bool - true if a device was connected and the connection was terminated, false otherwise
 */
bool BleRemoteControl::disconnect() {
	if (this->connected && pServer != nullptr) {
	  // Standard BLE Disconnect
	  // The disconnect method returns void, so check if connected
	  if (pServer->getConnectedCount() > 0) {
		// Use disconnect method
		pServer->disconnect(0); // 0 for all connections
		return true;
	  }
	}
	return false;
  }

void BleRemoteControl::onDisconnect(BLEServer* pServer) {
	this->connected = false;
	
	// Log disconnection
	ESP_LOGI(LOG_TAG, "Device disconnected");
  
	BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
	desc->setNotifications(false);
	desc = (BLE2902*)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
	desc->setNotifications(false);
  
	advertising->start();
  }

void BleRemoteControl::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}

/**
 * @brief Sets the delay time (in milliseconds) between multiple keystrokes
 * 
 * @param ms Time in milliseconds
 */
void BleRemoteControl::setDefaultDelay(uint32_t ms) {
  this->_delay_ms = ms;
}


bool BleRemoteControl::sendKey(String k, uint32_t delay_ms)
{
    if (!sendPress(k)) {
        return false;
    }
    delay(delay_ms);
    return sendRelease(k);
}

bool BleRemoteControl::sendPress(String k)
{
    // Check if the key is a media key
    if (isMediaKey(k)) {
        uint16_t mediaKeyCode = getMediaKeyCode(k);
        if (mediaKeyCode != 0) {
			sendMediaReport(mediaKeyCode);
            return true;
        }
        return false;
    }

    // Determine key and press
    uint8_t keyCode = 0;

    // Check if keyName is a hex value (format: 0xXX)
    if (k.startsWith("0x") && k.length() > 2) {
        // Convert hex string to integer
        char* endPtr;
        keyCode = (uint8_t)strtol(k.c_str(), &endPtr, 16);
        if (*endPtr != '\0') {
            // Conversion failed
            return false;
        }
    } else if (k.length() == 1) {
        // Single character
        keyCode = k.charAt(0);
    } else {
        // Special key via mapping
        keyCode = getKeyCode(k);
        if (keyCode == 0) {
            return false;
        }
    }
    
    // Press key
    press(keyCode);
    return true;
}

bool BleRemoteControl::sendMediaKeyHex(String k, uint8_t position, uint32_t delay_ms)
{
  uint16_t result = 0;
  if(!parseHexString(k, &result)) {
    return false;
  } else {
    if(position == 1) {
      sendMediaReport(result);
    } else if (position == 2)
    {
      sendMediaReport(0, result);
    } else {
      return false;
    }
    delay(delay_ms);
    sendMediaReport((uint16_t)0);
    return true;
  }
}

bool BleRemoteControl::sendMediaKey(uint16_t first, uint16_t second, uint32_t delay_ms)
{
  sendMediaReport(first, second);
  delay(delay_ms);
  sendMediaReport(0,0);
  return true;
}


bool BleRemoteControl::sendRelease(String k)
{
    // Check if the key is a media key
    if (isMediaKey(k)) {
		sendMediaReport((uint16_t)0);
		return true;
    }
    // Determine key and release
    uint8_t keyCode = 0;

    // Check if keyName is a hex value (format: 0xXX)
    if (k.startsWith("0x") && k.length() > 2) {
        // Convert hex string to integer
        char* endPtr;
        keyCode = (uint8_t)strtol(k.c_str(), &endPtr, 16);
        if (*endPtr != '\0') {
            // Conversion failed
            return false;
        }
    } else if (k.length() == 1) {
        // Single character
        keyCode = k.charAt(0);
    } else {
        // Special key via mapping
        keyCode = getKeyCode(k);
        if (keyCode == 0) {
            return false;
        }
    }
    
    // Release key
    release(keyCode);
    return true;
}

void BleRemoteControl::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
    _mediaKeyReport.consumer1 = 0;
    _mediaKeyReport.consumer2 = 0;
    _mediaKeyReport.padding = 0;
	sendKeyReport(&_keyReport);
	sendMediaReport(&_mediaKeyReport);
}

void BleRemoteControl::sendKeyReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();
  }	
}

void BleRemoteControl::sendMediaReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
	Serial.print("Sending Media Key Report: ");
	Serial.print("Consumer1: 0x");
	Serial.print(keys->consumer1, HEX);
	Serial.print(", Consumer2: 0x");
	Serial.print(keys->consumer2, HEX);
	Serial.print(", Padding: 0x");
	Serial.println(keys->padding, HEX);
    this->inputMediaKeys->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    this->inputMediaKeys->notify();
  }	
}

void BleRemoteControl::sendMediaReport(uint16_t key)
{
  if (this->isConnected())
  {
	Serial.print("Sending Single Media Key: 0x");
	Serial.println(key, HEX);
	
	_mediaKeyReport.consumer1 = key;
	_mediaKeyReport.consumer2 = 0;
	_mediaKeyReport.padding = 0;
	
	this->inputMediaKeys->setValue((uint8_t*)&_mediaKeyReport, sizeof(MediaKeyReport));
	this->inputMediaKeys->notify();
  }	
}

void BleRemoteControl::sendMediaReport(uint16_t key1, uint16_t key2)
{
  if (this->isConnected())
  {
	Serial.print("Sending Dual Media Keys: 0x");
	Serial.print(key1, HEX);
	Serial.print(", 0x");
	Serial.println(key2, HEX);
	
	_mediaKeyReport.consumer1 = key1;
	_mediaKeyReport.consumer2 = key2;
	_mediaKeyReport.padding = 0;
	
	this->inputMediaKeys->setValue((uint8_t*)&_mediaKeyReport, sizeof(MediaKeyReport));
	this->inputMediaKeys->notify();
  }	
}

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BleRemoteControl::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			return 0;
		}
		if (k & 0x80) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			return 0;
		}
	}
	sendKeyReport(&_keyReport);
	return 1;
}

size_t BleRemoteControl::press(const MediaKeyReport k)
{
    // Set the media key report values
    _mediaKeyReport.consumer1 |= k.consumer1;
    _mediaKeyReport.consumer2 |= k.consumer2;
    _mediaKeyReport.padding = 0xff;  // Always 0 for padding

	sendMediaReport(&_mediaKeyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleRemoteControl::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}

	sendKeyReport(&_keyReport);
	return 1;
}

size_t BleRemoteControl::release(const MediaKeyReport k)
{
    // Clear the specified media keys
    _mediaKeyReport.consumer1 &= ~k.consumer1;
    _mediaKeyReport.consumer2 &= ~k.consumer2;
    _mediaKeyReport.padding = 0;  // Always 0 for padding

	sendMediaReport(&_mediaKeyReport);
	return 1;
}

void BleRemoteControl::onConnect(BLEServer* pServer) {
  this->connected = true;

  // For regular BLE, log connection
  ESP_LOGI(LOG_TAG, "Device connected");

  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  desc = (BLE2902*)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);

  // Callback for external listeners
  if (connectCallback) {
    connectCallback("Connected");
  }
}

void BleRemoteControl::onWrite(BLECharacteristic* me) {
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  (void)value;
  ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}

void BleRemoteControl::delay_ms(uint64_t ms) {
  uint64_t m = esp_timer_get_time();
  if(ms){
    uint64_t e = (m + (ms * 1000));
    if(m > e){ //overflow
        while(esp_timer_get_time() > e) { }
    }
    while(esp_timer_get_time() < e) {}
  }
}

bool BleRemoteControl::isMediaKey(String key) {
	key.toLowerCase();
	for (int i = 0; i < NUM_MEDIA_KEY_MAPPINGS; i++) {
		if (key.equals(mediaKeyMappings[i].name)) {
		return true;
		}
	}  
	return false;
}

uint16_t BleRemoteControl::getMediaKeyCode(String key) {
	key.toLowerCase();
	
	for (int i = 0; i < NUM_MEDIA_KEY_MAPPINGS; i++) {
	  if (key.equals(mediaKeyMappings[i].name)) {
		return mediaKeyMappings[i].keyCode;
	  }
	}	
	return 0; // Not found
}

uint8_t BleRemoteControl::getKeyCode(String key) {
	key.toLowerCase();
	
	for (int i = 0; i < NUM_KEY_MAPPINGS; i++) {
	  if (key.equals(keyMappings[i].name)) {
		return keyMappings[i].keyCode;
	  }
	}
	
	return 0; // Not found
}