---
title: Hardware Guide
nav_order: 3
---

# Hardware Components Guide

- Simple Sexy PCB created by **Chimpo** [Forum](https://forum.dangerousthings.com/t/completed-personalized-electronic-nfc-ignition-system)

This document provides detailed information about each hardware component used in the Chelonian Access system, including current usage and potential capabilities.

## Core Components

### 1. ESP32-C3 SuperMini

**Specifications:**

- Microcontroller: ESP32-C3 (RISC-V)
- Operating Voltage: 5V (native)
- Clock Speed: 160MHz
- Flash Memory: 4MB
- RAM: 400KB SRAM
- ROM: 384KB
- Digital I/O Pins: 11
- PWM Channels: 11
- Analog Inputs: 6 (A0-A5)
- WiFi: 802.11 b/g/n (2.4 GHz)
- Bluetooth: 5.0 (BLE)
- Size: Ultra-compact 22.52 x 18mm

**Current Usage:**

- Pin 10: RFID SS (Slave Select)
- Pin 9: Relay 1 (Door unlock)
- Pin 6: Relay 2
- Pin 5: Relay 3
- Pin 4: Relay 4
- Pin 1: JQ6500 TX (to JQ6500 RX)
- Pin 0: JQ6500 RX (from JQ6500 TX)
- MISO/MOSI/SCK: SPI for RFID

**Available Features Not Yet Used:**

- Pin 8 (Blue LED) - Status indication
- WiFi capabilities for remote access
- Bluetooth LE for mobile app integration
- Deep sleep modes (43μA in deep sleep)
- ADC pins for battery monitoring
- PWM for LED brightness control
- RTC peripheral for timekeeping
- Touch sensor capabilities

**Future Potential:**

- Over-the-air (OTA) updates
- Web server for configuration
- MQTT integration
- BLE beacon functionality
- ESP-NOW for mesh networking

### 2. PN532 NFC/RFID Module

**Specifications:**

- Operating Voltage: 3.3V-5V
- Communication: SPI, I2C, or HSU (High Speed UART)
- Supported Cards: Mifare 1K, 4K, Ultralight, DesFire, and more
- Read Range: 5-7cm typically
- Frequency: 13.56MHz
- Current Consumption: ~150mA peak, ~50mA average

**Current Usage:**

- SPI communication mode
- Reading Mifare Classic 4-byte and 7-byte UIDs
- Basic card presence detection

**Available Features Not Yet Used:**

- Card emulation mode
- Peer-to-peer communication
- Writing data to cards
- Reading card memory sectors
- Authentication with card keys
- Anti-collision for multiple cards
- Low power card detection mode

**Future Potential:**

- Store access levels on the card itself
- Use card memory for logging
- Implement rolling codes for enhanced security
- Clone card detection
- Mobile phone NFC support

### 3. JQ6500 MP3 Player Module

**Specifications:**

- Operating Voltage: 3.2V-5V
- Communication: UART (9600 baud default)
- Supported formats: MP3, WAV
- Storage: Onboard flash memory (varies by model) or SD card
- Audio Output: Direct speaker drive or line out
- Control: Serial commands or button interface
- Current Consumption: ~20mA idle, ~200mA when driving speaker

**Current Usage:**

- Playing 6 audio tracks:
  1. Power-up sound
  2. "Are you still there" (10s timeout)
  3. Access granted
  4. Access denied (first attempt)
  5. Access denied (second attempt)
  6. Access denied (multiple attempts)
- Serial communication at 9600 baud
- Volume control via software

**Available Features Not Yet Used:**

- Busy pin for playback status
- Button control interface
- Folder-based organization
- Random playback
- Loop modes
- EQ settings

**Future Potential:**

- Voice announcements for card holders
- Multi-language support
- Background music/ambience
- Emergency alarm sounds
- Status announcements
- Custom sound effects

### 4. Relay Module - SRD-05VDC-SL-C (4-Channel)

**Specifications:**

- Model: SRD-05VDC-SL-C
- Channels: 4 independent relays
- Type: Active LOW (LOW signal = relay ON)
- Operating Voltage: 5V relay coil, 3.3V-5V logic compatible
- Relay Ratings:
  - 10A @ 250VAC
  - 10A @ 125VAC
  - 10A @ 30VDC
  - 10A @ 28VDC
- Isolation: Optocoupler isolated
- Current per relay: ~70-80mA when active
- Contact Form: SPDT (NO/NC/COM)

**Current Usage:**

- Relay 1: Door lock/unlock (10-second activation)
- Relay 2-4: Currently unused

**Available Features Not Yet Used:**

- Relay 2: Could control lights
- Relay 3: Could control alarm/siren
- Relay 4: Could control auxiliary systems
- NO/NC contacts for fail-safe/fail-secure
- Status LED on each relay

**Future Potential:**

- Interior light control
- Engine start/stop
- Alarm system integration
- Garage door control
- Sequential activation patterns
- Power control for other modules

### 5. Power Supply - Mini360 Buck Converter

**Specifications:**

- Model: Mini360 DC-DC Buck Converter
- Input Voltage: 4.75V-23V
- Output Voltage: 1V-17V (adjustable)
- Output Current: 1.8A continuous, 3A peak
- Efficiency: Up to 96%
- Switching Frequency: 340kHz
- Size: Ultra-compact module
- Protection: Over-current, thermal shutdown

**Current Usage:**

- Converting 12V automotive power to stable 5V
- Powering all system components
- Set to 5V output for relay module and component compatibility

**Features:**

- Adjustable output via trim pot
- High efficiency reduces heat
- Wide input range handles automotive voltage fluctuations
- Compact size for easy integration

## Power Considerations

### Current Consumption Breakdown

- ESP32-C3: ~100mA (active WiFi), ~20mA (idle), 43μA (deep sleep)
- PN532: ~50mA (average), 150mA (peak)
- JQ6500: ~20mA (idle), 200mA (playing with speaker)
- Relays: ~80mA per active relay
- **Total**: ~90mA idle, ~350-450mA active

### Power Optimization Opportunities

1. ESP32-C3 deep sleep: 43μA
2. PN532 power down: ~10μA
3. JQ6500 standby mode
4. Relay holding current reduction
5. WiFi/BLE power management

## Expansion Possibilities

### I2C / SPI ??

- RTC module (DS3231) for time-based access
- OLED display for status
- Additional EEPROM storage
- Temperature/humidity sensors
- Accelerometer for tamper detection

### Wireless Capabilities

- WiFi configuration portal
- Mobile app via BLE
- MQTT for home automation
- ESP-NOW mesh networking
- OTA firmware updates

## Best Practices

1. **Power Supply**: Mini360 provides stable 5V from automotive 12V
2. **Wiring**: Keep SPI lines short for RFID reliability
3. **Grounding**: Common ground for all components
4. **Protection**: Flyback diodes included in relay module
5. **Shielding**: Keep RFID antenna away from metal/interference
6. **Audio**: JQ6500 can drive speaker directly
