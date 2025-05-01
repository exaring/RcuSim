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

// Webserver and REST API variables

void setupWebServer();
String getDeviceInfo();

extern unsigned long startTime;
extern unsigned long bootCount;
extern bool deviceConnected;
extern AsyncWebServer server;
extern WiFiManager wifiManager;
#endif // WEB_SERVER_H