#include "main.h"

// Global variables
BleRemoteControl bleRemoteControl;
bool isConfigMode = false;
GenericCLI cli;
WiFiManager wifiManager;
Preferences preferences;
DisplayManager displayManager; 
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool isBleAdvertising = false;
unsigned long startTime = 0;
unsigned long bootCount = 0;

void tryConnectWifi() {
  displayManager.setLinesAndRender("Connecting to WiFi...");
  if (wifiManager.setup()) {
    displayManager.setLinesAndRender("IP: " + wifiManager.localIp().toString());
  } else {
    displayManager.setLinesAndRender("WiFi setup failed", "Check configuration");
  }

  if (wifiManager.isConnected()) {
    displayManager.setLinesAndRender("Starting Web Server");
    setupWebServer();
    displayManager.setLinesAndRender("IP: " + wifiManager.localIp().toString(), "Webserver running");
  } else {
    displayManager.setLinesAndRender("WiFi not connected", "Check configuration");
  }
} 

// CLI command handlers
void handleSetSSID(const CLIArgs& args) {
  if (args.empty()) {
    Serial.println("ERROR: SSID required");
    return;
  }
  String ssid = args.getPositional(0);
  Serial.println("Set SSID to: " + ssid);
  wifiManager.setSSID(ssid);
}

void handleSetPassword(const CLIArgs& args) {
  if (args.empty()) {
    Serial.println("ERROR: Password required");
    return;
  }
  String password = args.getPositional(0);
  Serial.println("Set password to: " + password);
  wifiManager.setPassword(password);
}

void handleSetIP(const CLIArgs& args) {
  if (args.empty()) {
    Serial.println("ERROR: IP address required");
    return;
  }
  String ip = args.getPositional(0);
  if (!wifiManager.isValidIPAddress(ip)) {
    Serial.println("ERROR: Invalid IP address");
    return;
  }
  Serial.println("Set IP to: " + ip);
  wifiManager.setStaticIP(IPAddress().fromString(ip));
  wifiManager.useStaticIP(true);
  Serial.println("Static IP mode enabled");
}

void handleSetGateway(const CLIArgs& args) {
  if (args.empty()) {
    Serial.println("ERROR: Gateway address required");
    return;
  }
  String gateway = args.getPositional(0);
  if (!wifiManager.isValidIPAddress(gateway)) {
    Serial.println("ERROR: Invalid gateway address");
    return;
  }
  Serial.println("Set gateway to: " + gateway);
  wifiManager.setGateway(IPAddress().fromString(gateway));
}

void handleCreateToken(const CLIArgs& args) {
  String newToken = generateRandomToken();
  saveAuthToken(newToken);
  
  Serial.println("New webserver authentication token created:");
  Serial.println("Token: " + newToken);
  cli.printSuccess("Authentication token created and saved successfully");
}

void handleSaveConfig(const CLIArgs& args) {
  if (wifiManager.hasUnsavedChanges()) {
    wifiManager.saveConfig();
    cli.printSuccess("Configuration saved!");
  } else {
    cli.printInfo("No changes to save");
  }
}

void handleConnect(const CLIArgs& args) {
  cli.printInfo("Trying to establish WiFi connection...");
  tryConnectWifi();
}

void handleShowConfig(const CLIArgs& args) {
  wifiManager.printConfig();
  Serial.println("> Token: " + authToken);
}

void handleDiagnostics(const CLIArgs& args) {
  Serial.println("Diagnostic information:");
  Serial.println("  Boot counter: " + String(bootCount));
  Serial.println("  Uptime: " + String((millis() - startTime) / 1000) + " seconds");
  Serial.println("  WiFi status: " + String(wifiManager.isConnected() ? "Connected" : "Not connected"));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("  Current IP: " + wifiManager.localIp().toString());
  }
}

// Command definitions array
const CLICommandDef customCommands[] = {
  // WiFi Configuration Commands
  {"setssid",     "Set WiFi SSID",                "setssid <ssid>",      handleSetSSID,      "WiFi"},
  {"setpwd",      "Set WiFi password",            "setpwd <password>",   handleSetPassword,  "WiFi"},
  {"setip",       "Set static IP address",        "setip <ip>",          handleSetIP,        "WiFi"},
  {"setgateway",  "Set gateway address",          "setgateway <ip>",     handleSetGateway,   "WiFi"},
  {"createtoken", "Generate new webserver token", "createtoken",         handleCreateToken,  "WiFi"},
  {"save",        "Save WiFi configuration",      "save",                handleSaveConfig,   "WiFi"},
  {"connect",     "Connect to WiFi",              "connect",             handleConnect,      "WiFi"},
  {"config",      "Show WiFi configuration",      "config",              handleShowConfig,   "WiFi"},
  
  // System Commands
  {"diag",        "Show diagnostic information",  "diag",                handleDiagnostics,  "System"},
  
  // End marker
  {nullptr, nullptr, nullptr, nullptr, nullptr}
};

void setupCLI() {
  // Configure CLI
  CLIConfig config;
  config.prompt = "RCU";
  config.welcomeMessage = "ESP32 BLE Remote Control v1.0";
  config.colorsEnabled = true;
  config.echoEnabled = true;
  config.historySize = 50;
  config.caseSensitive = false;
  cli.setConfig(config);
  
  // Register custom commands from array
  for (const CLICommandDef* cmd = customCommands; cmd->name != nullptr; cmd++) {
    cli.registerCommand(cmd->name, cmd->description, cmd->usage, cmd->handler, cmd->category);
  }

  CLIStandardCommands::registerClearCommand(cli);
  CLIStandardCommands::registerRebootCommand(cli);
  CLIStandardCommands::registerColorsCommand(cli);
  CLIStandardCommands::registerHistoryCommand(cli);
  cli.begin();
}

// The main setup routine executed once at bootup
void setup() {
  Serial.begin(115200);
  esp_log_level_set("wifi", ESP_LOG_ERROR);
  startTime = millis();
  updateBootCounter();
  
  displayManager.begin(); 
  displayManager.setHeadline("ESP32 Remote Control");
  displayManager.setLinesAndRender("Initializing...");
  
  setupBLE();
  tryConnectWifi();
  delay(500);
  setupCLI();
}

// The main loop routine runs over and over again
void loop() {
  cli.update();
  wifiManager.loop();
  
  if (CLIStandardCommands::isExitRequested()) {
    Serial.println("Exit requested - entering minimal mode");
    CLIStandardCommands::resetExitFlag();
  }
  delay(10);
}

// BLE setup
void setupBLE() {
  // Initialize BLE functionality, but don't start yet
  bleRemoteControl.begin();
  
  // Wait for initialization
  delay(500);
}

void updateBootCounter() {
  preferences.begin("rcu-config", false);
  bootCount = preferences.getUInt("bootCount", 0);
  bootCount++;
  preferences.putUInt("bootCount", bootCount);
}
