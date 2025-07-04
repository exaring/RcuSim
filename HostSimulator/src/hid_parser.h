#ifndef HID_PARSER_H
#define HID_PARSER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <stack>
#include <set>
#include "hid_constants.h"
#include "device_types.h"
#include "ble_host_config.h"

// HID Item structure
struct HIDItem {
    uint8_t tag;
    uint8_t type;
    uint8_t size;
    uint32_t data;
    
    HIDItem() : tag(0), type(0), size(0), data(0) {}
};

// HID Usage information
struct HIDUsage {
    uint16_t usagePage;
    uint16_t usage;
    String description;
    
    HIDUsage() : usagePage(0), usage(0) {}
    HIDUsage(uint16_t page, uint16_t use, const String& desc) : 
        usagePage(page), usage(use), description(desc) {}
};

// HID Collection information
struct HIDCollection {
    uint8_t type;
    uint16_t usagePage;
    uint16_t usage;
    String description;
    std::vector<HIDReportInfo> reports;
    
    HIDCollection() : type(0), usagePage(0), usage(0) {}
};

// Parser state
struct ParserState {
    uint16_t usagePage;
    uint16_t usageMinimum;
    uint16_t usageMaximum;
    std::vector<uint16_t> usages;
    int32_t logicalMinimum;
    int32_t logicalMaximum;
    uint8_t reportSize;
    uint8_t reportCount;
    uint8_t reportId;
    
    ParserState() : usagePage(0), usageMinimum(0), usageMaximum(0),
                   logicalMinimum(0), logicalMaximum(0), reportSize(0), 
                   reportCount(0), reportId(0) {}
};

class HIDReportParser {
private:
    std::vector<HIDItem> items;
    std::map<uint8_t, HIDReportInfo> reportMap;
    std::vector<HIDCollection> collections;
    std::stack<ParserState> stateStack;
    ParserState currentState;
    
    // Core parsing methods
    bool parseDescriptor(const std::vector<uint8_t>& descriptor);
    HIDItem parseItem(const uint8_t* data, size_t& offset, size_t maxLength);
    void processItem(const HIDItem& item);
    void processMainItem(const HIDItem& item);
    void processGlobalItem(const HIDItem& item);
    void processLocalItem(const HIDItem& item);
    
    // Utility methods
    String getUsageDescription(uint16_t usagePage, uint16_t usage);
    String getCollectionDescription(uint8_t collectionType);
    String formatHexDump(const std::vector<uint8_t>& data, size_t bytesPerLine = 16);
    String getInputOutputDescription(uint32_t flags);
    
    // Tabular analysis methods
    String getTabularDescription(uint8_t type, uint8_t tag, uint32_t value);
    String getCollectionTypeName(uint8_t type);
    String getUsagePageDescription(uint16_t page);
    String getUsageDescriptionByContext(uint16_t usage);
    void printDescriptorSummary(const std::vector<uint8_t>& descriptor);
    
public:
    HIDReportParser();
    ~HIDReportParser();
    
    bool parse(const std::vector<uint8_t>& descriptor);
    void clear();
    
    std::map<uint8_t, HIDReportInfo> getReportMap() const { return reportMap; }
    std::vector<HIDCollection> getCollections() const { return collections; }
    std::vector<HIDItem> getItems() const { return items; }
    
    String decodeReport(uint8_t reportId, const std::vector<uint8_t>& data);
    String decodeKeyboardReport(const std::vector<uint8_t>& data);
    String decodeConsumerReport(const std::vector<uint8_t>& data);
    String decodeMouseReport(const std::vector<uint8_t>& data);
    
    // Print methods
    void printParseResults();
    void printReportMap();
    void printCollections();
    void printHexDump(const std::vector<uint8_t>& data);
    void printDetailedAnalysis();
    void printItemDetails();
    void printUsageAnalysis();
    void printReportStructure();
    void printTabularHexAnalysis(const std::vector<uint8_t>& descriptor);
    
    static String formatReportData(const ReportData& report, bool includeHex = true);
};

#endif // HID_PARSER_H