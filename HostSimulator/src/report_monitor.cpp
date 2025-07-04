#include "report_monitor.h"
#include "hid_constants.h"
#include "ble_host_config.h"
#include <SPIFFS.h>

ReportMonitor::ReportMonitor() : 
    isMonitoring(false),
    outputFormat(OutputFormat::BOTH),
    pParser(nullptr),
    maxBufferSize(REPORT_MONITOR_BUFFER_SIZE),
    totalReportsReceived(0),
    monitoringStartTime(0),
    loggingEnabled(false),
    maxLogFileSize(LOG_FILE_MAX_SIZE) {
    
    // Initialize statistics
    for (int i = 0; i < 4; i++) {
        reportsByType[i] = 0;
    }
}

ReportMonitor::~ReportMonitor() {
    stopMonitoring();
    stopLogging();
}

bool ReportMonitor::initialize(HIDReportParser* parser) {
    if (!parser) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Invalid parser provided to ReportMonitor");
        return false;
    }
    
    pParser = parser;
    
    // Initialize SPIFFS for logging
    if (!SPIFFS.begin(true)) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "SPIFFS initialization failed - logging disabled");
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Report Monitor initialized");
    return true;
}

void ReportMonitor::startMonitoring() {
    if (isMonitoring) {
        return;
    }
    
    isMonitoring = true;
    monitoringStartTime = millis();
    resetStatistics();
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Report monitoring started");
    
    if (statusCallback) {
        statusCallback(STATUS_MONITORING);
    }
}

void ReportMonitor::stopMonitoring() {
    if (!isMonitoring) {
        return;
    }
    
    isMonitoring = false;
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Report monitoring stopped");
    
    if (statusCallback) {
        statusCallback(STATUS_MONITORING_STOPPED);
    }
}

void ReportMonitor::onReportReceived(const ReportData& report) {
    if (!isMonitoring) {
        return;
    }
    
    // Create a copy with decoded data
    ReportData processedReport = report;
    
    // Decode the report if parser is available
    if (pParser) {
        processedReport.decodedData = pParser->decodeReport(report.reportId, report.data);
    }
    
    // Add to buffer
    addToBuffer(processedReport);
    
    // Update statistics
    updateStatistics(processedReport);
    
    // Write to log if enabled
    if (loggingEnabled) {
        String logEntry = HIDReportParser::formatReportData(processedReport, true);
        writeToLog(logEntry);
    }
    
    // Call callback if set
    if (reportCallback) {
        reportCallback(processedReport);
    }
}

void ReportMonitor::addToBuffer(const ReportData& report) {
    reportBuffer.push_back(report);
    
    // Maintain buffer size
    while (reportBuffer.size() > maxBufferSize) {
        reportBuffer.erase(reportBuffer.begin());
    }
}

void ReportMonitor::updateStatistics(const ReportData& report) {
    totalReportsReceived++;
    
    // Update by type (assuming report ID maps to type somehow)
    // This is a simplified mapping - could be improved with HID descriptor analysis
    if (report.reportId <= 3) {
        reportsByType[report.reportId]++;
    } else {
        reportsByType[0]++; // Unknown
    }
}

void ReportMonitor::setMaxBufferSize(size_t size) {
    maxBufferSize = size;
    
    // Trim current buffer if necessary
    while (reportBuffer.size() > maxBufferSize) {
        reportBuffer.erase(reportBuffer.begin());
    }
}

void ReportMonitor::clearBuffer() {
    reportBuffer.clear();
    ESP_LOGI(BLE_HOST_LOG_TAG, "Report buffer cleared");
}

bool ReportMonitor::startLogging(const String& filename) {
    if (loggingEnabled) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "Logging already enabled");
        return false;
    }
    
    // Generate filename if not provided
    if (filename.isEmpty()) {
        logFileName = "/ble_reports_" + String(millis()) + ".log";
    } else {
        logFileName = filename.startsWith("/") ? filename : ("/" + filename);
    }
    
    // Test write access
    File testFile = SPIFFS.open(logFileName, "w");
    if (!testFile) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to create log file: %s", logFileName.c_str());
        return false;
    }
    
    // Write header
    testFile.println("# BLE HID Report Log");
    testFile.println("# Format: [timestamp] decoded_data [hex_data]");
    testFile.println("# Started: " + String(millis()));
    testFile.close();
    
    loggingEnabled = true;
    ESP_LOGI(BLE_HOST_LOG_TAG, "Logging started: %s", logFileName.c_str());
    
    return true;
}

bool ReportMonitor::stopLogging() {
    if (!loggingEnabled) {
        return false;
    }
    
    loggingEnabled = false;
    
    // Write footer
    File logFile = SPIFFS.open(logFileName, "a");
    if (logFile) {
        logFile.println("# Stopped: " + String(millis()));
        logFile.close();
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Logging stopped");
    return true;
}

bool ReportMonitor::writeToLog(const String& logEntry) {
    if (!loggingEnabled) {
        return false;
    }
    
    File logFile = SPIFFS.open(logFileName, "a");
    if (!logFile) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to open log file for writing");
        return false;
    }
    
    logFile.println(logEntry);
    logFile.close();
    
    // Check file size and rotate if necessary
    File checkFile = SPIFFS.open(logFileName, "r");
    if (checkFile && checkFile.size() > maxLogFileSize) {
        checkFile.close();
        // Rotate log file
        String backupName = logFileName + ".old";
        SPIFFS.remove(backupName);
        SPIFFS.rename(logFileName, backupName);
        ESP_LOGI(BLE_HOST_LOG_TAG, "Log file rotated");
    } else if (checkFile) {
        checkFile.close();
    }
    
    return true;
}

uint32_t ReportMonitor::getReportsByType(uint8_t type) const {
    if (type < 4) {
        return reportsByType[type];
    }
    return 0;
}

uint32_t ReportMonitor::getMonitoringDuration() const {
    if (!isMonitoring && monitoringStartTime == 0) {
        return 0;
    }
    return (millis() - monitoringStartTime) / 1000;
}

void ReportMonitor::resetStatistics() {
    totalReportsReceived = 0;
    for (int i = 0; i < 4; i++) {
        reportsByType[i] = 0;
    }
    monitoringStartTime = millis();
}

void ReportMonitor::printStatistics() const {
    Serial.println("\n=== Report Monitor Statistics ===");
    Serial.printf("Total Reports: %d\n", totalReportsReceived);
    Serial.printf("Monitoring Duration: %d seconds\n", getMonitoringDuration());
    Serial.printf("Buffer Size: %d / %d\n", reportBuffer.size(), maxBufferSize);
    
    if (totalReportsReceived > 0) {
        float reportsPerSecond = (float)totalReportsReceived / getMonitoringDuration();
        Serial.printf("Reports/Second: %.2f\n", reportsPerSecond);
    }
    
    Serial.println("\nReports by Type:");
    for (int i = 0; i < 4; i++) {
        if (reportsByType[i] > 0) {
            String typeName = (i == 0) ? "Unknown" :
                             (i == 1) ? "Input" :
                             (i == 2) ? "Output" : "Feature";
            Serial.printf("  %s: %d\n", typeName.c_str(), reportsByType[i]);
        }
    }
    
    if (loggingEnabled) {
        Serial.printf("\nLogging: %s\n", logFileName.c_str());
        File logFile = SPIFFS.open(logFileName, "r");
        if (logFile) {
            Serial.printf("Log Size: %d bytes\n", logFile.size());
            logFile.close();
        }
    }
    
    Serial.println("=================================");
}

void ReportMonitor::printReport(const ReportData& report) const {
    String output = "";
    
    switch (outputFormat) {
        case OutputFormat::HEX_ONLY:
            output = formatTimestamp(report.timestamp) + " [";
            for (size_t i = 0; i < report.data.size(); i++) {
                if (i > 0) output += " ";
                output += String(report.data[i], HEX);
            }
            output += "]";
            break;
            
        case OutputFormat::DECODED_ONLY:
            output = formatTimestamp(report.timestamp) + " " + report.decodedData;
            break;
            
        case OutputFormat::BOTH:
        default:
            output = HIDReportParser::formatReportData(report, true);
            break;
    }
    
    Serial.println(output);
}

void ReportMonitor::printBuffer() const {
    if (reportBuffer.empty()) {
        Serial.println("Report buffer is empty");
        return;
    }
    
    Serial.printf("\n=== Report Buffer (%d reports) ===\n", reportBuffer.size());
    for (const auto& report : reportBuffer) {
        printReport(report);
    }
    Serial.println("==================================");
}

void ReportMonitor::printRecentReports(size_t count) const {
    if (reportBuffer.empty()) {
        Serial.println("No reports in buffer");
        return;
    }
    
    size_t startIndex = (reportBuffer.size() > count) ? reportBuffer.size() - count : 0;
    
    Serial.printf("\n=== Recent Reports (last %d) ===\n", reportBuffer.size() - startIndex);
    for (size_t i = startIndex; i < reportBuffer.size(); i++) {
        printReport(reportBuffer[i]);
    }
    Serial.println("===============================");
}

String ReportMonitor::formatTimestamp(uint32_t timestamp) const {
    uint32_t seconds = timestamp / 1000;
    uint32_t milliseconds = timestamp % 1000;
    uint32_t minutes = seconds / 60;
    seconds = seconds % 60;
    uint32_t hours = minutes / 60;
    minutes = minutes % 60;
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "[%02d:%02d:%02d.%03d]", 
             hours, minutes, seconds, milliseconds);
    return String(buffer);
}

void ReportMonitor::setReportCallback(std::function<void(const ReportData&)> callback) {
    reportCallback = callback;
}

void ReportMonitor::setStatusCallback(std::function<void(const String&)> callback) {
    statusCallback = callback;
}

bool ReportMonitor::exportToCSV(const String& filename) const {
    if (reportBuffer.empty()) {
        ESP_LOGW(BLE_HOST_LOG_TAG, "No data to export");
        return false;
    }
    
    String csvFileName = filename.isEmpty() ? "/reports_export.csv" : filename;
    if (!csvFileName.startsWith("/")) {
        csvFileName = "/" + csvFileName;
    }
    
    File csvFile = SPIFFS.open(csvFileName, "w");
    if (!csvFile) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Failed to create CSV file");
        return false;
    }
    
    // Write header
    csvFile.println("Timestamp,ReportID,DataLength,HexData,DecodedData");
    
    // Write data
    for (const auto& report : reportBuffer) {
        csvFile.print(report.timestamp);
        csvFile.print(",");
        csvFile.print(report.reportId);
        csvFile.print(",");
        csvFile.print(report.data.size());
        csvFile.print(",\"");
        
        for (size_t i = 0; i < report.data.size(); i++) {
            if (i > 0) csvFile.print(" ");
            csvFile.print(String(report.data[i], HEX));
        }
        
        csvFile.print("\",\"");
        csvFile.print(report.decodedData);
        csvFile.println("\"");
    }
    
    csvFile.close();
    ESP_LOGI(BLE_HOST_LOG_TAG, "Data exported to CSV: %s", csvFileName.c_str());
    return true;
}

bool ReportMonitor::exportToJSON(const String& filename) const {
    // JSON export would require ArduinoJson library
    // For now, return false to indicate not implemented
    ESP_LOGW(BLE_HOST_LOG_TAG, "JSON export not yet implemented");
    return false;
}