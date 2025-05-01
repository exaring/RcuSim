# RcuSim
Initial Esp32 sketch to simulate BLE Remote Controls. Based on https://github.com/T-vK/ESP32-BLE-Keyboard and adopted to newer api's. Prototype only!

## Hardware
Developed on an Esp32 Wroom 32 module from AZ-Delivery.
Should work on any ESP32 module with integrated Bluetooth.

## Control interface
Initial version uses a simple rs232 serial interface for management.

### Serial connection
Connect via USB (Silicon Labs CP21xx https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads).
### Parameters
* Baudrate 115.200
* Databits 8
* Parity None
* Stopbits 1

### Commands and Keys
```
unpair                - Removes all stored pairings
press <key>           - Presses a key (can use name, character, or 0xXX format)
release <key>         - Releases a key (can use name, character, or 0xXX format)
key <key> [delay]     - Presses a key and releases it after delay ms (default: 50ms)
type <text>           - Sends a text
releaseall            - Releases all pressed keys
disconnect            - Terminates the current connection
battery <percentage>  - Sets the reported battery level (0-100)
reboot                - Restarts the device
diag                  - Shows diagnostic information
config                - Shows current device configuration

=== Key Formats ===
- Single character    - Example: a, B, 7
- Named key           - Example: enter, f1, up, space
- Media key           - Example: playpause, mute, volumeup
- Hex value           - Example: 0x28, 0xE2, 0x1A

=== Available Regular Keys ===
up, down, left, right, enter
return, esc, escape, backspace, tab
space, ctrl, alt, shift, win
gui, insert, delete, del, home
end, pageup, pagedown, capslock, f1
f2, f3, f4, f5, f6
f7, f8, f9, f10, f11
f12, printscreen, scrolllock, pause

=== Available Media Keys ===
playpause, play, pause, nexttrack
next, prevtrack, previous, prev
stop, mute, volumeup, volup
volumedown, voldown, home, computer
mypc, calc, calculator, bookmarks
favorites, search, browserstop, back
mediaselect, mail, email, power
```
