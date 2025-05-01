#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <IPAddress.h>

class WiFiManager {
private:
    // Preferences Namespace für Speicherung
    const char* preferencesNamespace = "wificonfig";
    
    // WLAN-Konfiguration
    String _ssid;
    String _password;
    IPAddress _staticIp;
    IPAddress _gateway;
    IPAddress _subnet;

    bool _unsavedChanges = false;
    bool _useStaticIp = false;
    
    // Preferences-Instance
    Preferences preferences;
    
    // Private Hilfsmethoden
    bool loadConfigFromPreferences();
    bool saveConfigToPreferences();
    void applyNetworkConfig();

public:
    WiFiManager();
    ~WiFiManager();
    
    // Verbindungsverwaltung
    bool setup();
    bool connect();
    bool disconnect();
    int status(); // Gibt den WiFi-Status zurück
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }  
   
    
    // Konfigurationsmethoden
    void setSSID(const String& ssid);
    void setPassword(const String& password);
    void useStaticIP(bool use);
    void setStaticIP(IPAddress ip);
    void setGateway(IPAddress gateway);
    void setSubnet(IPAddress subnet);
    
    // Konfigurationsspeicherung
    bool saveConfig();
    bool loadConfig();
    void resetConfig(); // Löscht die gespeicherte Konfiguration
    void printConfig();
    bool hasUnsavedChanges() const { return _unsavedChanges; }
    
    // Getter für die Properties
    String ssid() const { return _ssid; }
    String password() const { return _password; }
    IPAddress staticIp() const { return _staticIp; }
    IPAddress gateway() const { return _gateway; }
    IPAddress subnet() const { return _subnet; }
    IPAddress localIp() const { return WiFi.localIP(); }
    int RSSI() const { return WiFi.RSSI(); }
    int channel() const { return WiFi.channel(); }
    String BSSIDstr() const { return WiFi.BSSIDstr(); }
    String macAddress() const { return WiFi.macAddress(); }
    bool isUsingStaticIp() const { return _useStaticIp; }

    // Helper
    bool isValidIPAddress(const String& ip);
};

#endif // WIFI_MANAGER_H