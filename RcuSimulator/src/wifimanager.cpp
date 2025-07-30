#include "globals.h"
#include "WiFiManager.h"

WiFiManager::WiFiManager() {
    // Standardwerte für Netzwerkkonfiguration
    _staticIp = IPAddress(0, 0, 0, 0);
    _gateway = IPAddress(0, 0, 0, 0);
    _subnet = IPAddress(255, 255, 255, 0);
}

WiFiManager::~WiFiManager() {
    // Sicherstellen, dass Preferences geschlossen wird
    preferences.end();
}

bool WiFiManager::setup() {
    if(loadConfig()) {
        Serial.println("Loaded WiFi configuration from NVM:");
        Serial.print("  SSID: ");
        Serial.println(_ssid);
        Serial.print("  Password: ");
        Serial.println(_password);
        Serial.print("Static IP : ");
        Serial.println(isUsingStaticIp() ? "Yes" : "No (DHCP)");
        if (isUsingStaticIp()) {
          Serial.print("IP: ");
          Serial.println(staticIp().toString());
        }    
        if (connect()) {
          Serial.println("Connected to WiFi!");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          return true;
        } else {
          Serial.println("Failed to connect to WiFi!");
          return false;
        }
    } else {
        Serial.println("Failed to load WiFi configuration from NVM!");
        return false;
    }      
}  

bool WiFiManager::connect() {
    // WiFi-Modus setzen
    WiFi.mode(WIFI_STA);
    
    // Statische IP-Konfiguration anwenden, wenn aktiviert
    if (_useStaticIp) {
        applyNetworkConfig();
    }
    
    // Mit WLAN verbinden
    WiFi.begin(_ssid.c_str(), _password.c_str());
    
    // 10 Sekunden auf Verbindung warten
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
    }
    
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::disconnect() {
    return WiFi.disconnect();
}

int WiFiManager::status() {
    return WiFi.status();
}

void WiFiManager::setSSID(const String& ssid) {
    if (_ssid == ssid) return; 
    _ssid = ssid;
    _unsavedChanges = true;
}

void WiFiManager::setPassword(const String& password) {
    if (_password == password) return;
    _password = password;
    _unsavedChanges = true;
}

void WiFiManager::useStaticIP(bool use) {
    if (_useStaticIp == use) return;
    _useStaticIp = use;
    _unsavedChanges = true;
}

void WiFiManager::setStaticIP(IPAddress ip) {
    if (_staticIp == ip) return; 
    _staticIp = ip;
    _unsavedChanges = true;
}

void WiFiManager::setGateway(IPAddress gateway) {
    if (_gateway == gateway) return;
    _gateway = gateway;
    _unsavedChanges = true;
}

void WiFiManager::setSubnet(IPAddress subnet) {
    if (_subnet == subnet) return;
    _subnet = subnet;
    _unsavedChanges = true;
}

bool WiFiManager::saveConfig() {
    return saveConfigToPreferences();
}

bool WiFiManager::loadConfig() {
    return loadConfigFromPreferences();
}

void WiFiManager::resetConfig() {
    preferences.begin(preferencesNamespace, false);
    preferences.clear();
    preferences.end();
    
    // Zurücksetzen der Werte
    _ssid = "";
    _password = "";
    _staticIp = IPAddress(0, 0, 0, 0);
    _gateway = IPAddress(0, 0, 0, 0);
    _subnet = IPAddress(255, 255, 255, 0);
    _useStaticIp = false;
    _unsavedChanges = false;
}

void WiFiManager::printConfig() {
    Serial.println("WiFi Configuration:");
    Serial.print("> SSID: ");
    Serial.println(_ssid);
    Serial.print("> Password: ");
    Serial.println(_password);
    Serial.print("> Static IP : ");
    Serial.println(isUsingStaticIp() ? "Yes" : "No (DHCP)");
    Serial.print("> Gateway: ");
    Serial.println(_gateway.toString());
    Serial.print("> Connected: ");
    Serial.println(isConnected() ? "Yes" : "No");   
    if (isUsingStaticIp()) {
      Serial.print("> IP: ");
      Serial.println(staticIp().toString());
    };
    if (isConnected()) {
        if(!isUsingStaticIp()) {
          Serial.print("> IP: ");
          Serial.println(WiFi.localIP().toString());
        } 
        Serial.print("> RSSI - dBm: ");
        Serial.println(WiFi.RSSI());
        Serial.print("> BSSID: ");
        Serial.println(WiFi.BSSIDstr());
        Serial.print("> Channel: ");
        Serial.println(WiFi.channel());
      }     
}

bool WiFiManager::saveConfigToPreferences() {
    preferences.begin(preferencesNamespace, false);
    
    preferences.putString("ssid", _ssid);
    preferences.putString("password", _password);
    
    // IP-Adressen als Strings speichern
    preferences.putString("static_ip", _staticIp.toString());
    preferences.putString("gateway", _gateway.toString());
    preferences.putString("subnet", _subnet.toString());
    
    // Konfigurationsflags
    preferences.putBool("use_static", _useStaticIp);
    
    preferences.end();
    _unsavedChanges = false; 
    return true;
}

bool WiFiManager::loadConfigFromPreferences() {
    bool configExists = false;
    
    preferences.begin(preferencesNamespace, true); // Nur-Lesen-Modus
    
    if (preferences.isKey("ssid")) {
        _ssid = preferences.getString("ssid", "");
        _password = preferences.getString("password", "");
        
        // IP-Adressen aus Strings laden
        _staticIp.fromString(preferences.getString("static_ip", "0.0.0.0"));
        _gateway.fromString(preferences.getString("gateway", "0.0.0.0"));
        _subnet.fromString(preferences.getString("subnet", "255.255.255.0"));
        
        // Konfigurationsflags
        _useStaticIp = preferences.getBool("use_static", false);
        
        configExists = true;
        _unsavedChanges = false;
    }
    
    preferences.end();
    return configExists;
}

void WiFiManager::applyNetworkConfig() {
    if (_useStaticIp) {
      WiFi.config(_staticIp, _gateway, _subnet);
    }
}

/**
 * Checks if a string represents a valid IPv4 address.
 * 
 * A valid IPv4 address consists of four numbers between 0 and 255,
 * separated by dots.
 * 
 * @param ip The string to check
 * @return true if the string is a valid IPv4 address, otherwise false
 */
bool WiFiManager::isValidIPAddress(const String& ip) {
    // IP address must contain 3 dots
    int dotCount = 0;
    for (char c : ip) {
      if (c == '.') dotCount++;
    }
    if (dotCount != 3) return false;
    
    // Split IP address into 4 octets
    int octets[4] = {0};
    int octetIndex = 0;
    String octetStr = "";
    
    for (size_t i = 0; i <= ip.length(); i++) {
      // Process current octet at dots or end of string
      if (i == ip.length() || ip[i] == '.') {
        // Empty octet is invalid
        if (octetStr.length() == 0) return false;
        
        // Convert octet to a number
        int octet = octetStr.toInt();
        
        // Octet must be between 0 and 255
        if (octet < 0 || octet > 255) return false;
        
        // Leading zeros are only allowed for 0
        if (octet != 0 && octetStr[0] == '0') return false;
        if (octet == 0 && octetStr.length() > 1) return false;
        
        // Save octet
        if (octetIndex > 3) return false;  // Too many octets
        octets[octetIndex++] = octet;
        
        // Reset for next octet
        octetStr = "";
      } 
      // Add digits to current octet
      else if (isdigit(ip[i])) {
        octetStr += ip[i];
      }
      // Other characters than digits and dots are not allowed
      else {
        return false;
      }
    }
    
    // There must be exactly 4 octets
    return octetIndex == 4;
  }

void WiFiManager::loop() {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
        if (!_isConnected) {
            // Connection was established
            _isConnected = true;
        }
    } else {
        if (_isConnected) {
            // Connection was lost
            _isConnected = false;
            Serial.println("WiFi connection lost");
            
            // Optionally try to reconnect after a delay
            // We can implement automatic reconnection here if needed
        }
    }
}