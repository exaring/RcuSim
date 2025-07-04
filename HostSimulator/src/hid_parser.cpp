#include "hid_parser.h"

HIDReportParser::HIDReportParser() {
    clear();
}

HIDReportParser::~HIDReportParser() {
    clear();
}

bool HIDReportParser::parse(const std::vector<uint8_t>& descriptor) {
    clear();
    
    if (descriptor.empty()) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Empty HID report descriptor");
        return false;
    }
    
    ESP_LOGI(BLE_HOST_LOG_TAG, "Parsing HID report descriptor (%d bytes)", descriptor.size());
    
    return parseDescriptor(descriptor);
}

void HIDReportParser::clear() {
    items.clear();
    reportMap.clear();
    collections.clear();
    while (!stateStack.empty()) {
        stateStack.pop();
    }
    currentState = ParserState();
}

bool HIDReportParser::parseDescriptor(const std::vector<uint8_t>& descriptor) {
    size_t offset = 0;
    
    try {
        while (offset < descriptor.size()) {
            HIDItem item = parseItem(descriptor.data(), offset, descriptor.size());
            if (item.tag == 0 && item.type == 0 && item.size == 0) {
                break; // Parse error
            }
            
            items.push_back(item);
            processItem(item);
        }
        
        ESP_LOGI(BLE_HOST_LOG_TAG, "Successfully parsed %d HID items", items.size());
        return true;
        
    } catch (const std::exception& e) {
        ESP_LOGE(BLE_HOST_LOG_TAG, "Exception during HID parsing: %s", e.what());
        return false;
    }
}

HIDItem HIDReportParser::parseItem(const uint8_t* data, size_t& offset, size_t maxLength) {
    HIDItem item;
    
    if (offset >= maxLength) {
        return item; // End of data
    }
    
    uint8_t prefix = data[offset++];
    
    // Extract item properties from prefix
    item.size = prefix & 0x03;
    item.type = (prefix >> 2) & 0x03;
    item.tag = (prefix >> 4) & 0x0F;
    
    // Handle special case for long items
    if (item.size == 3) {
        if (offset + 2 >= maxLength) {
            return HIDItem(); // Error
        }
        item.size = data[offset++];
        item.tag = data[offset++];
        // Skip long item data
        offset += item.size;
        return item;
    }
    
    // Read item data
    item.data = 0;
    for (int i = 0; i < item.size && offset < maxLength; i++) {
        item.data |= ((uint32_t)data[offset++]) << (i * 8);
    }
    
    return item;
}

void HIDReportParser::processItem(const HIDItem& item) {
    switch (item.type) {
        case 0: // Main item
            processMainItem(item);
            break;
        case 1: // Global item
            processGlobalItem(item);
            break;
        case 2: // Local item
            processLocalItem(item);
            break;
        case 3: // Reserved
            break;
    }
}

void HIDReportParser::processMainItem(const HIDItem& item) {
    switch (item.tag) {
        case 8: // Input
        case 9: // Output  
        case 11: // Feature
        {
            HIDReportInfo reportInfo;
            reportInfo.reportId = currentState.reportId;
            reportInfo.reportType = (item.tag == 8) ? HID_REPORT_TYPE_INPUT :
                                   (item.tag == 9) ? HID_REPORT_TYPE_OUTPUT : 
                                                    HID_REPORT_TYPE_FEATURE;
            reportInfo.reportSize = currentState.reportSize * currentState.reportCount;
            
            // Create description based on usage
            if (!currentState.usages.empty()) {
                reportInfo.description = getUsageDescription(currentState.usagePage, currentState.usages[0]);
            } else if (currentState.usageMinimum != 0 || currentState.usageMaximum != 0) {
                reportInfo.description = getUsageDescription(currentState.usagePage, currentState.usageMinimum);
            } else {
                reportInfo.description = "Unknown";
            }
            
            reportMap[reportInfo.reportId] = reportInfo;
            
            // Clear local state
            currentState.usages.clear();
            currentState.usageMinimum = 0;
            currentState.usageMaximum = 0;
            break;
        }
        case 10: // Collection
            stateStack.push(currentState);
            break;
        case 12: // End Collection
            if (!stateStack.empty()) {
                currentState = stateStack.top();
                stateStack.pop();
            }
            break;
    }
}

void HIDReportParser::processGlobalItem(const HIDItem& item) {
    switch (item.tag) {
        case 0: // Usage Page
            currentState.usagePage = item.data;
            break;
        case 1: // Logical Minimum
            currentState.logicalMinimum = (int32_t)item.data;
            break;
        case 2: // Logical Maximum
            currentState.logicalMaximum = (int32_t)item.data;
            break;
        case 7: // Report Size
            currentState.reportSize = item.data;
            break;
        case 8: // Report ID
            currentState.reportId = item.data;
            break;
        case 9: // Report Count
            currentState.reportCount = item.data;
            break;
    }
}

void HIDReportParser::processLocalItem(const HIDItem& item) {
    switch (item.tag) {
        case 0: // Usage
            currentState.usages.push_back(item.data);
            break;
        case 1: // Usage Minimum
            currentState.usageMinimum = item.data;
            break;
        case 2: // Usage Maximum
            currentState.usageMaximum = item.data;
            break;
    }
}

String HIDReportParser::getUsageDescription(uint16_t usagePage, uint16_t usage) {
    switch (usagePage) {
        case HID_USAGE_PAGE_GENERIC_DESKTOP:
            switch (usage) {
                case 0x01: return "Pointer";
                case 0x02: return "Mouse";
                case 0x06: return "Keyboard";
                case 0x30: return "X";
                case 0x31: return "Y";
                case 0x32: return "Z";
                case 0x38: return "Wheel";
                default: return "Desktop_" + String(usage, HEX);
            }
        case HID_USAGE_PAGE_KEYBOARD:
            if (usage >= 0x04 && usage <= 0x1D) {
                return "Key_" + String((char)('A' + usage - 0x04));
            } else if (usage >= 0x1E && usage <= 0x27) {
                return "Key_" + String(usage - 0x1D);
            }
            return "Keyboard_" + String(usage, HEX);
        case HID_USAGE_PAGE_CONSUMER:
            switch (usage) {
                case 0x30: return "Power";
                case 0x40: return "Menu";
                case 0xB0: return "Play";
                case 0xB1: return "Pause";
                case 0xB2: return "Record";
                case 0xB3: return "Fast_Forward";
                case 0xB4: return "Rewind";
                case 0xB5: return "Next_Track";
                case 0xB6: return "Previous_Track";
                case 0xB7: return "Stop";
                case 0xCD: return "Play_Pause";
                case 0xE2: return "Mute";
                case 0xE9: return "Volume_Up";
                case 0xEA: return "Volume_Down";
                default: return "Consumer_" + String(usage, HEX);
            }
        default:
            return "Page" + String(usagePage, HEX) + "_" + String(usage, HEX);
    }
}

String HIDReportParser::getCollectionDescription(uint8_t collectionType) {
    switch (collectionType) {
        case 0x00: return "Physical";
        case 0x01: return "Application";
        case 0x02: return "Logical";
        default: return "Collection_" + String(collectionType, HEX);
    }
}

String HIDReportParser::decodeReport(uint8_t reportId, const std::vector<uint8_t>& data) {
    auto it = reportMap.find(reportId);
    if (it == reportMap.end()) {
        return "Unknown report ID: " + String(reportId);
    }
    
    const HIDReportInfo& info = it->second;
    String result = info.description + " (ID:" + String(reportId) + "): ";
    
    // Try specific decoders based on report content
    if (info.description.indexOf("Keyboard") >= 0) {
        result += decodeKeyboardReport(data);
    } else if (info.description.indexOf("Consumer") >= 0) {
        result += decodeConsumerReport(data);
    } else if (info.description.indexOf("Mouse") >= 0) {
        result += decodeMouseReport(data);
    } else {
        // Generic hex dump
        for (size_t i = 0; i < data.size(); i++) {
            if (i > 0) result += " ";
            result += String(data[i], HEX);
        }
    }
    
    return result;
}

String HIDReportParser::decodeKeyboardReport(const std::vector<uint8_t>& data) {
    if (data.size() < 3) {
        return "Invalid keyboard report size";
    }
    
    String result = "";
    uint8_t modifiers = data[0];
    
    // Decode modifiers
    if (modifiers != 0) {
        result += "Modifiers: ";
        for (const auto& pair : MODIFIER_KEYS) {
            if (modifiers & pair.first) {
                result += pair.second + " ";
            }
        }
        result += "| ";
    }
    
    // Decode key codes
    String keys = "";
    for (size_t i = 2; i < data.size() && i < 8; i++) {
        if (data[i] != 0) {
            auto it = KEYBOARD_KEYS.find(data[i]);
            if (it != KEYBOARD_KEYS.end()) {
                if (!keys.isEmpty()) keys += " ";
                keys += it->second;
            } else {
                if (!keys.isEmpty()) keys += " ";
                keys += "0x" + String(data[i], HEX);
            }
        }
    }
    
    if (!keys.isEmpty()) {
        result += "Keys: " + keys;
    } else if (modifiers == 0) {
        result += "No keys pressed";
    }
    
    return result;
}

String HIDReportParser::decodeConsumerReport(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return "Invalid consumer report size";
    }
    
    String result = "";
    
    // Decode 16-bit consumer codes
    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 < data.size()) {
            uint16_t consumerCode = (data[i + 1] << 8) | data[i];
            if (consumerCode != 0) {
                auto it = CONSUMER_KEYS.find(consumerCode);
                if (it != CONSUMER_KEYS.end()) {
                    if (!result.isEmpty()) result += " ";
                    result += it->second;
                } else {
                    if (!result.isEmpty()) result += " ";
                    result += "0x" + String(consumerCode, HEX);
                }
            }
        }
    }
    
    return result.isEmpty() ? "No consumer keys" : result;
}

String HIDReportParser::decodeMouseReport(const std::vector<uint8_t>& data) {
    if (data.size() < 3) {
        return "Invalid mouse report size";
    }
    
    String result = "";
    uint8_t buttons = data[0];
    int8_t deltaX = (int8_t)data[1];
    int8_t deltaY = (int8_t)data[2];
    
    if (buttons != 0) {
        result += "Buttons: ";
        if (buttons & 0x01) result += "L ";
        if (buttons & 0x02) result += "R ";
        if (buttons & 0x04) result += "M ";
        result += "| ";
    }
    
    if (deltaX != 0 || deltaY != 0) {
        result += "Delta: X=" + String(deltaX) + " Y=" + String(deltaY);
    }
    
    if (data.size() > 3 && data[3] != 0) {
        result += " Wheel: " + String((int8_t)data[3]);
    }
    
    return result.isEmpty() ? "No mouse activity" : result;
}

void HIDReportParser::printTabularHexAnalysis(const std::vector<uint8_t>& descriptor) {
    Serial.println("\n=== HID Report Descriptor - Tabular Analysis ===");
    Serial.printf("Total Length: %d bytes\n\n", descriptor.size());
    
    // Header
    Serial.println("Offset | Hex Data    | Type   | Tag | Size | Value    | Description");
    Serial.println("-------|-------------|--------|-----|------|----------|---------------------------");
    
    size_t offset = 0;
    
    while (offset < descriptor.size()) {
        // Parse prefix byte
        uint8_t prefix = descriptor[offset];
        uint8_t size = prefix & 0x03;
        uint8_t type = (prefix >> 2) & 0x03;
        uint8_t tag = (prefix >> 4) & 0x0F;
        
        // Handle special case for long items
        if (size == 3) {
            size = 4; // Special encoding for 4+ byte items
        }
        
        // Format hex data
        String hexData = String(prefix, HEX);
        hexData.toUpperCase();
        if (hexData.length() == 1) hexData = "0" + hexData;
        
        // Get data value
        uint32_t value = 0;
        for (int i = 0; i < size && (offset + 1 + i) < descriptor.size(); i++) {
            uint8_t dataByte = descriptor[offset + 1 + i];
            value |= ((uint32_t)dataByte) << (i * 8);
            
            String byteHex = String(dataByte, HEX);
            byteHex.toUpperCase();
            if (byteHex.length() == 1) byteHex = "0" + byteHex;
            hexData += " " + byteHex;
        }
        
        // Pad hex data to consistent width
        while (hexData.length() < 11) {
            hexData += " ";
        }
        
        // Get type string
        String typeStr = "";
        switch (type) {
            case 0: typeStr = "Main  "; break;
            case 1: typeStr = "Global"; break;
            case 2: typeStr = "Local "; break;
            case 3: typeStr = "Resrvd"; break;
        }
        
        // Get description
        String description = getTabularDescription(type, tag, value);
        
        // Format value
        String valueStr = "";
        if (size > 0) {
            valueStr = "0x" + String(value, HEX);
            valueStr.toUpperCase();
            while (valueStr.length() < 8) {
                valueStr = " " + valueStr;
            }
        } else {
            valueStr = "     -  ";
        }
        
        // Print row
        Serial.printf("%06X | %s | %s | %3d | %4d | %s | %s\n",
                     offset, hexData.c_str(), typeStr.c_str(), tag, size, 
                     valueStr.c_str(), description.c_str());
        
        offset += 1 + size;
    }
    
    Serial.println("================================================================");
    
    // Add summary
    printDescriptorSummary(descriptor);
}

String HIDReportParser::getTabularDescription(uint8_t type, uint8_t tag, uint32_t value) {
    if (type == 0) { // Main Items
        switch (tag) {
            case 0x08: return "Input(" + getInputOutputDescription(value) + ")";
            case 0x09: return "Output(" + getInputOutputDescription(value) + ")";
            case 0x0A: return "Collection(" + getCollectionTypeName(value) + ")";
            case 0x0B: return "Feature(" + getInputOutputDescription(value) + ")";
            case 0x0C: return "End Collection";
            default: return "Main Item " + String(tag, HEX);
        }
    } else if (type == 1) { // Global Items
        switch (tag) {
            case 0x00: return "Usage Page(" + getUsagePageDescription(value) + ")";
            case 0x01: return "Logical Minimum(" + String((int32_t)value) + ")";
            case 0x02: return "Logical Maximum(" + String((int32_t)value) + ")";
            case 0x03: return "Physical Minimum(" + String((int32_t)value) + ")";
            case 0x04: return "Physical Maximum(" + String((int32_t)value) + ")";
            case 0x05: return "Unit Exponent(" + String((int8_t)value) + ")";
            case 0x06: return "Unit(0x" + String(value, HEX) + ")";
            case 0x07: return "Report Size(" + String(value) + " bits)";
            case 0x08: return "Report ID(" + String(value) + ")";
            case 0x09: return "Report Count(" + String(value) + ")";
            case 0x0A: return "Push";
            case 0x0B: return "Pop";
            default: return "Global Item " + String(tag, HEX);
        }
    } else if (type == 2) { // Local Items
        switch (tag) {
            case 0x00: return "Usage(" + getUsageDescriptionByContext(value) + ")";
            case 0x01: return "Usage Minimum(0x" + String(value, HEX) + ")";
            case 0x02: return "Usage Maximum(0x" + String(value, HEX) + ")";
            case 0x03: return "Designator Index(" + String(value) + ")";
            case 0x04: return "Designator Minimum(" + String(value) + ")";
            case 0x05: return "Designator Maximum(" + String(value) + ")";
            case 0x07: return "String Index(" + String(value) + ")";
            case 0x08: return "String Minimum(" + String(value) + ")";
            case 0x09: return "String Maximum(" + String(value) + ")";
            case 0x0A: return "Delimiter";
            default: return "Local Item " + String(tag, HEX);
        }
    } else {
        return "Reserved Item";
    }
}

String HIDReportParser::getInputOutputDescription(uint32_t flags) {
    String result = "";
    
    // Most important flags for compact display
    if (flags & 0x01) result += "Const,";
    else result += "Data,";
    
    if (flags & 0x02) result += "Var,";
    else result += "Array,";
    
    if (flags & 0x04) result += "Rel";
    else result += "Abs";
    
    if (flags & 0x08) result += ",Wrap";
    if (flags & 0x40) result += ",Null";
    
    return result;
}

String HIDReportParser::getCollectionTypeName(uint8_t type) {
    switch (type) {
        case 0x00: return "Physical";
        case 0x01: return "Application";
        case 0x02: return "Logical";
        case 0x03: return "Report";
        case 0x04: return "Named Array";
        case 0x05: return "Usage Switch";
        case 0x06: return "Usage Modifier";
        default: return "0x" + String(type, HEX);
    }
}

String HIDReportParser::getUsagePageDescription(uint16_t page) {
    switch (page) {
        case 0x01: return "Generic Desktop";
        case 0x02: return "Simulation";
        case 0x03: return "VR Controls";
        case 0x04: return "Sport";
        case 0x05: return "Game";
        case 0x06: return "Generic Device";
        case 0x07: return "Keyboard/Keypad";
        case 0x08: return "LEDs";
        case 0x09: return "Button";
        case 0x0A: return "Ordinal";
        case 0x0B: return "Telephony";
        case 0x0C: return "Consumer";
        case 0x0D: return "Digitizer";
        case 0x0F: return "PID";
        case 0x10: return "Unicode";
        case 0x14: return "Alphanumeric";
        case 0x40: return "Medical";
        default:
            if (page >= 0xFF00) return "Vendor(0x" + String(page, HEX) + ")";
            return "0x" + String(page, HEX);
    }
}

String HIDReportParser::getUsageDescriptionByContext(uint16_t usage) {
    // Context-sensitive usage descriptions
    switch (usage) {
        case 0x01: return "Pointer";
        case 0x02: return "Mouse";
        case 0x06: return "Keyboard";
        case 0x30: return "X";
        case 0x31: return "Y";
        case 0x32: return "Z";
        case 0x38: return "Wheel";
        case 0xCD: return "Play/Pause";
        case 0xE2: return "Mute";
        case 0xE9: return "Volume Up";
        case 0xEA: return "Volume Down";
        case 0xB5: return "Next Track";
        case 0xB6: return "Previous Track";
        default: return "0x" + String(usage, HEX);
    }
}

void HIDReportParser::printDescriptorSummary(const std::vector<uint8_t>& descriptor) {
    Serial.println("\n=== Descriptor Summary ===");
    
    // Count different item types
    int mainItems = 0, globalItems = 0, localItems = 0;
    int inputItems = 0, outputItems = 0, featureItems = 0;
    int collections = 0;
    std::set<uint8_t> reportIds;
    std::set<uint16_t> usagePages;
    
    size_t offset = 0;
    while (offset < descriptor.size()) {
        uint8_t prefix = descriptor[offset];
        uint8_t size = prefix & 0x03;
        uint8_t type = (prefix >> 2) & 0x03;
        uint8_t tag = (prefix >> 4) & 0x0F;
        
        if (size == 3) size = 4;
        
        // Get value for analysis
        uint32_t value = 0;
        for (int i = 0; i < size && (offset + 1 + i) < descriptor.size(); i++) {
            value |= ((uint32_t)descriptor[offset + 1 + i]) << (i * 8);
        }
        
        // Count items
        switch (type) {
            case 0: // Main
                mainItems++;
                if (tag == 0x08) inputItems++;
                else if (tag == 0x09) outputItems++;
                else if (tag == 0x0A) collections++;
                else if (tag == 0x0B) featureItems++;
                break;
            case 1: // Global
                globalItems++;
                if (tag == 0x00) usagePages.insert(value); // Usage Page
                else if (tag == 0x08) reportIds.insert(value); // Report ID
                break;
            case 2: // Local
                localItems++;
                break;
        }
        
        offset += 1 + size;
    }
    
    Serial.printf("Total Items: %d (Main: %d, Global: %d, Local: %d)\n", 
                 mainItems + globalItems + localItems, mainItems, globalItems, localItems);
    Serial.printf("Reports: Input: %d, Output: %d, Feature: %d\n", 
                 inputItems, outputItems, featureItems);
    Serial.printf("Collections: %d\n", collections);
    
    if (!reportIds.empty()) {
        Serial.print("Report IDs: ");
        for (auto id : reportIds) {
            Serial.print(id);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    if (!usagePages.empty()) {
        Serial.print("Usage Pages: ");
        for (auto page : usagePages) {
            Serial.print(getUsagePageDescription(page));
            Serial.print(" ");
        }
        Serial.println();
    }
    
    Serial.println("===========================");
}

void HIDReportParser::printParseResults() {
    Serial.println("\n=== HID Parser Results ===");
    Serial.printf("Total items parsed: %d\n", items.size());
    Serial.printf("Reports found: %d\n", reportMap.size());
    Serial.printf("Collections found: %d\n", collections.size());
    
    printReportMap();
    printCollections();
    printDetailedAnalysis();
    
    Serial.println("===========================");
}

void HIDReportParser::printDetailedAnalysis() {
    Serial.println("\n=== Detailed HID Analysis ===");
    printItemDetails();
    printUsageAnalysis();
    printReportStructure();
    Serial.println("==============================");
}

void HIDReportParser::printItemDetails() {
    Serial.println("\nHID Descriptor Items:");
    Serial.println("Offset | Type  | Tag | Size | Data     | Description");
    Serial.println("-------|-------|-----|------|----------|------------------");
    
    size_t offset = 0;
    for (size_t i = 0; i < items.size(); i++) {
        const HIDItem& item = items[i];
        
        String typeStr = "";
        String tagStr = "";
        String description = "";
        
        // Determine type
        switch (item.type) {
            case 0: typeStr = "Main "; break;
            case 1: typeStr = "Global"; break;
            case 2: typeStr = "Local"; break;
            case 3: typeStr = "Resrvd"; break;
        }
        
        // Determine tag and description based on type
        if (item.type == 0) { // Main items
            switch (item.tag) {
                case 8: tagStr = "Input"; description = "Input report"; break;
                case 9: tagStr = "Output"; description = "Output report"; break;
                case 10: tagStr = "Collection"; description = getCollectionDescription(item.data); break;
                case 11: tagStr = "Feature"; description = "Feature report"; break;
                case 12: tagStr = "EndCol"; description = "End Collection"; break;
                default: tagStr = String(item.tag); break;
            }
        } else if (item.type == 1) { // Global items
            switch (item.tag) {
                case 0: tagStr = "UsagePage"; description = "Usage Page: 0x" + String(item.data, HEX); break;
                case 1: tagStr = "LogMin"; description = "Logical Minimum: " + String((int32_t)item.data); break;
                case 2: tagStr = "LogMax"; description = "Logical Maximum: " + String((int32_t)item.data); break;
                case 3: tagStr = "PhysMin"; description = "Physical Minimum: " + String((int32_t)item.data); break;
                case 4: tagStr = "PhysMax"; description = "Physical Maximum: " + String((int32_t)item.data); break;
                case 5: tagStr = "UnitExp"; description = "Unit Exponent: " + String(item.data); break;
                case 6: tagStr = "Unit"; description = "Unit: 0x" + String(item.data, HEX); break;
                case 7: tagStr = "ReportSize"; description = "Report Size: " + String(item.data) + " bits"; break;
                case 8: tagStr = "ReportID"; description = "Report ID: " + String(item.data); break;
                case 9: tagStr = "ReportCnt"; description = "Report Count: " + String(item.data); break;
                case 10: tagStr = "Push"; description = "Push global state"; break;
                case 11: tagStr = "Pop"; description = "Pop global state"; break;
                default: tagStr = String(item.tag); break;
            }
        } else if (item.type == 2) { // Local items
            switch (item.tag) {
                case 0: tagStr = "Usage"; description = "Usage: 0x" + String(item.data, HEX); break;
                case 1: tagStr = "UsageMin"; description = "Usage Minimum: 0x" + String(item.data, HEX); break;
                case 2: tagStr = "UsageMax"; description = "Usage Maximum: 0x" + String(item.data, HEX); break;
                case 3: tagStr = "DesigIdx"; description = "Designator Index: " + String(item.data); break;
                case 4: tagStr = "DesigMin"; description = "Designator Minimum: " + String(item.data); break;
                case 5: tagStr = "DesigMax"; description = "Designator Maximum: " + String(item.data); break;
                case 7: tagStr = "StrIdx"; description = "String Index: " + String(item.data); break;
                case 8: tagStr = "StrMin"; description = "String Minimum: " + String(item.data); break;
                case 9: tagStr = "StrMax"; description = "String Maximum: " + String(item.data); break;
                case 10: tagStr = "Delim"; description = "Delimiter"; break;
                default: tagStr = String(item.tag); break;
            }
        }
        
        Serial.printf("%6d | %s | %3s | %4d | 0x%06X | %s\n", 
                     offset, typeStr.c_str(), tagStr.c_str(), 
                     item.size, item.data, description.c_str());
        
        offset += 1 + item.size; // 1 byte for prefix + data bytes
    }
}

void HIDReportParser::printUsageAnalysis() {
    Serial.println("\nUsage Analysis:");
    Serial.println("\nUsage Pages Detected:");
    bool foundGenericDesktop = false, foundKeyboard = false, foundConsumer = false;
    
    for (const auto& item : items) {
        if (item.type == 1 && item.tag == 0) { // Usage Page
            switch (item.data) {
                case HID_USAGE_PAGE_GENERIC_DESKTOP:
                    if (!foundGenericDesktop) {
                        Serial.println("  • Generic Desktop Controls (0x01)");
                        Serial.println("    - Mouse, Keyboard, Joystick controls");
                        foundGenericDesktop = true;
                    }
                    break;
                case HID_USAGE_PAGE_KEYBOARD:
                    if (!foundKeyboard) {
                        Serial.println("  • Keyboard/Keypad (0x07)");
                        Serial.println("    - Key codes and modifiers");
                        foundKeyboard = true;
                    }
                    break;
                case HID_USAGE_PAGE_CONSUMER:
                    if (!foundConsumer) {
                        Serial.println("  • Consumer Controls (0x0C)");
                        Serial.println("    - Media keys, volume, power controls");
                        foundConsumer = true;
                    }
                    break;
                default:
                    Serial.printf("  • Unknown Usage Page: 0x%02X\n", item.data);
                    break;
            }
        }
    }
}

void HIDReportParser::printReportStructure() {
    Serial.println("\nReport Structure Analysis:");
    
    if (reportMap.empty()) {
        Serial.println("  No reports defined");
        return;
    }
    
    for (const auto& pair : reportMap) {
        const HIDReportInfo& info = pair.second;
        String typeStr = (info.reportType == HID_REPORT_TYPE_INPUT) ? "Input" :
                        (info.reportType == HID_REPORT_TYPE_OUTPUT) ? "Output" : "Feature";
        
        Serial.printf("\nReport ID %d (%s):\n", info.reportId, typeStr.c_str());
        Serial.printf("  Size: %d bits (%d bytes)\n", info.reportSize, (info.reportSize + 7) / 8);
        Serial.printf("  Description: %s\n", info.description.c_str());
        
        // Provide usage recommendations
        if (info.reportType == HID_REPORT_TYPE_INPUT) {
            Serial.println("  Usage: Device sends this data to host");
            if (info.description.indexOf("Keyboard") >= 0) {
                Serial.println("  Expected: Modifier keys (1 byte) + Reserved (1 byte) + Key codes (6 bytes)");
            } else if (info.description.indexOf("Consumer") >= 0) {
                Serial.println("  Expected: Media control codes (variable length)");
            }
        } else if (info.reportType == HID_REPORT_TYPE_OUTPUT) {
            Serial.println("  Usage: Host sends this data to device");
            Serial.println("  Expected: LED states or other output controls");
        } else {
            Serial.println("  Usage: Bidirectional configuration data");
        }
    }
    
    // Calculate total bandwidth
    uint32_t totalInputBits = 0, totalOutputBits = 0;
    for (const auto& pair : reportMap) {
        const HIDReportInfo& info = pair.second;
        if (info.reportType == HID_REPORT_TYPE_INPUT) {
            totalInputBits += info.reportSize;
        } else if (info.reportType == HID_REPORT_TYPE_OUTPUT) {
            totalOutputBits += info.reportSize;
        }
    }
    
    Serial.println("\nBandwidth Analysis:");
    Serial.printf("  Input Reports: %d bits (%d bytes) total\n", totalInputBits, (totalInputBits + 7) / 8);
    Serial.printf("  Output Reports: %d bits (%d bytes) total\n", totalOutputBits, (totalOutputBits + 7) / 8);
}

void HIDReportParser::printReportMap() {
    if (reportMap.empty()) {
        Serial.println("No reports found");
        return;
    }
    
    Serial.println("\nReport Map:");
    for (const auto& pair : reportMap) {
        const HIDReportInfo& info = pair.second;
        String typeStr = (info.reportType == HID_REPORT_TYPE_INPUT) ? "Input" :
                        (info.reportType == HID_REPORT_TYPE_OUTPUT) ? "Output" : "Feature";
        Serial.printf("  ID %d: %s, %d bits, %s\n", 
                     info.reportId, typeStr.c_str(), info.reportSize, info.description.c_str());
    }
}

void HIDReportParser::printCollections() {
    if (collections.empty()) {
        Serial.println("No collections found");
        return;
    }
    
    Serial.println("\nCollections:");
    for (size_t i = 0; i < collections.size(); i++) {
        const HIDCollection& coll = collections[i];
        Serial.printf("  %d: %s (Page: 0x%02X, Usage: 0x%02X)\n",
                     i, coll.description.c_str(), coll.usagePage, coll.usage);
    }
}

void HIDReportParser::printHexDump(const std::vector<uint8_t>& data) {
    Serial.println(formatHexDump(data));
}

String HIDReportParser::formatHexDump(const std::vector<uint8_t>& data, size_t bytesPerLine) {
    String result = "";
    
    for (size_t i = 0; i < data.size(); i += bytesPerLine) {
        result += String(i, HEX) + ": ";
        
        // Hex bytes
        for (size_t j = 0; j < bytesPerLine && (i + j) < data.size(); j++) {
            result += String(data[i + j], HEX) + " ";
        }
        
        // Padding
        for (size_t j = data.size() - i; j < bytesPerLine; j++) {
            result += "   ";
        }
        
        result += " |";
        
        // ASCII representation
        for (size_t j = 0; j < bytesPerLine && (i + j) < data.size(); j++) {
            char c = data[i + j];
            result += (c >= 32 && c <= 126) ? String(c) : ".";
        }
        
        result += "|\n";
    }
    
    return result;
}

String HIDReportParser::formatReportData(const ReportData& report, bool includeHex) {
    String timestamp = String(report.timestamp);
    String result = "[" + timestamp + "] ";
    
    if (!report.decodedData.isEmpty()) {
        result += report.decodedData;
    } else {
        result += "Report ID " + String(report.reportId) + ": ";
        for (size_t i = 0; i < report.data.size(); i++) {
            if (i > 0) result += " ";
            result += String(report.data[i], HEX);
        }
    }
    
    if (includeHex && !report.decodedData.isEmpty()) {
        result += " [";
        for (size_t i = 0; i < report.data.size(); i++) {
            if (i > 0) result += " ";
            result += String(report.data[i], HEX);
        }
        result += "]";
    }
    
    return result;
}