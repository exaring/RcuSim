#include "globals.h"
#include "webserver.h"

AsyncWebServer server(80);

void sendJsonResponse(AsyncWebServerRequest *request, int httpCode, String message) {
  String jsonResponse = "{\"status\":" + String(httpCode) + ",\"message\":\"" + message + "\"}";
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void setupWebServer() {
    Serial.println("Initializing web server and REST API...");
    
    // Endpoint for diagnostic information
    server.on("/api/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request){
      String jsonResponse = getDeviceInfo();
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    });
    
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
        sendJsonResponse(request, 400, "Pairing information removed successfully");
      } else {
        sendJsonResponse(request, 400, "Failed to remove pairing information");
      }
    });

    // API endpoint for press command
    server.on("/api/press", HTTP_GET, [](AsyncWebServerRequest *request) {
      String keyParam = "";
      String response = "{\"status\":\"error\",\"message\":\"Missing key parameter\"}";
      int responseCode = 400;
      
      // Check if key parameter exists
      if (request->hasParam("key")) {
        keyParam = request->getParam("key")->value();
        // TODO: Implement key press logic
        response = "{\"status\":\"success\",\"message\":\"Key pressed: " + keyParam + "\"}";
        responseCode = 200;
      }
      
      AsyncWebServerResponse *serverResponse = request->beginResponse(responseCode, "application/json", response);
      serverResponse->addHeader("Access-Control-Allow-Origin", "*");
      request->send(serverResponse);
    });

    // API endpoint for release command
    server.on("/api/release", HTTP_GET, [](AsyncWebServerRequest *request) {
      String keyParam = "";
      String response = "{\"status\":\"error\",\"message\":\"Missing key parameter\"}";
      int responseCode = 400;
      
      // Check if key parameter exists
      if (request->hasParam("key")) {
        keyParam = request->getParam("key")->value();
        // TODO: Implement key release logic
        response = "{\"status\":\"success\",\"message\":\"Key released: " + keyParam + "\"}";
        responseCode = 200;
      }
      
      AsyncWebServerResponse *serverResponse = request->beginResponse(responseCode, "application/json", response);
      serverResponse->addHeader("Access-Control-Allow-Origin", "*");
      request->send(serverResponse);
    });

    // API endpoint for key command (press + optional delay + release)
    server.on("/api/key", HTTP_GET, [](AsyncWebServerRequest *request) {
      String keyParam = "";
      int delayParam = 100; // Default delay value
      String response = "{\"status\":\"error\",\"message\":\"Missing key parameter\"}";
      int responseCode = 400;
      
      // Check if key parameter exists
      if (request->hasParam("key")) {
        keyParam = request->getParam("key")->value();
        
        // Check if delay parameter exists
        if (request->hasParam("delay")) {
          delayParam = request->getParam("delay")->value().toInt();
        }
        
        if(bleRemoteControl.key(keyParam, delayParam))
        {
          response = "{\"status\":\"success\",\"message\":\"Key pressed and released: " + keyParam + 
                     "\", \"delay\":" + String(delayParam) + "}";
          responseCode = 200;
        } else {
          response = "{\"status\":\"error\",\"message\":\"Failed to process key: " + keyParam + "\"}";
        }
      } else {
        response = "{\"status\":\"error\",\"message\":\"Missing key parameter\"}";
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
        levelParam = request->getParam("key")->value().toInt();
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
      String htmlResponse = "<html><head><title>ESP32 BLE Remote Control</title>";
      htmlResponse += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
      htmlResponse += "<style>body{font-family:Arial,sans-serif;margin:20px;line-height:1.6}";
      htmlResponse += "h1{color:#0066cc}";
      htmlResponse += ".container{max-width:800px;margin:0 auto;padding:20px;border:1px solid #ddd;border-radius:5px}";
      htmlResponse += ".info{margin-bottom:10px}";
      htmlResponse += "a{color:#0066cc;text-decoration:none}";
      htmlResponse += "a:hover{text-decoration:underline}</style></head>";
      htmlResponse += "<body><div class='container'>";
      htmlResponse += "<h1>ESP32 BLE Remote Control</h1>";
      htmlResponse += "<div class='info'><strong>Device name:</strong>" DEVICE_NAME "</div>";
      htmlResponse += "<div class='info'><strong>IP address:</strong> " + wifiManager.localIp().toString() + "</div>";
      htmlResponse += "<div class='info'><strong>MAC address:</strong> " + wifiManager.macAddress() + "</div>";
      htmlResponse += "<div class='info'><strong>WiFi:</strong> " + wifiManager.ssid() + "</div>";
      htmlResponse += "<div class='info'><strong>RSSI:</strong> " + String(wifiManager.RSSI()) + " dBm</div>";
      htmlResponse += "<div class='info'><a href='/api/diagnostics'>Diagnostic information (JSON)</a></div>";
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
    Serial.println("REST API endpoints available:");
    Serial.println("  GET /api/diagnostics - Diagnostic information");
    Serial.println("  GET /api/pair - Start pairing");
    Serial.println("  GET /api/stoppair - Stop pairing");
    Serial.println("  GET /api/unpair - Unpair device");
    Serial.println("  GET /api/press - Press key");
    Serial.println("  GET /api/release - Release key");
    Serial.println("  GET /api/key - Press and release key with optional delay");
    Serial.println("  GET /api/System/reboot - Reboots the ESP32");
    Serial.println("  GET /api/system/diagnostics - Diagnostic information in JSON format");
    Serial.println("  GET /api/system/battery - Set battery level (0-100)");
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