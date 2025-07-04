#include "cli_handler.h"
#include "ble_host_config.h"

CLIHandler::CLIHandler() : 
    pScanner(nullptr), 
    pClient(nullptr), 
    pParser(nullptr), 
    pMonitor(nullptr),
    echoEnabled(true) {
}

CLIHandler::~CLIHandler() {
    // Cleanup if needed
}

bool CLIHandler::initialize(BLEScanner* scanner, BLEHostClient* client, 
                          HIDReportParser* parser, ReportMonitor* monitor) {
    pScanner = scanner;
    pClient = client;
    pParser = parser;
    pMonitor = monitor;
    
    if (!pScanner || !pClient || !pParser || !pMonitor) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Invalid component pointers provided to CLI");
        return false;
    }
    
    // Register commands
    commands.clear();
    
    commands.emplace_back("help", "Show available commands", "help [command]",
        [this](const std::vector<String>& args) { handleHelp(args); });
    
    commands.emplace_back("scan", "Scan for BLE devices", "scan [duration] [--filter-name=name] [--filter-rssi=value]",
        [this](const std::vector<String>& args) { handleScan(args); });
    
    commands.emplace_back("list", "List found devices", "list",
        [this](const std::vector<String>& args) { handleList(args); });
    
    commands.emplace_back("pair", "Connect to a device", "pair <index|address>",
        [this](const std::vector<String>& args) { handlePair(args); });
    
    commands.emplace_back("disconnect", "Disconnect from device", "disconnect",
        [this](const std::vector<String>& args) { handleDisconnect(args); }, true);
    
    commands.emplace_back("explain", "Show detailed device info", "explain <index|address>",
        [this](const std::vector<String>& args) { handleExplain(args); });
    
    commands.emplace_back("services", "Show device services", "services",
        [this](const std::vector<String>& args) { handleServices(args); }, true);
    
    commands.emplace_back("monitor", "Start report monitoring", "monitor [--format=hex|decoded|both]",
        [this](const std::vector<String>& args) { handleMonitor(args); }, true);
    
    commands.emplace_back("stop-monitor", "Stop report monitoring", "stop-monitor",
        [this](const std::vector<String>& args) { handleStopMonitor(args); });
    
    commands.emplace_back("status", "Show system status", "status",
        [this](const std::vector<String>& args) { handleStatus(args); });
    
    commands.emplace_back("clear", "Clear screen or buffer", "clear [buffer|screen]",
        [this](const std::vector<String>& args) { handleClear(args); });
    
    commands.emplace_back("filter", "Set scan filter", "filter [--name=name] [--rssi=value] [--clear]",
        [this](const std::vector<String>& args) { handleFilter(args); });
    
    commands.emplace_back("config", "Show/set configuration", "config [get|set] [parameter] [value]",
        [this](const std::vector<String>& args) { handleConfig(args); });
    
    commands.emplace_back("log", "Control logging", "log [start|stop|clear] [filename]",
        [this](const std::vector<String>& args) { handleLog(args); });
    
    commands.emplace_back("stats", "Show statistics", "stats [reset]",
        [this](const std::vector<String>& args) { handleStats(args); });
    
    commands.emplace_back("export", "Export data", "export <csv|json> [filename]",
        [this](const std::vector<String>& args) { handleExport(args); });
    
    commands.emplace_back("reboot", "Restart the device", "reboot",
        [this](const std::vector<String>& args) { handleReboot(args); });
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "CLI initialized with %d commands", commands.size());
    return true;
}

void CLIHandler::processInput() {
    while (Serial.available()) {
        char c = Serial.read();
        
        if (echoEnabled) {
            Serial.print(c);
        }
        
        if (c == '\n' || c == '\r') {
            if (!inputBuffer.isEmpty()) {
                executeCommand(inputBuffer);
                inputBuffer = "";
            }
            printPrompt();
        } else if (c == '\b' || c == 127) { // Backspace
            if (!inputBuffer.isEmpty()) {
                inputBuffer.remove(inputBuffer.length() - 1);
                if (echoEnabled) {
                    Serial.print(" \b");
                }
            }
        } else if (c >= 32 && c <= 126) { // Printable characters
            inputBuffer += c;
        }
    }
}

void CLIHandler::executeCommand(const String& commandLine) {
    std::vector<String> args = parseArguments(commandLine);
    
    if (args.empty()) {
        return;
    }
    
    String commandName = args[0];
    commandName.toLowerCase();
    args.erase(args.begin()); // Remove command name from args
    
    // Find and execute command
    for (const auto& cmd : commands) {
        if (cmd.name.equalsIgnoreCase(commandName)) {
            if (cmd.requiresConnection && !validateConnectionRequired()) {
                return;
            }
            
            try {
                cmd.handler(args);
            } catch (const std::exception& e) {
                printError("Command execution failed: " + String(e.what()));
            }
            return;
        }
    }
    
    printError("Unknown command: " + commandName + ". Type 'help' for available commands.");
}

std::vector<String> CLIHandler::parseArguments(const String& input) {
    std::vector<String> args;
    String current = "";
    bool inQuotes = false;
    
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!current.isEmpty()) {
                args.push_back(current);
                current = "";
            }
        } else {
            current += c;
        }
    }
    
    if (!current.isEmpty()) {
        args.push_back(current);
    }
    
    return args;
}

void CLIHandler::handleHelp(const std::vector<String>& args) {
    if (args.empty()) {
        printCommandList();
    } else {
        String commandName = args[0];
        for (const auto& cmd : commands) {
            if (cmd.name.equalsIgnoreCase(commandName)) {
                Serial.println("\nCommand: " + cmd.name);
                Serial.println("Description: " + cmd.description);
                Serial.println("Usage: " + cmd.usage);
                if (cmd.requiresConnection) {
                    Serial.println("Note: Requires active connection");
                }
                return;
            }
        }
        printError("Command not found: " + commandName);
    }
}

void CLIHandler::handleScan(const std::vector<String>& args) {
    uint32_t duration = BLE_SCAN_TIME_DEFAULT;
    ScanFilter filter;
    
    // Parse arguments
    for (const auto& arg : args) {
        if (arg.startsWith("--filter-name=")) {
            filter.nameFilter = arg.substring(14);
            filter.filterByName = true;
        } else if (arg.startsWith("--filter-rssi=")) {
            filter.minRSSI = arg.substring(14).toInt();
            filter.filterByRSSI = true;
        } else if (arg.toInt() > 0) {
            duration = arg.toInt();
        }
    }
    
    if (pScanner->isCurrentlyScanning()) {
        printWarning("Scan already in progress");
        return;
    }
    
    if (filter.filterByName || filter.filterByRSSI) {
        pScanner->setFilter(filter);
        printInfo("Applied scan filter");
    }
    
    if (!pScanner->startScan(duration)) {
        printError("Failed to start scan");
        return;
    }
    
    printSuccess("Scan started for " + String(duration) + " seconds");
}

void CLIHandler::handleList(const std::vector<String>& args) {
    pScanner->printScanResults();
}

void CLIHandler::handlePair(const std::vector<String>& args) {
    if (args.empty()) {
        printError("Usage: pair <index|address>");
        return;
    }
    
    if (pClient->isConnected()) {
        printWarning("Already connected to a device. Disconnect first.");
        return;
    }
    
    ScannedDevice device = findDevice(args[0]);
    if (!device.isValid()) {
        printError("Device not found: " + args[0]);
        return;
    }
    
    printInfo("Connecting to: " + device.name + " (" + device.address + ")");
    
    if (pClient->connectToDevice(device.address)) {
        printSuccess("Successfully connected and discovered services");
        
        // Subscribe to reports automatically
        if (pClient->subscribeToReports()) {
            printInfo("Subscribed to HID reports");
        }
    } else {
        printError("Failed to connect to device");
    }
}

void CLIHandler::handleDisconnect(const std::vector<String>& args) {
    if (!pClient->isConnected()) {
        printWarning("No device connected");
        return;
    }
    
    String address = pClient->getConnectedDeviceAddress();
    
    if (pClient->disconnect()) {
        printSuccess("Disconnected from " + address);
    } else {
        printError("Failed to disconnect properly");
    }
}

void CLIHandler::handleExplain(const std::vector<String>& args) {
    if (args.empty()) {
        printError("Usage: explain <index|address>");
        return;
    }
    
    ScannedDevice device = findDevice(args[0]);
    if (!device.isValid()) {
        printError("Device not found: " + args[0]);
        return;
    }
    
    // Show basic device info first
    Serial.println("\n=== Basic Device Information ===");
    pScanner->printDevice(device);
    
    // If connected to this device, show detailed info
    if (pClient->isConnected() && 
        pClient->getConnectedDeviceAddress().equalsIgnoreCase(device.address)) {
        
        Serial.println("============================================================");
        Serial.println("DETAILED DEVICE ANALYSIS");
        Serial.println("============================================================");
        
        pClient->printDeviceInfo();
        pClient->printHIDInformation();
        
        // Advanced HID descriptor analysis
        HIDInformation hidInfo = pClient->getHIDInformation();
        if (!hidInfo.reportDescriptor.empty()) {
            Serial.println("============================================================");
            Serial.println("HID REPORT DESCRIPTOR ANALYSIS");
            Serial.println("============================================================");
            
            // Parse and analyze the descriptor
            if (pParser->parse(hidInfo.reportDescriptor)) {
                // Show tabular hex analysis (compact and clear)
                pParser->printTabularHexAnalysis(hidInfo.reportDescriptor);
                
                // Show detailed analysis
                pParser->printDetailedAnalysis();
                
            } else {
                printWarning("Failed to parse HID descriptor");
                Serial.println("\n=== Raw Descriptor Data ===");
                pParser->printHexDump(hidInfo.reportDescriptor);
            }
            
            // Show compatibility analysis
            printCompatibilityAnalysis(hidInfo);
        } else {
            printWarning("No HID descriptor available - device may not be properly connected");
        }
        
        // Show services analysis
        pClient->printServices();
        
        Serial.println("============================================================");
        Serial.println("ANALYSIS COMPLETE");
        Serial.println("============================================================");
        
    } else {
        Serial.println("\n=== Connection Required for Detailed Analysis ===");
        printInfo("To see detailed HID analysis, connect to the device first:");
        printInfo("Use command: pair " + args[0]);
        Serial.println("========================================================");
        
        // Show what we can determine from scan data
        Serial.println("\n=== Available Information from Scan ===");
        Serial.printf("Device Name: %s\n", device.name.c_str());
        Serial.printf("MAC Address: %s\n", device.address.c_str());
        Serial.printf("Signal Strength: %d dBm\n", device.rssi);
        Serial.printf("Manufacturer: %s\n", device.manufacturer.isEmpty() ? "Unknown" : device.manufacturer.c_str());
        
        // Debug output to identify the issue
        Serial.printf("DEBUG - Device Type Enum: %d\n", (int)device.deviceType);
        
        String deviceTypeStr = "Unknown";
        switch (device.deviceType) {
            case DeviceType::KEYBOARD: deviceTypeStr = "Keyboard"; break;
            case DeviceType::MOUSE: deviceTypeStr = "Mouse"; break;
            case DeviceType::REMOTE_CONTROL: deviceTypeStr = "Remote Control"; break;
            case DeviceType::GAME_CONTROLLER: deviceTypeStr = "Game Controller"; break;
            case DeviceType::MULTIMEDIA_REMOTE: deviceTypeStr = "Multimedia Remote"; break;
            default: deviceTypeStr = "Unknown"; break;
        }
        Serial.printf("Device Type: %s\n", deviceTypeStr.c_str());
        
        Serial.printf("HID Service Detected: %s\n", device.hasHIDService ? "Yes" : "No");
        Serial.printf("Device Info Service: %s\n", device.hasDeviceInfoService ? "Yes" : "No");
        Serial.printf("Battery Service: %s\n", device.hasBatteryService ? "Yes" : "No");
        
        if (!device.serviceUUIDs.empty()) {
            Serial.println("\nAdvertised Services:");
            for (const auto& uuid : device.serviceUUIDs) {
                Serial.printf("  - %s\n", uuid.c_str());
            }
        }
        
        Serial.println("=====================================================");
    }
}

void CLIHandler::handleServices(const std::vector<String>& args) {
    pClient->printServices();
}

void CLIHandler::handleMonitor(const std::vector<String>& args) {
    if (pMonitor->isCurrentlyMonitoring()) {
        printWarning("Report monitoring already active");
        return;
    }
    
    OutputFormat format = OutputFormat::BOTH;
    
    // Parse format argument
    for (const auto& arg : args) {
        if (arg.startsWith("--format=")) {
            String formatStr = arg.substring(9);
            formatStr.toLowerCase();
            if (formatStr == "hex") {
                format = OutputFormat::HEX_ONLY;
            } else if (formatStr == "decoded") {
                format = OutputFormat::DECODED_ONLY;
            } else if (formatStr == "both") {
                format = OutputFormat::BOTH;
            } else {
                printWarning("Invalid format: " + formatStr + ". Using 'both'");
            }
        }
    }
    
    pMonitor->setOutputFormat(format);
    pMonitor->startMonitoring();
    
    String formatName = (format == OutputFormat::HEX_ONLY) ? "hex" :
                       (format == OutputFormat::DECODED_ONLY) ? "decoded" : "both";
    printSuccess("Report monitoring started (format: " + formatName + ")");
    printInfo("Press any key and Enter to see prompt, or use 'stop-monitor' to stop");
}

void CLIHandler::handleStopMonitor(const std::vector<String>& args) {
    if (!pMonitor->isCurrentlyMonitoring()) {
        printWarning("Report monitoring not active");
        return;
    }
    
    pMonitor->stopMonitoring();
    printSuccess("Report monitoring stopped");
    pMonitor->printStatistics();
}

void CLIHandler::handleStatus(const std::vector<String>& args) {
    Serial.println("\n=== System Status ===");
    Serial.printf("Device: %s\n", BLE_HOST_DEVICE_NAME);
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    
    Serial.println("\nBLE Scanner:");
    Serial.printf("  Status: %s\n", pScanner->isCurrentlyScanning() ? "Scanning" : "Idle");
    Serial.printf("  Devices Found: %d\n", pScanner->getDeviceCount());
    
    Serial.println("\nBLE Client:");
    Serial.printf("  Status: %s\n", pClient->isConnected() ? "Connected" : "Disconnected");
    if (pClient->isConnected()) {
        Serial.printf("  Device: %s\n", pClient->getConnectedDeviceAddress().c_str());
    }
    
    Serial.println("\nReport Monitor:");
    Serial.printf("  Status: %s\n", pMonitor->isCurrentlyMonitoring() ? "Active" : "Inactive");
    Serial.printf("  Reports Received: %d\n", pMonitor->getTotalReportsReceived());
    Serial.printf("  Buffer Size: %d\n", pMonitor->getBufferSize());
    if (pMonitor->isLogging()) {
        Serial.printf("  Logging: %s\n", pMonitor->getLogFileName().c_str());
    }
    
    Serial.println("====================");
}

void CLIHandler::handleClear(const std::vector<String>& args) {
    if (args.empty() || args[0].equalsIgnoreCase("screen")) {
        // Clear screen
        Serial.print("\033[2J\033[H");
        printInfo("Screen cleared");
    } else if (args[0].equalsIgnoreCase("buffer")) {
        pMonitor->clearBuffer();
        printInfo("Report buffer cleared");
    } else {
        printError("Usage: clear [screen|buffer]");
    }
}

void CLIHandler::handleFilter(const std::vector<String>& args) {
    ScanFilter filter = pScanner->getFilter();
    bool changed = false;
    
    for (const auto& arg : args) {
        if (arg.startsWith("--name=")) {
            filter.nameFilter = arg.substring(7);
            filter.filterByName = true;
            changed = true;
        } else if (arg.startsWith("--rssi=")) {
            filter.minRSSI = arg.substring(7).toInt();
            filter.filterByRSSI = true;
            changed = true;
        } else if (arg.equalsIgnoreCase("--clear")) {
            filter = ScanFilter();
            changed = true;
        }
    }
    
    if (changed) {
        pScanner->setFilter(filter);
        printSuccess("Scan filter updated");
    }
    
    // Show current filter
    Serial.println("\nCurrent Scan Filter:");
    Serial.printf("  Name Filter: %s\n", filter.filterByName ? filter.nameFilter.c_str() : "None");
    Serial.printf("  RSSI Filter: %s\n", filter.filterByRSSI ? (String(filter.minRSSI) + " dBm").c_str() : "None");
}

void CLIHandler::handleConfig(const std::vector<String>& args) {
    printInfo("Configuration commands not yet implemented");
}

void CLIHandler::handleLog(const std::vector<String>& args) {
    if (args.empty()) {
        Serial.println("\nLogging Status:");
        Serial.printf("  Active: %s\n", pMonitor->isLogging() ? "Yes" : "No");
        if (pMonitor->isLogging()) {
            Serial.printf("  File: %s\n", pMonitor->getLogFileName().c_str());
        }
        return;
    }
    
    String action = args[0];
    action.toLowerCase();
    
    if (action == "start") {
        String filename = (args.size() > 1) ? args[1] : "";
        if (pMonitor->startLogging(filename)) {
            printSuccess("Logging started: " + pMonitor->getLogFileName());
        } else {
            printError("Failed to start logging");
        }
    } else if (action == "stop") {
        if (pMonitor->stopLogging()) {
            printSuccess("Logging stopped");
        } else {
            printError("Failed to stop logging");
        }
    } else {
        printError("Usage: log [start|stop] [filename]");
    }
}

void CLIHandler::handleStats(const std::vector<String>& args) {
    if (!args.empty() && args[0].equalsIgnoreCase("reset")) {
        pMonitor->resetStatistics();
        printSuccess("Statistics reset");
        return;
    }
    
    pMonitor->printStatistics();
}

void CLIHandler::handleExport(const std::vector<String>& args) {
    printInfo("Export commands not yet implemented");
}

void CLIHandler::handleReboot(const std::vector<String>& args) {
    printInfo("Rebooting device in 3 seconds...");
    delay(3000);
    ESP.restart();
}

ScannedDevice CLIHandler::findDevice(const String& identifier) {
    // Try to parse as index first
    int index = identifier.toInt();
    if (index > 0 && index <= (int)pScanner->getDeviceCount()) {
        return pScanner->getDevice(index - 1); // Convert to 0-based
    }
    
    // Try as MAC address
    return pScanner->getDeviceByAddress(identifier);
}

bool CLIHandler::validateConnectionRequired() {
    if (!pClient->isConnected()) {
        printError("This command requires an active connection. Use 'pair' first.");
        return false;
    }
    return true;
}

void CLIHandler::printError(const String& message) {
    Serial.println("ERROR: " + message);
}

void CLIHandler::printSuccess(const String& message) {
    Serial.println("SUCCESS: " + message);
}

void CLIHandler::printInfo(const String& message) {
    Serial.println("INFO: " + message);
}

void CLIHandler::printWarning(const String& message) {
    Serial.println("WARNING: " + message);
}

void CLIHandler::printPrompt() {
    Serial.print(CLI_PROMPT);
}

void CLIHandler::printWelcome() {
    Serial.println("\nWelcome to ESP32 BLE Host Simulator!");
    Serial.println("Type 'help' to see available commands.");
    Serial.println("Type 'scan' to start looking for BLE devices.");
    printPrompt();
}

void CLIHandler::printCommandList() {
    Serial.println("\nAvailable Commands:");
    Serial.println("==================");
    
    for (const auto& cmd : commands) {
        Serial.printf("%-15s - %s\n", cmd.name.c_str(), cmd.description.c_str());
    }
    
    Serial.println("\nUse 'help <command>' for detailed usage information.");
}

// Event handlers
void CLIHandler::onDeviceFound(const ScannedDevice& device) {
    if (pScanner->isCurrentlyScanning()) {
        Serial.printf("\rFound: %s (%s) RSSI: %d dBm", 
                     device.name.c_str(), device.address.c_str(), device.rssi);
        Serial.flush();
    }
}

void CLIHandler::onConnectionStateChanged(ConnectionState state) {
    switch (state) {
        case ConnectionState::CONNECTING:
            printInfo("Connecting...");
            break;
        case ConnectionState::CONNECTED:
            printSuccess("Device connected");
            break;
        case ConnectionState::DISCONNECTED:
            printInfo("Device disconnected");
            break;
        case ConnectionState::ERROR:
            printError("Connection error");
            break;
    }
}

void CLIHandler::onReportReceived(const ReportData& report) {
    if (pMonitor->isCurrentlyMonitoring()) {
        pMonitor->printReport(report);
    }
}

void CLIHandler::onScanComplete() {
    Serial.println("\n");
    printSuccess("Scan completed");
    printInfo("Use 'list' to see all found devices");
    printPrompt();
}

// Analysis functions
void CLIHandler::printCompatibilityAnalysis(const HIDInformation& hidInfo) {
    Serial.println("\n=== Compatibility Analysis ===");
    
    // HID version analysis
    Serial.printf("HID Version: %d.%02d", (hidInfo.bcdHID >> 8), (hidInfo.bcdHID & 0xFF));
    if (hidInfo.bcdHID >= 0x0111) {
        Serial.println(" (Modern - Good compatibility)");
    } else {
        Serial.println(" (Legacy - May have compatibility issues)");
    }
    
    // Country code analysis
    if (hidInfo.bCountryCode == 0) {
        Serial.println("Country Code: 0 (Not localized)");
    } else {
        Serial.printf("Country Code: %d (Localized keyboard)\n", hidInfo.bCountryCode);
    }
    
    // Report analysis
    bool hasKeyboard = false, hasConsumer = false, hasMouse = false;
    for (const auto& pair : hidInfo.reportMap) {
        const HIDReportInfo& info = pair.second;
        if (info.description.indexOf("Keyboard") >= 0) hasKeyboard = true;
        if (info.description.indexOf("Consumer") >= 0) hasConsumer = true;
        if (info.description.indexOf("Mouse") >= 0) hasMouse = true;
    }
    
    Serial.println("\nDevice Type Classification:");
    if (hasKeyboard && hasConsumer) {
        Serial.println("  • Multimedia Keyboard (Keyboard + Media Keys)");
    } else if (hasKeyboard) {
        Serial.println("  • Standard Keyboard");
    } else if (hasMouse) {
        Serial.println("  • Mouse/Pointing Device");
    } else {
        Serial.println("  • Custom HID Device");
    }
    
    Serial.println("\nRecommended Usage:");
    if (hasConsumer) {
        Serial.println("  • Use monitor command to see media key presses");
        Serial.println("  • Compatible with media player applications");
    }
    if (hasKeyboard) {
        Serial.println("  • Use monitor command to see keystrokes");
        Serial.println("  • Compatible with text input applications");
    }
    
    Serial.println("===============================");
}

void CLIHandler::printDescriptorBreakdown(const std::vector<uint8_t>& descriptor) {
    Serial.println("Offset | Hex  | Binary   | Type/Tag | Size | Data     | Description");
    Serial.println("-------|------|----------|----------|------|----------|------------------");
    
    size_t offset = 0;
    while (offset < descriptor.size()) {
        uint8_t prefix = descriptor[offset];
        
        // Parse prefix byte
        uint8_t size = prefix & 0x03;
        uint8_t type = (prefix >> 2) & 0x03;
        uint8_t tag = (prefix >> 4) & 0x0F;
        
        // Format binary representation
        String binary = "";
        for (int i = 7; i >= 0; i--) {
            binary += (prefix & (1 << i)) ? "1" : "0";
        }
        
        // Get data
        uint32_t data = 0;
        for (int i = 0; i < size && (offset + 1 + i) < descriptor.size(); i++) {
            data |= ((uint32_t)descriptor[offset + 1 + i]) << (i * 8);
        }
        
        // Generate description
        String typeStr = (type == 0) ? "Main" : (type == 1) ? "Glob" : (type == 2) ? "Locl" : "Rsrv";
        String description = getItemDescription(type, tag, data);
        
        Serial.printf("%6d | 0x%02X | %s | %s/%02d   | %4d | 0x%06X | %s\n",
                     offset, prefix, binary.c_str(), typeStr.c_str(), tag, 
                     size, data, description.c_str());
        
        offset += 1 + size;
    }
}

String CLIHandler::getItemDescription(uint8_t type, uint8_t tag, uint32_t data) {
    if (type == 0) { // Main items
        switch (tag) {
            case 8: return "Input(" + formatInputOutputFlags(data) + ")";
            case 9: return "Output(" + formatInputOutputFlags(data) + ")";
            case 10: return "Collection(" + String(data) + ")";
            case 11: return "Feature(" + formatInputOutputFlags(data) + ")";
            case 12: return "End Collection";
            default: return "Unknown Main";
        }
    } else if (type == 1) { // Global items
        switch (tag) {
            case 0: return "Usage Page(0x" + String(data, HEX) + ")";
            case 1: return "Logical Min(" + String((int32_t)data) + ")";
            case 2: return "Logical Max(" + String((int32_t)data) + ")";
            case 3: return "Physical Min(" + String((int32_t)data) + ")";
            case 4: return "Physical Max(" + String((int32_t)data) + ")";
            case 5: return "Unit Exponent(" + String(data) + ")";
            case 6: return "Unit(0x" + String(data, HEX) + ")";
            case 7: return "Report Size(" + String(data) + ")";
            case 8: return "Report ID(" + String(data) + ")";
            case 9: return "Report Count(" + String(data) + ")";
            case 10: return "Push";
            case 11: return "Pop";
            default: return "Global(" + String(tag) + ")";
        }
    } else if (type == 2) { // Local items
        switch (tag) {
            case 0: return "Usage(0x" + String(data, HEX) + ")";
            case 1: return "Usage Min(0x" + String(data, HEX) + ")";
            case 2: return "Usage Max(0x" + String(data, HEX) + ")";
            case 3: return "Designator Index(" + String(data) + ")";
            case 4: return "Designator Min(" + String(data) + ")";
            case 5: return "Designator Max(" + String(data) + ")";
            case 7: return "String Index(" + String(data) + ")";
            case 8: return "String Min(" + String(data) + ")";
            case 9: return "String Max(" + String(data) + ")";
            case 10: return "Delimiter";
            default: return "Local(" + String(tag) + ")";
        }
    }
    return "Unknown";
}

String CLIHandler::formatInputOutputFlags(uint32_t flags) {
    String result = "";
    if (flags & 0x01) result += "Const,";
    else result += "Data,";
    if (flags & 0x02) result += "Var,";
    else result += "Array,";
    if (flags & 0x04) result += "Rel,";
    else result += "Abs,";
    if (flags & 0x08) result += "Wrap,";
    if (flags & 0x10) result += "NonLin,";
    if (flags & 0x20) result += "NoPref,";
    if (flags & 0x40) result += "Null,";
    if (flags & 0x80) result += "Vol,";
    
    // Remove trailing comma
    if (result.endsWith(",")) {
        result.remove(result.length() - 1);
    }
    return result;
}