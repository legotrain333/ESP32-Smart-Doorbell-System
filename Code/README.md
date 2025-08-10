# ESP32 Smart Doorbell System

## System Architecture

This smart doorbell system consists of three devices:

1. **Sender (ESP32)** - Doorbell button with ultrasonic sensor
2. **Middle Device/Receiver (ESP32)** - DND controller and message router (same device)
3. **Relay (ESP8266)** - LED light strip controller

## Communication Flow

```
Sender (ESP32) → Middle Device/Receiver (ESP32) → Relay (ESP8266)
     ↓                    ↓                    ↓
Button Press    DND Control & Routing    LED Light Strip Control
```

## MAC Address Configuration

### Current Configuration:
- **Sender MAC**: `9C:9E:6E:43:8A:F0` (ESP32 sender device)
- **Middle Device/Receiver MAC**: `9C:9E:6E:43:8A:F0` (ESP32 middle/receiver device - same device)
- **Relay MAC**: `F0:08:D1:05:F9:60` (ESP8266 relay device)

### Communication Paths:
1. **Sender → Middle/Receiver**: `9C:9E:6E:43:8A:F0` → `9C:9E:6E:43:8A:F0` (same device)
2. **Middle/Receiver → Relay**: `9C:9E:6E:43:8A:F0` → `F0:08:D1:05:F9:60`

## Setup Instructions

### 1. Get Actual MAC Addresses
First, upload the MAC address test files to get your actual device MAC addresses:

- **For ESP32 devices**: Upload `macesp32.ino`
- **For ESP8266 devices**: Upload `macesp01s.ino`

### 2. Update MAC Addresses
Replace the MAC addresses in each file with your actual device MACs:

#### sender.ino
```cpp
const uint8_t receiverMac[] = {0x9C, 0x9E, 0x6E, 0x43, 0x8A, 0xF0}; // Middle/Receiver device MAC
```

#### middle.ino
```cpp
const uint8_t senderMac[] = {0x9C, 0x9E, 0x6E, 0x43, 0x8A, 0xF0};   // This device's MAC
const uint8_t relayMac[] = {0xF0, 0x08, 0xD1, 0x05, 0xF9, 0x60};    // Relay device MAC
```

#### relay.ino
```cpp
const uint8_t senderMac[] = {0x9C, 0x9E, 0x6E, 0x43, 0x8A, 0xF0}; // Middle/Receiver device MAC
```

### 3. Upload Code
Upload each file to its respective device:

1. **sender.ino** → ESP32 sender device
2. **middle.ino** → ESP32 middle/receiver device (same device)
3. **relay.ino** → ESP8266 relay device

## Pin Configurations

### Sender (ESP32)
- **BUTTON_PIN**: 0 (Self-reset momentary switch)
- **RGB_RED_PIN**: 5 (RGB LED Red)
- **RGB_GREEN_PIN**: 6 (RGB LED Green)
- **RGB_BLUE_PIN**: 7 (RGB LED Blue)
- **TRIG_PIN**: 19 (Ultrasonic sensor)
- **ECHO_PIN**: 20 (Ultrasonic sensor)

### Middle Device/Receiver (ESP32) - Same Device
- **DND_BUTTON_PIN**: 15 (DND button)
- **RESET_BUTTON_PIN**: 20 (Reset button)
- **DND_LED_PIN**: 19 (DND LED)

### Relay (ESP8266)
- **RELAY_PIN**: 0 (Relay control for LED light strip - WARNING: GPIO0 affects boot mode)

## System Operation

### Normal Operation (DND OFF):
1. User presses doorbell button on sender
2. Sender sends `DOORBELL_REQUEST` to middle/receiver device
3. Middle device sends `ON` command to relay
4. Relay turns on LED light strip
5. Middle device sends `LIGHT_ON_CONFIRM` to sender
6. Sender shows green LED
7. When door opens (ultrasonic detects), sender sends `DOOR_OPEN_SIGNAL`
8. Middle device sends `OFF` command to relay
9. Relay turns off LED light strip

### DND Mode:
1. User presses DND button on middle device
2. Middle device LED shows DND status
3. When doorbell is pressed, middle device sends `DND_RESPONSE` to sender
4. Sender shows red blinking LED (5 times)
5. LED light strip does not turn on

### Reset Function:
1. User presses reset button on middle device
2. Middle device sends `OFF` command to relay
3. Middle device sends `RESET_SIGNAL` to sender
4. Sender turns off LED and resets state

## Troubleshooting

### Common Issues:

1. **Devices not communicating**:
   - Check MAC addresses are correct
   - Ensure devices are powered on
   - Check Serial Monitor for error messages

2. **Relay not responding**:
   - Verify GPIO0 wiring (affects boot mode)
   - Check relay power supply
   - Confirm relay is active LOW
   - Check LED light strip power supply

3. **Button not working**:
   - Check button wiring (should connect to GND when pressed)
   - Verify pull-up resistors are working
   - Check debounce settings

4. **Ultrasonic sensor issues**:
   - Check TRIG and ECHO pin connections
   - Verify sensor power supply
   - Check for interference or obstacles

### Debug Information:
- All devices output detailed Serial information
- LED indicators show system status
- Error conditions are logged with specific messages

## Power Management

### Sender Device:
- Deep sleep after 30 seconds of inactivity
- Wakes on button press or timer
- Power-efficient ultrasonic measurements

### Middle Device/Receiver:
- Always on for immediate response
- EEPROM saves DND state across power cycles

### Relay Device:
- Always on for immediate response
- Minimal power consumption

## Security Features

- MAC address filtering on all devices
- Message length validation
- Trusted sender verification
- EEPROM data validation with magic numbers

## Code Quality Features

- Non-blocking state machines
- Comprehensive error handling
- Retry logic for communication
- Watchdog timer protection
- Type-safe EEPROM operations
- Modular helper functions 