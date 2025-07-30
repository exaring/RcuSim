#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include <Arduino.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "wifimanager.h"


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// HTML/CSS Constants
#define HTML_DOCTYPE "<!DOCTYPE html>"
#define HTML_HEAD_START "<html><head><title>ESP32 BLE Remote Control</title>"
#define HTML_VIEWPORT "<meta name='viewport' content='width=device-width, initial-scale=1'>"
#define HTML_CSS_STYLES "<style>" \
  "body{font-family:Arial,sans-serif;margin:20px;line-height:1.6}" \
  "h1{color:#0066cc}" \
  "h2{color:#0066cc;margin-top:20px}" \
  ".container{max-width:800px;margin:0 auto;padding:20px;border:1px solid #ddd;border-radius:5px}" \
  ".info{margin-bottom:10px}" \
  ".api-section{margin-top:15px;padding:10px;background:#f7f7f7;border-radius:5px}" \
  ".endpoint{margin-bottom:8px}" \
  "a{color:#0066cc;text-decoration:none}" \
  "a:hover{text-decoration:underline}" \
  ".params{font-size:0.9em;color:#666;margin-left:20px}" \
  "</style>"
#define HTML_HEAD_END "</head>"
#define HTML_BODY_START "<body><div class='container'>"
#define HTML_TITLE "<h1>ESP32 BLE Remote Control</h1>"
#define HTML_BODY_END "</div></body></html>"

#define HTML_SECTION_START "<div class='api-section'>"
#define HTML_SECTION_END "</div>"
#define HTML_ENDPOINT_START "<div class='endpoint'>"
#define HTML_ENDPOINT_END "</div>"
#define HTML_PARAMS_START "<div class='params'>"
#define HTML_PARAMS_END "</div>"

// Key reference content - removed hardcoded lists, will be generated dynamically
#define HTML_NUMBER_KEYS "<div class='endpoint'><strong>Numbers:</strong> 0, 1, 2, 3, 4, 5, 6, 7, 8, 9</div>"
#define HTML_KEY_TIPS "<div class='endpoint'><strong>Tip:</strong> Use the /api/rawmediakey endpoint for hexadecimal values (format: 0xXX or 0xXXXX)</div>"

// HTML generation helpers
String generateHtmlHeader();
String generateDeviceInfoSection();
String generateApiSection(String title, String content);
String generateKeysSection();
String generateMediaKeysFromMapping(); // New function to generate media keys dynamically

// Webserver and REST API variables

void setupWebServer();
String getDeviceInfo();

String generateRandomToken();
void saveAuthToken(const String& token);
void sendJsonResponse(AsyncWebServerRequest *request, int httpCode, String message);

extern unsigned long startTime;
extern unsigned long bootCount;
extern bool deviceConnected;
extern AsyncWebServer server;
extern WiFiManager wifiManager;
extern String authToken;
#endif // WEB_SERVER_H