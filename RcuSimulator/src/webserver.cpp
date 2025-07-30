#include "globals.h"
#include "webserver.h"
#include "utils.h"
#include "BleRemoteControl.h"

AsyncWebServer server(80);
String authToken = "";

// Token management functions
void loadAuthToken() {
  Preferences preferences;
  preferences.begin("webserver", true);
  authToken = preferences.getString("auth_token", "");
  preferences.end();
  
  if (authToken.isEmpty()) {
    authToken = generateRandomToken();
    saveAuthToken(authToken);
    Serial.println("Generated new auth token: " + authToken);
  } else {
    Serial.println("Loaded auth token from storage");
  }
}

void saveAuthToken(const String& token) {
  Preferences preferences;
  preferences.begin("webserver", false);
  preferences.putString("auth_token", token);
  preferences.end();
  authToken = token;
}

String generateRandomToken() {
  String token = "";
  const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  for (int i = 0; i < 32; i++) {
    token += chars[random(0, sizeof(chars) - 1)];
  }
  return token;
}

bool validateToken(AsyncWebServerRequest *request) {
  if (!request->hasParam("token")) {
    return false;
  }
  
  String providedToken = request->getParam("token")->value();
  return providedToken.equals(authToken);
}

void sendUnauthorizedResponse(AsyncWebServerRequest *request) {
  sendJsonResponse(request, 401, "Unauthorized: Invalid or missing token");
}

// Helper function to check for key parameter in request
bool getKeyParameter(AsyncWebServerRequest *request, String &keyParam) {
  if (request->hasParam("key")) {
    keyParam = request->getParam("key")->value();
    return true;
  }
  return false;
}

// Helper function to generate HTML sections
String generateHtmlHeader() {
  return String(HTML_HEAD_START) + HTML_VIEWPORT + HTML_CSS_STYLES + 
         HTML_HEAD_END + HTML_BODY_START + HTML_TITLE;
}

String generateDeviceInfoSection() {
  String html = "<div class='info'><strong>Device name:</strong> " BLE_DEVICE_NAME "</div>";
  html += "<div class='info'><strong>IP address:</strong> " + wifiManager.localIp().toString() + "</div>";
  html += "<div class='info'><strong>MAC address:</strong> " + wifiManager.macAddress() + "</div>";
  html += "<div class='info'><strong>WiFi:</strong> " + wifiManager.ssid() + "</div>";
  html += "<div class='info'><strong>RSSI:</strong> " + String(wifiManager.RSSI()) + " dBm</div>";
  return html;
}

String generateApiSection(String title, String content) {
  return String(HTML_SECTION_START) + "<h3>" + title + "</h3>" + content + HTML_SECTION_END;
}

String generateMediaKeysFromMapping() {
  String mediaKeys = "<div class='endpoint'><strong>Media Keys:</strong> ";
  
  // Get media keys from the actual mapping array
  for (int i = 0; i < NUM_MEDIA_KEY_MAPPINGS; i++) {
    mediaKeys += mediaKeyMappings[i].name;
    if (i < NUM_MEDIA_KEY_MAPPINGS - 1) {
      mediaKeys += ", ";
    }
  }
  
  mediaKeys += "</div>";
  return mediaKeys;
}

String generateKeysSection() {
  return generateMediaKeysFromMapping() + String(HTML_NUMBER_KEYS) + HTML_KEY_TIPS;
}

void sendJsonResponse(AsyncWebServerRequest *request, int httpCode, String message) {
  StaticJsonDocument<256> doc;
  doc["status"] = httpCode;
  doc["message"] = message;
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  AsyncWebServerResponse *response = request->beginResponse(httpCode, "application/json", jsonResponse);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void setupWebServer() {
    Serial.println("Initializing web server and REST API...");
    
    // Load authentication token
    loadAuthToken();
    
    // API endpoint for pair command
    server.on("/api/pair", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      // Direct call to the extended BleRemoteControl method
      if (bleRemoteControl.startAdvertising())
      {
        Serial.println("BLE advertising started for pairing...");
        sendJsonResponse(request, 200, "BLE advertising started for pairing");
      } else {
        Serial.println("Failed to start BLE advertising for pairing");
        sendJsonResponse(request, 400, "Failed to start BLE advertising for pairing");
      }
    });

    // API endpoint for stoppair command
    server.on("/api/stoppair", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      if (!bleRemoteControl.isAdvertising()) {
        sendJsonResponse(request, 400, "BLE advertising is not active");
        return;
      }
      
      // Direct call to the extended BleRemoteControl method
      bleRemoteControl.stopAdvertising();
      sendJsonResponse(request, 200, "BLE advertising stopped");
    });

    // API endpoint for unpair command
    server.on("/api/unpair", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      if (bleRemoteControl.removeBonding()) {
        sendJsonResponse(request, 200, "Pairing information removed successfully");
      } else {
        sendJsonResponse(request, 400, "Failed to remove pairing information");
      }
    });

    // API endpoint for press command
    server.on("/api/press", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      String keyParam = "";
      if(!bleRemoteControl.isConnected()) {
        sendJsonResponse(request, 400, "{\"status\":\"error\",\"message\":\"Not connected to a host\"}");
        return;
      } 
      if (!getKeyParameter(request, keyParam)) {
        sendJsonResponse(request, 400, "{\"status\":\"error\",\"message\":\"Missing key parameter\"}");
        return;
      } 
      if (bleRemoteControl.sendPress(keyParam)) {
        sendJsonResponse(request, 200, "{\"status\":\"success\",\"message\":\"Key pressed: " + keyParam + "\"}");
      } else {
        sendJsonResponse(request, 400, "{\"status\":\"success\",\"message\":\"Failed to press key: " + keyParam + "\"}");
      }
    });

    // API endpoint for release command
    server.on("/api/release", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      String keyParam = "";
      if(!bleRemoteControl.isConnected()) {
        sendJsonResponse(request, 400, "{\"status\":\"error\",\"message\":\"Not connected to a host\"}");
        return;
      } 
      if (!getKeyParameter(request, keyParam)) {
        sendJsonResponse(request, 400, "{\"status\":\"error\",\"message\":\"Missing key parameter\"}");
        return;
      } 
      if (bleRemoteControl.sendRelease(keyParam)) {
        sendJsonResponse(request, 200, "{\"status\":\"success\",\"message\":\"Key released: " + keyParam + "\"}");
      } else {
        sendJsonResponse(request, 400, "{\"status\":\"success\",\"message\":\"Failed to release key: " + keyParam + "\"}");
      }
    });

    // API endpoint for releaseAll command
    server.on("/api/releaseall", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      if(!bleRemoteControl.isConnected()) {
        sendJsonResponse(request, 400, "Not connected to a host");
        return;
      }
      
      bleRemoteControl.releaseAll();
      sendJsonResponse(request, 200, "All keys released successfully");
    });

    // API endpoint for key command (press + optional delay + release)
    server.on("/api/key", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      String keyParam = "";
      int delayParam = 100; // Default delay value
      int responseCode = 400;
      String response = "{\"status\":\"error\",\"message\":\"Missing key parameter\"}";
      
      if (getKeyParameter(request, keyParam)) {
        // Check if delay parameter exists
        if (request->hasParam("delay")) {
          delayParam = request->getParam("delay")->value().toInt();
        }
        
        if(bleRemoteControl.sendKey(keyParam, delayParam))
        {
          response = "{\"status\":\"success\",\"message\":\"Key pressed and released: " + keyParam + 
                     "\", \"delay\":" + String(delayParam) + "}";
          responseCode = 200;
        } else {
          response = "{\"status\":\"error\",\"message\":\"Failed to process key: " + keyParam + "\"}";
        }
      }
      sendJsonResponse(request, responseCode, response);
    });

    // API endpoint for raw media key command (hex values)
    server.on("/api/rawmediakey", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      if (!bleRemoteControl.isConnected()) {
        sendJsonResponse(request, 400, "Not connected to a host");
        return;
      }
      
      if (!request->hasParam("value")) {
        sendJsonResponse(request, 400, "Missing value parameter");
        return;
      }
      
      String valueParam = request->getParam("value")->value();
      int delayParam = 100; // Default delay value
      
      // Check if delay parameter exists
      if (request->hasParam("delay")) {
        delayParam = request->getParam("delay")->value().toInt();
      }
      
      // Parse hex value
      uint16_t hexValue;
      if (!parseHexValue16(valueParam, hexValue)) {
        sendJsonResponse(request, 400, "Invalid hex value format (use 0xXX or 0xXXXX)");
        return;
      }
      
      if (bleRemoteControl.sendMediaKey(hexValue, 0, delayParam)) {
        StaticJsonDocument<256> doc;
        doc["status"] = "success";
        doc["message"] = "Raw media key sent";
        doc["value"] = "0x" + String(hexValue, HEX);
        doc["delay"] = delayParam;
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        sendJsonResponse(request, 200, jsonResponse);
      } else {
        sendJsonResponse(request, 400, "Failed to send raw media key");
      }
    });

    server.on("/api/system/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      String jsonResponse = getDeviceInfo();
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    });

    server.on("/api/system/battery", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      int levelParam = 100;
      String response = "{\"status\":\"error\",\"message\":\"Missing level parameter\"}";
      int responseCode = 400;
      
      // Check if key parameter exists
      if (request->hasParam("level")) {
        levelParam = request->getParam("level")->value().toInt();
        if(levelParam >= 0 && levelParam <= 100) {
          bleRemoteControl.setBatteryLevel(levelParam);
          response = "{\"status\":\"success\",\"message\":\"Battery level set to " + String(levelParam) + "\"}";
          responseCode = 200;
        } else {
          response = "{\"status\":\"error\",\"message\":\"Invalid battery level value'"+ String(levelParam) + "'\"}";
        }
      }
      sendJsonResponse(request, responseCode, response);
    });

    server.on("/api/system/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      String response = "{\"status\":\"success\",\"message\":\"Rebooting device...\"}";
      AsyncWebServerResponse *serverResponse = request->beginResponse(200, "application/json", response);
      serverResponse->addHeader("Access-Control-Allow-Origin", "*");
      request->send(serverResponse);
      delay(1000); // Delay to allow the response to be sent before rebooting
      ESP.restart(); // Reboot the device
    });
    
    // API endpoint to configure BLE device parameters
    server.on("/api/ble/config", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      // This will be called after body is received
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Handle POST body data
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      // Parse JSON from request body
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, (char*)data, len);
      
      if (error) {
        sendJsonResponse(request, 400, "Invalid JSON format");
        return;
      }
      
      bool configChanged = false;
      String errorMsg = "";
      String successMsg = "";
      
      // Only process Vendor ID if present in JSON
      if (doc.containsKey("vendorId")) {
        if (doc["vendorId"].is<const char*>()) {
          String vendorIdStr = doc["vendorId"].as<String>();
          uint16_t vendorId;
          if (parseHexValue16(vendorIdStr, vendorId) && vendorId > 0) {
            if (bleRemoteControl.setVendorId(vendorId)) {
              configChanged = true;
              successMsg += "Vendor ID updated. ";
            } else {
              errorMsg += "Failed to set vendor ID. ";
            }
          } else {
            errorMsg += "Invalid vendor ID hex format (use 0xXXXX). ";
          }
        } else {
          errorMsg += "Vendor ID must be a hex string. ";
        }
      }
      
      // Only process Product ID if present in JSON
      if (doc.containsKey("productId")) {
        if (doc["productId"].is<const char*>()) {
          String productIdStr = doc["productId"].as<String>();
          uint16_t productId;
          if (parseHexValue16(productIdStr, productId)) {
            if (bleRemoteControl.setProductId(productId)) {
              configChanged = true;
              successMsg += "Product ID updated. ";
            } else {
              errorMsg += "Failed to set product ID. ";
            }
          } else {
            errorMsg += "Invalid product ID hex format (use 0xXXXX). ";
          }
        } else {
          errorMsg += "Product ID must be a hex string. ";
        }
      }
      
      // Only process Version ID if present in JSON
      if (doc.containsKey("versionId")) {
        if (doc["versionId"].is<const char*>()) {
          String versionIdStr = doc["versionId"].as<String>();
          uint16_t versionId;
          if (parseHexValue16(versionIdStr, versionId)) {
            if (bleRemoteControl.setVersionId(versionId)) {
              configChanged = true;
              successMsg += "Version ID updated. ";
            } else {
              errorMsg += "Failed to set version ID. ";
            }
          } else {
            errorMsg += "Invalid version ID hex format (use 0xXXXX). ";
          }
        } else {
          errorMsg += "Version ID must be a hex string. ";
        }
      }
      
      // Only process Country Code if present in JSON
      if (doc.containsKey("countryCode")) {
        if (doc["countryCode"].is<const char*>()) {
          String countryCodeStr = doc["countryCode"].as<String>();
          uint8_t countryCode;
          if (parseHexValue8(countryCodeStr, countryCode)) {
            bleRemoteControl.setCountryCode(countryCode);
            configChanged = true;
            successMsg += "Country code updated. ";
          } else {
            errorMsg += "Invalid country code hex format (use 0xXX). ";
          }
        } else {
          errorMsg += "Country code must be a hex string. ";
        }
      }
      
      // Only process HID Flags if present in JSON
      if (doc.containsKey("hidFlags")) {
        if (doc["hidFlags"].is<const char*>()) {
          String hidFlagsStr = doc["hidFlags"].as<String>();
          uint8_t hidFlags;
          if (parseHexValue8(hidFlagsStr, hidFlags)) {
            bleRemoteControl.setHidFlags(hidFlags);
            configChanged = true;
            successMsg += "HID flags updated. ";
          } else {
            errorMsg += "Invalid HID flags hex format (use 0xXX). ";
          }
        } else {
          errorMsg += "HID flags must be a hex string. ";
        }
      }
      
      // Only process Device Name if present in JSON
      if (doc.containsKey("deviceName")) {
        if (doc["deviceName"].is<const char*>()) {
          String deviceName = doc["deviceName"].as<String>();
          if (deviceName.length() > 0 && deviceName.length() <= 64) {
            if (bleRemoteControl.setDeviceName(deviceName)) {
              configChanged = true;
              successMsg += "Device name updated. ";
            } else {
              errorMsg += "Failed to set device name. ";
            }
          } else {
            errorMsg += "Device name must be 1-64 characters. ";
          }
        } else {
          errorMsg += "Device name must be a string. ";
        }
      }
      
      // Only process Manufacturer Name if present in JSON
      if (doc.containsKey("manufacturerName")) {
        if (doc["manufacturerName"].is<const char*>()) {
          String manufacturerName = doc["manufacturerName"].as<String>();
          if (manufacturerName.length() <= 64) {
            bleRemoteControl.setManufacturerName(manufacturerName);
            configChanged = true;
            successMsg += "Manufacturer name updated. ";
          } else {
            errorMsg += "Manufacturer name must be max 64 characters. ";
          }
        } else {
          errorMsg += "Manufacturer name must be a string. ";
        }
      }
      
      // Only process Initial Battery Level if present in JSON
      if (doc.containsKey("initialBatteryLevel")) {
        if (doc["initialBatteryLevel"].is<int>()) {
          uint8_t batteryLevel = doc["initialBatteryLevel"];
          if (batteryLevel <= 100) {
            bleRemoteControl.setInitialBatteryLevel(batteryLevel);
            configChanged = true;
            successMsg += "Initial battery level updated. ";
          } else {
            errorMsg += "Battery level must be 0-100. ";
          }
        } else {
          errorMsg += "Battery level must be a number. ";
        }
      }
      
      // Only process MAC Address if present in JSON
      if (doc.containsKey("macAddress")) {
        if (doc["macAddress"].is<const char*>()) {
          String macAddress = doc["macAddress"].as<String>();
          if (bleRemoteControl.setMacAddress(macAddress)) {
            configChanged = true;
            successMsg += "MAC address updated. ";
          } else {
            errorMsg += "Invalid MAC address format (use AA:BB:CC:DD:EE:FF). ";
          }
        } else {
          errorMsg += "MAC address must be a string. ";
        }
      }
      
      // Check if any errors occurred
      if (!errorMsg.isEmpty()) {
        sendJsonResponse(request, 400, "Configuration errors: " + errorMsg);
        return;
      }
      
      // Save configuration if changes were made
      if (configChanged) {
        if (bleRemoteControl.saveConfiguration()) {
          StaticJsonDocument<256> responseDoc;
          responseDoc["status"] = "success";
          responseDoc["message"] = "BLE configuration updated successfully";
          responseDoc["details"] = successMsg.isEmpty() ? "Configuration saved" : successMsg;
          responseDoc["note"] = "Restart required for changes to take effect";
          
          String jsonResponse;
          serializeJson(responseDoc, jsonResponse);
          
          AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
          response->addHeader("Access-Control-Allow-Origin", "*");
          request->send(response);
        } else {
          sendJsonResponse(request, 500, "Failed to save configuration");
        }
      } else {
        sendJsonResponse(request, 200, "No configuration changes requested");
      }
    });

    // API endpoint to get current BLE configuration
    server.on("/api/ble/config", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      StaticJsonDocument<512> doc;
      doc["vendorId"] = "0x" + String(bleRemoteControl.getVendorId(), HEX);
      doc["productId"] = "0x" + String(bleRemoteControl.getProductId(), HEX);
      doc["versionId"] = "0x" + String(bleRemoteControl.getVersionId(), HEX);
      doc["countryCode"] = "0x" + String(bleRemoteControl.getCountryCode(), HEX);
      doc["hidFlags"] = "0x" + String(bleRemoteControl.getHidFlags(), HEX);
      doc["deviceName"] = bleRemoteControl.getDeviceName();
      doc["manufacturerName"] = bleRemoteControl.getManufacturerName();
      doc["initialBatteryLevel"] = bleRemoteControl.getInitialBatteryLevel();
      doc["currentBatteryLevel"] = bleRemoteControl.getBatteryLevel();
      doc["macAddress"] = bleRemoteControl.getCurrentMacAddressString();
      doc["usingCustomMac"] = bleRemoteControl.isUsingCustomMac();
      doc["connected"] = bleRemoteControl.isConnected();
      doc["advertising"] = bleRemoteControl.isAdvertising();
      
      String jsonResponse;
      serializeJson(doc, jsonResponse);
      
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    });

    // API endpoint to reset BLE configuration to defaults
    server.on("/api/ble/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }
      
      bleRemoteControl.resetConfiguration();
      
      StaticJsonDocument<256> doc;
      doc["status"] = "success";
      doc["message"] = "BLE configuration reset to defaults";
      doc["note"] = "Restart required for changes to take effect";
      
      String jsonResponse;
      serializeJson(doc, jsonResponse);
      
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    });

    // Doc endpoint - Token required for documentation
    server.on("/doc", HTTP_GET, [](AsyncWebServerRequest *request){
      if (!validateToken(request)) {
        sendUnauthorizedResponse(request);
        return;
      }

      // Generate complete HTML response step by step
      String htmlResponse = "";
      htmlResponse.reserve(8192); // Reserve memory to avoid fragmentation
      
      htmlResponse += "<!DOCTYPE html>";
      htmlResponse += "<html><head>";
      htmlResponse += "<title>ESP32 BLE Remote Control - Documentation</title>";
      htmlResponse += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
      htmlResponse += "<style>";
      htmlResponse += "body{font-family:Arial,sans-serif;margin:20px;line-height:1.6}";
      htmlResponse += "h1{color:#0066cc}";
      htmlResponse += "h2{color:#0066cc;margin-top:20px}";
      htmlResponse += ".container{max-width:800px;margin:0 auto;padding:20px;border:1px solid #ddd;border-radius:5px}";
      htmlResponse += ".info{margin-bottom:10px}";
      htmlResponse += ".api-section{margin-top:15px;padding:10px;background:#f7f7f7;border-radius:5px}";
      htmlResponse += ".endpoint{margin-bottom:8px}";
      htmlResponse += "a{color:#0066cc;text-decoration:none}";
      htmlResponse += "a:hover{text-decoration:underline}";
      htmlResponse += ".params{font-size:0.9em;color:#666;margin-left:20px}";
      htmlResponse += "</style>";
      htmlResponse += "</head>";
      htmlResponse += "<body><div class='container'>";
      htmlResponse += "<h1>ESP32 BLE Remote Control - Documentation</h1>";
      
      // Device information section
      htmlResponse += "<div class='info'><strong>Device name:</strong> ";
      htmlResponse += BLE_DEVICE_NAME;
      htmlResponse += "</div>";
      htmlResponse += "<div class='info'><strong>IP address:</strong> ";
      htmlResponse += wifiManager.localIp().toString();
      htmlResponse += "</div>";
      htmlResponse += "<div class='info'><strong>MAC address:</strong> ";
      htmlResponse += wifiManager.macAddress();
      htmlResponse += "</div>";
      htmlResponse += "<div class='info'><strong>WiFi:</strong> ";
      htmlResponse += wifiManager.ssid();
      htmlResponse += "</div>";
      htmlResponse += "<div class='info'><strong>RSSI:</strong> ";
      htmlResponse += String(wifiManager.RSSI());
      htmlResponse += " dBm</div>";
      
      // Get the current token from the request
      String currentToken = request->getParam("token")->value();
      String baseUrl = "http://" + wifiManager.localIp().toString();
      
      // Authentication info section
      htmlResponse += "<h2>Authentication</h2>";
      htmlResponse += "<div class='api-section'><h3>Authentication</h3>";
      htmlResponse += "<div class='endpoint'><strong>Token Required:</strong> All API endpoints require a 'token' parameter</div>";
      htmlResponse += "<div class='endpoint'>Current Token: <code>";
      htmlResponse += currentToken;
      htmlResponse += "</code></div>";
      htmlResponse += "</div>";
      
      // API endpoints section
      htmlResponse += "<h2>API Endpoints</h2>";
      
      // BLE control endpoints
      htmlResponse += "<div class='api-section'><h3>BLE Control</h3>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/pair?token=" + currentToken;
      htmlResponse += "'>Start Pairing</a> - Starts BLE advertising for pairing</div>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/stoppair?token=" + currentToken;
      htmlResponse += "'>Stop Pairing</a> - Stops BLE advertising</div>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/unpair?token=" + currentToken;
      htmlResponse += "'>Unpair</a> - Removes all stored BLE pairings</div>";
      htmlResponse += "</div>";
      
      // Key control endpoints
      htmlResponse += "<div class='api-section'><h3>Remote Control</h3>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/releaseall?token=" + currentToken;
      htmlResponse += "'>Release All Keys</a> - Release all currently pressed keys</div>";
      htmlResponse += "<div class='endpoint'><strong>GET /api/key</strong> - Press and release a key";
      htmlResponse += "<div class='params'>Parameters: key (required), delay in ms (optional, default=100), token (required)</div></div>";
      htmlResponse += "<div class='endpoint'><strong>GET /api/rawmediakey</strong> - Send raw hex media key values";
      htmlResponse += "<div class='params'>Parameters: value (hex, required), delay in ms (optional, default=100), token (required)</div></div>";
      htmlResponse += "</div>";
      
      // Key examples section with working links
      htmlResponse += "<div class='api-section'><h3>Key Examples</h3>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/key?key=up&token=" + currentToken;
      htmlResponse += "'>Up Arrow</a></div>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/key?key=down&token=" + currentToken;
      htmlResponse += "'>Down Arrow</a></div>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/key?key=enter&token=" + currentToken;
      htmlResponse += "'>Enter</a></div>";
      htmlResponse += "<div class='endpoint'><a href='";
      htmlResponse += baseUrl + "/api/key?key=playpause&token=" + currentToken;
      htmlResponse += "'>Play/Pause</a></div>";
      htmlResponse += "</div>";
      
      // Available keys section
      htmlResponse += "<h2>Available Keys</h2>";
      htmlResponse += "<div class='api-section'><h3>Keys Reference</h3>";
      htmlResponse += "<div class='endpoint'><strong>Media Keys:</strong> ";
      
      // Add media keys safely
      for (int i = 0; i < NUM_MEDIA_KEY_MAPPINGS; i++) {
        htmlResponse += mediaKeyMappings[i].name;
        if (i < NUM_MEDIA_KEY_MAPPINGS - 1) {
          htmlResponse += ", ";
        }
      }
      htmlResponse += "</div>";
      htmlResponse += "<div class='endpoint'><strong>Numbers:</strong> 0, 1, 2, 3, 4, 5, 6, 7, 8, 9</div>";
      htmlResponse += "<div class='endpoint'><strong>Tip:</strong> Use the /api/rawmediakey endpoint for hexadecimal values (format: 0xXX or 0xXXXX)</div>";
      htmlResponse += "</div>";
      
      htmlResponse += "</div></body></html>";
      
      request->send(200, "text/html", htmlResponse);
    });
    
    // Root endpoint (unprotected) - Configuration information
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String htmlResponse = generateHtmlHeader();
      
      // Device information section (without sensitive data)
      String html = "<div class='info'><strong>Device name:</strong> " BLE_DEVICE_NAME "</div>";
      html += "<div class='info'><strong>IP address:</strong> " + wifiManager.localIp().toString() + "</div>";
      html += "<div class='info'><strong>WiFi:</strong> " + wifiManager.ssid() + "</div>";
      htmlResponse += html;
      
      // Configuration instructions
      htmlResponse += "<h2>Getting Started</h2>";
      String configContent = HTML_ENDPOINT_START;
      configContent += "<strong>Step 1:</strong> Connect to the device via serial console to get your authentication token";
      configContent += HTML_ENDPOINT_END;
      configContent += HTML_ENDPOINT_START;
      configContent += "<strong>Step 2:</strong> Use the CLI command <code>config</code> to view your current token";
      configContent += HTML_ENDPOINT_END;
      configContent += HTML_ENDPOINT_START;
      configContent += "<strong>Step 3:</strong> Access the full documentation at <code>/doc?token=YOUR_TOKEN</code>";
      configContent += HTML_ENDPOINT_END;
      configContent += HTML_ENDPOINT_START;
      configContent += "<strong>Step 4:</strong> Use the token as a parameter in all API calls";
      configContent += HTML_ENDPOINT_END;
      htmlResponse += generateApiSection("Configuration", configContent);
      
      // Token information
      htmlResponse += "<h2>Authentication</h2>";
      String authContent = HTML_ENDPOINT_START;
      authContent += "<strong>Token Required:</strong> All API endpoints require authentication";
      authContent += HTML_ENDPOINT_END;
      authContent += HTML_ENDPOINT_START;
      authContent += "<strong>Token Location:</strong> Available via serial console using the <code>config</code> command";
      authContent += HTML_ENDPOINT_END;
      authContent += HTML_ENDPOINT_START;
      authContent += "<strong>Generate New Token:</strong> Use the <code>createtoken</code> command in the CLI";
      authContent += HTML_ENDPOINT_END;
      authContent += HTML_ENDPOINT_START;
      authContent += "Example API call: <code>/api/key?key=up&token=YOUR_TOKEN_HERE</code>";
      authContent += HTML_ENDPOINT_END;
      htmlResponse += generateApiSection("Authentication", authContent);
      
      // Available endpoints overview (without links)
      htmlResponse += "<h2>Available API Endpoints</h2>";
      String endpointsContent = HTML_ENDPOINT_START;
      endpointsContent += "<strong>BLE Control:</strong> /api/pair, /api/stoppair, /api/unpair";
      endpointsContent += HTML_ENDPOINT_END;
      endpointsContent += HTML_ENDPOINT_START;
      endpointsContent += "<strong>Remote Control:</strong> /api/key, /api/press, /api/release, /api/releaseall, /api/rawmediakey";
      endpointsContent += HTML_ENDPOINT_END;
      endpointsContent += HTML_ENDPOINT_START;
      endpointsContent += "<strong>System:</strong> /api/system/diagnostics, /api/system/battery, /api/system/reboot";
      endpointsContent += HTML_ENDPOINT_END;
      endpointsContent += HTML_ENDPOINT_START;
      endpointsContent += "<strong>Configuration:</strong> /api/ble/config (GET/POST), /api/ble/reset (POST)";
      endpointsContent += HTML_ENDPOINT_END;
      endpointsContent += HTML_ENDPOINT_START;
      endpointsContent += "<strong>Documentation:</strong> /doc (requires token for full interactive documentation)";
      endpointsContent += HTML_ENDPOINT_END;
      htmlResponse += generateApiSection("API Overview", endpointsContent);
      
      // Serial console instructions
      htmlResponse += "<h2>Serial Console Access</h2>";
      String serialContent = HTML_ENDPOINT_START;
      serialContent += "<strong>Baud Rate:</strong> 115200,8,N,1";
      serialContent += HTML_ENDPOINT_END;
      serialContent += HTML_ENDPOINT_START;
      serialContent += "<strong>Commands:</strong> Type <code>help</code> to see all available CLI commands";
      serialContent += HTML_ENDPOINT_END;
      serialContent += HTML_ENDPOINT_START;
      serialContent += "<strong>Configuration:</strong> Use <code>config</code> to view current settings including your token";
      serialContent += HTML_ENDPOINT_END;
      serialContent += HTML_ENDPOINT_START;
      serialContent += "<strong>WiFi Setup:</strong> Use <code>setssid</code>, <code>setpwd</code>, and <code>connect</code> commands";
      serialContent += HTML_ENDPOINT_END;
      htmlResponse += generateApiSection("Serial Console", serialContent);
      
      htmlResponse += HTML_BODY_END;
      
      request->send(200, "text/html", htmlResponse);
    });

    // 404 handler for not found endpoints
    server.onNotFound([](AsyncWebServerRequest *request){
      request->send(404, "text/plain", "404: Not Found");
    });
    
    // CORS headers for API access
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Start the web server
    server.begin();
    Serial.println("Web server started on port 80");
    Serial.println("http://" + wifiManager.localIp().toString() + "/");
  }
  
  // Generate diagnostic information as JSON
  String getDeviceInfo() {
    // Calculate uptime in seconds
    unsigned long uptime = (millis() - startTime) / 1000;
    unsigned long uptimeDays = uptime / 86400;
    unsigned long uptimeHours = (uptime % 86400) / 3600;
    unsigned long uptimeMinutes = (uptime % 3600) / 60;
    unsigned long uptimeSeconds = uptime % 60;
    
    String uptimeStr = String(uptimeDays) + "d " + String(uptimeHours) + "h " +
                       String(uptimeMinutes) + "m " + String(uptimeSeconds) + "s";
    
    // Create JSON object for diagnostic information
    StaticJsonDocument<1024> doc;
    
    // System information
    JsonObject system = doc.createNestedObject("system");
    system["deviceName"] = BLE_DEVICE_NAME;
    system["manufacturer"] = BLE_MANUFACTURER_NAME;
    system["chipModel"] = ESP.getChipModel();
    system["chipRevision"] = ESP.getChipRevision();
    system["chipCores"] = ESP.getChipCores();
    system["sdkVersion"] = ESP.getSdkVersion();
    system["freeHeap"] = ESP.getFreeHeap();
    system["uptime"] = uptimeStr;
    system["uptimeSeconds"] = uptime;
    system["bootCount"] = bootCount;
    
    // WiFi information
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["connected"] = (wifiManager.isConnected());;
    wifi["ssid"] = wifiManager.ssid();
    wifi["ipAddress"] = wifiManager.localIp().toString();
    wifi["macAddress"] = wifiManager.macAddress();
    wifi["rssi"] = wifiManager.RSSI();
    wifi["channel"] = wifiManager.channel();
    
    // BLE information
    JsonObject ble = doc.createNestedObject("ble");
    ble["deviceName"] = BLE_DEVICE_NAME;
    ble["manufacturer"] = BLE_MANUFACTURER_NAME;
    ble["initialized"] = true; // If the code reaches here, BLE is initialized
    ble["connected"] = deviceConnected;
    ble["serviceUUID"] = SERVICE_UUID;
    ble["library"] = "ESP32 BLE Arduino";
    
    // Serialize the JSON object
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    
    return jsonOutput;
  }