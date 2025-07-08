# Remote Control Simulator
A clean, more complex rewrite of the RcuSim Sketch found [here.](https://github.com/exaring/RcuSim/tree/main/RcuSimSketch) 

## Hardware
Developed on an Esp32 Wroom 32 module and a NodeMcu from AZ-Delivery.
Should work on any ESP32 module with integrated Bluetooth.

### Display
A SSD1306 display (128x64) can be connected to the ESP32 and activated in teh source code using the `USE_DISPLAY` define.

The display is used to show Wifi and Ble status information.

## Configuration
A simple rs232 serial interface is used for configuration.

### Serial connection
Connect via USB (Silicon Labs CP21xx https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads).
### Parameters
* Baudrate 115.200
* Databits 8
* Parity None
* Stopbits 1

## Config commands
```
  help                  - Shows this help
  setssid               - set the ssid of the WiFi network
  setpwd                - set the password of the WiFi network
  setpwd                - set the password of the WiFi network
  setip                 - set the static IP address (format: xxx.xxx.xxx.xxx)
  setgateway            - set the gateway address (format: xxx.xxx.xxx.xxx)
  save                  - save the current WiFi configuration to NVM
  connect               - connect to the WiFi network with the current configuration
  config                - shows the current WiFi configuration
  reboot                - Restarts the device
  diag                  - Shows diagnostic information
```

Hint: first configure using the setxxx commands, then save, then connect or reboot.

## Ble usage
The Ble simulation interface can be accessed via "RESTish" HTTP GET requests.

### BLE Control
```http://{ipaddress}/api/pair``` - Starts BLE advertising for pairing
```http://{ipaddress}/api/stoppair``` - Stops BLE advertising
```http://{ipaddress}/api/unpair``` - Removes all stored BLE pairings
### Remote Control
```http://{ipaddress}/api/key?key={keycode}&delay={delay}``` - Press and release a key
Parameters: key (required), delay (optional, default=100ms)
```http://{ipaddress}/api/press?key={keycode}``` - Press a key without releasing
Parameters: key (required)
```http://{ipaddress}/api/release?key={keycode}``` - Release a previously pressed key
Parameters: key (required)
```http://{ipaddress}/api/releaseall``` - Release all currently pressed keys
### System
```http://{ipaddress}/api/system/diagnostics``` - Detailed system information
```http://{ipaddress}/api/system/battery?level={level}```Set Battery Level - Set the reported battery level
Parameters: level (0-100)
```http://{ipaddress}/api/system/reboot``` - Restart the ESP32


## Other stuff
### Potential housings
https://www.thingiverse.com/thing:2448685
https://www.printables.com/model/503766-case-for-096-oled-wemos-d1-mini-push-buttons?lang=de
https://www.printables.com/model/585299-ssd1306-case