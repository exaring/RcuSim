#ifndef REPORT_MONITOR_H
#define REPORT_MONITOR_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "ble_host_config.h"
#include "device_types.h"
#include "hid_parser.h"

class ReportMonitor {
private:
    bool isMonitoring;
    OutputFormat outputFormat;
    HIDReportParser* pParser;
    
    std::vector<ReportData> reportBuffer;
    size_t maxBufferSize;
    
    // Statistics
    uint32_t totalReportsReceived;
    uint32_t reportsByType[4]; // Index 0=unknown, 1=input, 2=output, 3=feature
    uint32_t monitoringStartTime;
    
    // Logging
    bool loggingEnabled;
    String logFileName;
    size_t maxLogFileSize;
    
    // Callbacks
    std::function<void(const ReportData&)> reportCallback;
    std::function<void(const String&)> statusCallback;
    
    void addToBuffer(const ReportData& report);
    void updateStatistics(const ReportData& report);
    String formatTimestamp(uint32_t timestamp) const;
    bool writeToLog(const String& logEntry);
    
public:
    ReportMonitor();
    ~ReportMonitor();
    
    bool initialize(HIDReportParser* parser);
    
    void startMonitoring();
    void stopMonitoring();
    bool isCurrentlyMonitoring() const { return isMonitoring; }
    
    void onReportReceived(const ReportData& report);
    
    void setOutputFormat(OutputFormat format) { outputFormat = format; }
    OutputFormat getOutputFormat() const { return outputFormat; }
    
    void setMaxBufferSize(size_t size);
    size_t getBufferSize() const { return reportBuffer.size(); }
    std::vector<ReportData> getBufferContents() const { return reportBuffer; }
    void clearBuffer();
    
    bool startLogging(const String& filename = "");
    bool stopLogging();
    bool isLogging() const { return loggingEnabled; }
    String getLogFileName() const { return logFileName; }
    void setMaxLogFileSize(size_t size) { maxLogFileSize = size; }
    
    void setReportCallback(std::function<void(const ReportData&)> callback);
    void setStatusCallback(std::function<void(const String&)> callback);
    
    // Statistics
    uint32_t getTotalReportsReceived() const { return totalReportsReceived; }
    uint32_t getReportsByType(uint8_t type) const;
    uint32_t getMonitoringDuration() const;
    void resetStatistics();
    void printStatistics() const;
    
    // Report display
    void printReport(const ReportData& report) const;
    void printBuffer() const;
    void printRecentReports(size_t count = 10) const;
    
    // Export functions
    bool exportToCSV(const String& filename) const;
    bool exportToJSON(const String& filename) const;
};

#endif // REPORT_MONITOR_H