# ESP32 BLE Host Simulator

A professional BLE Host Simulator for ESP32 that connects to BLE HID devices and analyzes their reports. This implementation provides comprehensive scanning, connection management, and real-time HID report analysis capabilities.

## Features

- **BLE Device Scanning**: Automatic discovery of BLE HID devices with filtering options
- **Device Connection Management**: Connect to remote controls, keyboards, mice, and other HID devices
- **HID Report Analysis**: Complete parsing and decoding of HID report descriptors
- **Real-time Monitoring**: Live display and logging of incoming HID reports
- **CLI Interface**: Comprehensive command-line interface for all operations
- **Report Decoding**: Intelligent decoding of keyboard, consumer control, and mouse reports
- **Logging System**: File-based logging with automatic rotation
- **Statistics**: Detailed monitoring statistics and performance metrics

## Hardware Requirements

- ESP32 Development Board (ESP32-WROOM-32)
- Minimum 4MB Flash memory
- USB cable for programming and serial communication

## Software Requirements

- PlatformIO IDE or Arduino IDE with ESP32 support
- ESP32 BLE Arduino Library
- ArduinoJson Library

## Quick Start

1. **Clone and build:**
```bash
git clone <repository-url>
cd ble-host-simulator
pio run -t upload
```

2. **Open serial monitor:**
```bash
pio device monitor -b 115200
```

3. **Start scanning:**
```
ble-host> scan 10
ble-host> list
ble-host> pair 0
```

## CLI Commands

### Device Discovery
```bash
scan [duration] [--filter-name=name] [--filter-rssi=value]  # Scan for BLE devices
list                                                        # Show discovered devices
pair <index|address>                                       # Connect to device
disconnect                                                 # Disconnect from device
```

### Device Analysis
```bash
explain <index|address>     # Show detailed device information
services                    # Show available BLE services
status                      # Show system status
```

### Report Monitoring
```bash
monitor [--format=hex|decoded|both]    # Start report monitoring
stop-monitor                           # Stop monitoring
stats [reset]                          # Show/reset statistics
log [start|stop] [filename]            # Control logging
```

### Utility Commands
```bash
help [command]              # Show help
clear [screen|buffer]       # Clear screen or report buffer
filter [options]            # Set scan filters
reboot                      # Restart device
```

## Usage Examples

### Basic Device Connection
```
ble-host> scan 10
Scanning for BLE devices (duration: 10s)...
Found: Examote One (a4:c1:38:81:21:05) RSSI: -45 dBm
Scan completed. Found 1 devices

ble-host> pair 0
SUCCESS: Successfully connected and discovered services
INFO: Subscribed to HID reports
```

### Device Analysis
```
ble-host> explain 0
Device Details:
  Address: a4:c1:38:81:21:05
  Name: Examote One
  Device Type: Remote Control
  Services:
    HID Service: Yes
    Device Info Service: Yes
    Battery Service: Yes

=== Device Information ===
Manufacturer: Exaring
Model: Examote One
Vendor ID: 0x02d01
Product ID: 0xc02e
Battery Level: 100%

=== HID Information ===
Report Structure:
  Report ID 1: Input, Size: 64 bits, Keyboard
  Report ID 2: Input, Size: 40 bits, Consumer Control
```

### Real-time Monitoring
```
ble-host> monitor --format=both
SUCCESS: Report monitoring started (format: both)

[00:01:23.456] Consumer_PLAY_PAUSE (ID:2): PLAY_PAUSE [CD 00 00 00 00]
[00:01:24.123] No consumer keys (ID:2):  [00 00 00 00 00]
[00:01:25.789] Keyboard (ID:1): Keys: UP [00 00 52 00 00 00 00 00]
```

## Supported Devices

The system is compatible with various BLE HID devices:

- **Remote Controls**: TV remotes, media player controllers
- **Keyboards**: Bluetooth keyboards with standard HID implementation
- **Mice**: Bluetooth mice with button and movement reporting
- **Game Controllers**: Bluetooth gamepads and joysticks
- **Custom HID Devices**: Any BLE device implementing HID over GATT

## Report Decoding

The built-in HID parser can decode the following report types:

### Keyboard Reports
- Modifier keys (Ctrl, Shift, Alt, GUI)
- Character keys (A-Z, 0-9)
- Function keys (F1-F12)
- Special keys (Arrow keys, Enter, Escape, etc.)

### Consumer Control Reports
- Media controls (Play, Pause, Stop, Next, Previous)
- Volume controls (Volume Up/Down, Mute)
- System controls (Power, Menu, Home, Back)
- Custom consumer codes

### Mouse Reports
- Button states (Left, Right, Middle)
- Movement deltas (X, Y coordinates)
- Scroll wheel data

## Configuration

### Scan Filtering
```bash
filter --name="Remote" --rssi=-50    # Filter by name and signal strength
filter --clear                       # Clear all filters
```

### Monitoring Options
```bash
monitor --format=hex        # Show raw hex data only
monitor --format=decoded    # Show decoded data only
monitor --format=both       # Show both (default)
```

### Logging
```bash
log start reports.log       # Start logging to file
log stop                    # Stop logging
```

## Performance Monitoring

### System Status
```bash
ble-host> status
=== System Status ===
Free Heap: 185432 bytes
Uptime: 1234 seconds
BLE Client: Connected to a4:c1:38:81:21:05
Report Monitor: Active, 47 reports received
```

### Statistics
```bash
ble-host> stats
=== Report Monitor Statistics ===
Total Reports: 47
Monitoring Duration: 120 seconds
Reports/Second: 0.39
Reports by Type:
  Input: 45
  Output: 2
```

## Troubleshooting

### Common Issues

**Connection Failed:**
- Ensure device is in pairing mode
- Check signal strength (RSSI > -70 dBm recommended)
- Verify device supports standard BLE HID profile

**No Reports Received:**
- Confirm successful pairing with `status` command
- Check if device is sending reports (try pressing buttons)
- Verify HID service subscription with `services` command

**Memory Issues:**
- Monitor free heap with `status` command
- Reduce buffer size if needed
- Clear report buffer periodically with `clear buffer`

### Debug Information
```bash
# Enable verbose logging in platformio.ini
build_flags = -DCORE_DEBUG_LEVEL=5
```

## Technical Details

### Memory Usage
- Base system: ~80KB RAM
- Report buffer: Configurable (default: 256 reports)
- HID descriptor parsing: ~2-5KB per device

### File System
- Uses SPIFFS for log file storage
- Automatic log rotation at 1MB file size
- CSV export format supported

### BLE Configuration
- Central role only
- Supports BLE 4.0 and higher
- Maximum 4 simultaneous connections (framework limit)
- Active scanning for complete device information

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Support

For questions, issues, or feature requests, please create an issue in the repository or contact the development team.