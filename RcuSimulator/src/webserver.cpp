#include "globals.h"
#include "webserver.h"

AsyncWebServer server(80);

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
  String html = "<html><head><title>ESP32 BLE Remote Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;line-height:1.6}";
  html += "h1{color:#0066cc}";
  html += "h2{color:#0066cc;margin-top:20px}";
  html += ".container{max-width:800px;margin:0 auto;padding:20px;border:1px solid #ddd;border-radius:5px}";
  html += ".info{margin-bottom:10px}";
  html += ".api-section{margin-top:15px;padding:10px;background:#f7f7f7;border-radius:5px}";
  html += ".endpoint{margin-bottom:8px}";
  html += "a{color:#0066cc;text-decoration:none}";
  html += "a:hover{text-decoration:underline}";
  html += ".params{font-size:0.9em;color:#666;margin-left:20px}</style></head>";
  html += "<body><div class='container'>";
  html += "<h1>ESP32 BLE Remote Control</h1>";
  return html;
}

String generateDeviceInfoSection() {
  String html = "<div class='info'><strong>Device name:</strong> " DEVICE_NAME "</div>";
  html += "<div class='info'><strong>IP address:</strong> " + wifiManager.localIp().toString() + "</div>";
  html += "<div class='info'><strong>MAC address:</strong> " + wifiManager.macAddress() + "</div>";
  html += "<div class='info'><strong>WiFi:</strong> " + wifiManager.ssid() + "</div>";
  html += "<div class='info'><strong>RSSI:</strong> " + String(wifiManager.RSSI()) + " dBm</div>";
  return html;
}

String generateApiSection(String title, String content) {
  String html = "<div class='api-section'>";
  html += "<h3>" + title + "</h3>";
  html += content;
  html += "</div>";
  return html;
}

// Add this function to webserver.cpp to generate the key list section
String generateKeysSection() {
  String htmlContent = "";
  
  // Navigation keys
  String navigationKeys = "<div class='endpoint'><strong>Navigation:</strong> ";
  navigationKeys += "up, down, left, right, enter, back, home, menu";
  navigationKeys += "</div>";
  
  // Media keys
  String mediaKeys = "<div class='endpoint'><strong>Media:</strong> ";
  mediaKeys += "playpause, play, pause, stop, record, fastforward, rewind, ";
  mediaKeys += "nextsong, previoussong, volumeup, volumedown, mute";
  mediaKeys += "</div>";
  
  // Number keys
  String numberKeys = "<div class='endpoint'><strong>Numbers:</strong> ";
  numberKeys += "0, 1, 2, 3, 4, 5, 6, 7, 8, 9";
  numberKeys += "</div>";
  
  // Function keys
  String functionKeys = "<div class='endpoint'><strong>Function:</strong> ";
  functionKeys += "f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12";
  functionKeys += "</div>";
  
  // Special keys
  String specialKeys = "<div class='endpoint'><strong>Special:</strong> ";
  specialKeys += "escape, tab, capslock, shift, ctrl, alt, space, backspace, delete, ";
  specialKeys += "insert, pageup, pagedown, home, end, power, info, guide, ";
  specialKeys += "red, green, yellow, blue";
  specialKeys += "</div>";
  
  // Browser/app keys
  String browserKeys = "<div class='endpoint'><strong>Browser:</strong> ";
  browserKeys += "search, favorite, browser, mail, calculator";
  browserKeys += "</div>";
  
  // Tips about hex format
  String keyTips = "<div class='endpoint'><strong>Tip:</strong> ";
  keyTips += "You can also use hexadecimal key codes in format 0xXX";
  keyTips += "</div>";
  
  htmlContent += navigationKeys;
  htmlContent += mediaKeys;
  htmlContent += numberKeys;
  htmlContent += functionKeys;
  htmlContent += specialKeys;
  htmlContent += browserKeys;
  htmlContent += keyTips;
  
  return htmlContent;
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
    
    // API endpoint for pair command
    server.on("/api/pair", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (isBleAdvertising) {
        sendJsonResponse(request, 400, "BLE advertising is already running");
        return;
      }
      
      // Direct call to the extended BleRemoteControl method
      bleRemoteControl.startAdvertising();
      isBleAdvertising = true;
      Serial.println("BLE advertising started for pairing...");
      sendJsonResponse(request, 200, "BLE advertising started for pairing");
    });

    // API endpoint for stoppair command
    server.on("/api/stoppair", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!isBleAdvertising) {
        sendJsonResponse(request, 400, "BLE advertising is not active");
        return;
      }
      
      // Direct call to the extended BleRemoteControl method
      bleRemoteControl.stopAdvertising();
      isBleAdvertising = false;
      sendJsonResponse(request, 200, "BLE advertising stopped");
    });

    // API endpoint for unpair command
    server.on("/api/unpair", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (bleRemoteControl.removeBonding()) {
        sendJsonResponse(request, 200, "Pairing information removed successfully");
      } else {
        sendJsonResponse(request, 400, "Failed to remove pairing information");
      }
    });

    // API endpoint for press command
    server.on("/api/press", HTTP_GET, [](AsyncWebServerRequest *request) {
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
      if(!bleRemoteControl.isConnected()) {
        sendJsonResponse(request, 400, "Not connected to a host");
        return;
      }
      
      bleRemoteControl.releaseAll();
      sendJsonResponse(request, 200, "All keys released successfully");
    });

    // API endpoint for key command (press + optional delay + release)
    server.on("/api/key", HTTP_GET, [](AsyncWebServerRequest *request) {
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

    server.on("/api/system/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request) {
      String jsonResponse = getDeviceInfo();
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    });

    server.on("/api/system/battery", HTTP_GET, [](AsyncWebServerRequest *request) {
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
      String response = "{\"status\":\"success\",\"message\":\"Rebooting device...\"}";
      AsyncWebServerResponse *serverResponse = request->beginResponse(200, "application/json", response);
      serverResponse->addHeader("Access-Control-Allow-Origin", "*");
      request->send(serverResponse);
      delay(1000); // Delay to allow the response to be sent before rebooting
      ESP.restart(); // Reboot the device
    });    
    
    // Default endpoint (Root)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String htmlResponse = generateHtmlHeader();
      
      // Device information section
      htmlResponse += generateDeviceInfoSection();
      
      // API endpoints section
      htmlResponse += "<h2>API Endpoints</h2>";
      
      // BLE control endpoints
      String bleContent = "<div class='endpoint'><a href='/api/pair'>Start Pairing</a> - Starts BLE advertising for pairing</div>";
      bleContent += "<div class='endpoint'><a href='/api/stoppair'>Stop Pairing</a> - Stops BLE advertising</div>";
      bleContent += "<div class='endpoint'><a href='/api/unpair'>Unpair</a> - Removes all stored BLE pairings</div>";
      htmlResponse += generateApiSection("BLE Control", bleContent);
      
      // Key control endpoints
      String keyContent = "<div class='endpoint'><a href='/api/key?key=up'>Press Key</a> - Press and release a key";
      keyContent += "<div class='params'>Parameters: key (required), delay in ms (optional, default=100)</div></div>";
      keyContent += "<div class='endpoint'><a href='/api/press?key=up'>Press Only</a> - Press a key without releasing";
      keyContent += "<div class='params'>Parameters: key (required)</div></div>";
      keyContent += "<div class='endpoint'><a href='/api/release?key=up'>Release Only</a> - Release a previously pressed key";
      keyContent += "<div class='params'>Parameters: key (required)</div></div>";
      keyContent += "<div class='endpoint'><a href='/api/releaseall'>Release All Keys</a> - Release all currently pressed keys</div>";
      htmlResponse += generateApiSection("Remote Control", keyContent);
      
      // System endpoints
      String sysContent = "<div class='endpoint'><a href='/api/system/diagnostics'>System Diagnostics</a> - Detailed system information</div>";
      sysContent += "<div class='endpoint'><a href='/api/system/battery?level=100'>Set Battery Level</a> - Set the reported battery level";
      sysContent += "<div class='params'>Parameters: level (0-100)</div></div>";
      sysContent += "<div class='endpoint'><a href='/api/system/reboot'>Reboot Device</a> - Restart the ESP32</div>";
      htmlResponse += generateApiSection("System", sysContent);
      
      // Key examples section
      String examplesContent = "<div class='endpoint'><a href='/api/key?key=up'>Up Arrow</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=down'>Down Arrow</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=left'>Left Arrow</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=right'>Right Arrow</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=enter'>Enter</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=playpause'>Play/Pause</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=volumeup'>Volume Up</a></div>";
      examplesContent += "<div class='endpoint'><a href='/api/key?key=volumedown'>Volume Down</a></div>";
      htmlResponse += generateApiSection("Key Examples", examplesContent);
      
      // Add the new keys reference section
      htmlResponse += "<h2>Available Keys</h2>";
      htmlResponse += generateApiSection("Keys Reference", generateKeysSection());
      
      htmlResponse += "</div></body></html>";
      
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
    system["deviceName"] = DEVICE_NAME;
    system["manufacturer"] = DEVICE_MANUFACTURER;
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