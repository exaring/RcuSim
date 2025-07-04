#ifndef CLI_HANDLER_H
#define CLI_HANDLER_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "ble_host_config.h"
#include "ble_scanner.h"
#include "ble_client.h"
#include "hid_parser.h"
#include "report_monitor.h"

// Command structure
struct Command {
    String name;
    String description;
    String usage;
    std::function<void(const std::vector<String>&)> handler;
    bool requiresConnection;
    
    Command(const String& n, const String& desc, const String& use, 
            std::function<void(const std::vector<String>&)> h, bool reqConn = false) :
        name(n), description(desc), usage(use), handler(h), requiresConnection(reqConn) {}
};

class CLIHandler {
private:
    // Component pointers
    BLEScanner* pScanner;
    BLEHostClient* pClient;
    HIDReportParser* pParser;
    ReportMonitor* pMonitor;
    
    // CLI state
    std::vector<Command> commands;
    String inputBuffer;
    bool echoEnabled;
    
    // Command handlers
    void handleHelp(const std::vector<String>& args);
    void handleScan(const std::vector<String>& args);
    void handleList(const std::vector<String>& args);
    void handlePair(const std::vector<String>& args);
    void handleDisconnect(const std::vector<String>& args);
    void handleExplain(const std::vector<String>& args);
    void handleServices(const std::vector<String>& args);
    void handleMonitor(const std::vector<String>& args);
    void handleStopMonitor(const std::vector<String>& args);
    void handleStatus(const std::vector<String>& args);
    void handleClear(const std::vector<String>& args);
    void handleFilter(const std::vector<String>& args);
    void handleConfig(const std::vector<String>& args);
    void handleLog(const std::vector<String>& args);
    void handleStats(const std::vector<String>& args);
    void handleExport(const std::vector<String>& args);
    void handleReboot(const std::vector<String>& args);
    
    // Utility functions
    std::vector<String> parseArguments(const String& input);
    ScannedDevice findDevice(const String& identifier);
    bool validateConnectionRequired();
    
    // Output functions
    void printError(const String& message);
    void printSuccess(const String& message);
    void printInfo(const String& message);
    void printWarning(const String& message);
    void printPrompt();
    
    // Analysis functions
    void printCompatibilityAnalysis(const HIDInformation& hidInfo);
    void printDescriptorBreakdown(const std::vector<uint8_t>& descriptor);
    String getItemDescription(uint8_t type, uint8_t tag, uint32_t data);
    String formatInputOutputFlags(uint32_t flags);
    
public:
    CLIHandler();
    ~CLIHandler();
    
    // Initialization
    bool initialize(BLEScanner* scanner, BLEHostClient* client, 
                   HIDReportParser* parser, ReportMonitor* monitor);
    
    // Core functions
    void processInput();
    void executeCommand(const String& commandLine);
    
    // Configuration
    void setEchoEnabled(bool enabled) { echoEnabled = enabled; }
    bool isEchoEnabled() const { return echoEnabled; }
    
    // Display functions
    void printWelcome();
    void printCommandList();
    
    // Event handlers
    void onDeviceFound(const ScannedDevice& device);
    void onConnectionStateChanged(ConnectionState state);
    void onReportReceived(const ReportData& report);
    void onScanComplete();
};

#endif // CLI_HANDLER_H