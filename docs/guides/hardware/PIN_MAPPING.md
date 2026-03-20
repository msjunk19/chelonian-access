---
title: Pin Mapping Reference
parent: Hardware Guides
nav_order: 2
---

# Pin Mapping Reference

This document provides a comprehensive reference for all pin connections in the Chelonian Access system.

## ESP32-C3 SuperMini Pin Assignments

### Core System Pins

| Pin Number | Function | Connected To | Notes |
|------------|----------|--------------|--------|
| GPIO 7 | SPI SS | PN532 SS | RFID reader slave select |
| GPIO 5 | SPI MISO | PN532 MISO | RFID reader data in |
| GPIO 6 | SPI MOSI | PN532 MOSI | RFID reader data out |
| GPIO 4 | SPI SCK | PN532 SCK | RFID reader clock |

| GPIO 0 | Relay 1 | Door Lock Relay | Active LOW trigger |
| GPIO 1 | Relay 2 | Auxiliary Relay | Available for expansion |
| GPIO 2 | Relay 3 | Auxiliary Relay | Available for expansion |
| GPIO 3 | Relay 4 | Auxiliary Relay | Available for expansion |

| GPIO 21 | TX | JQ6500 RX | Audio module serial |
| GPIO 20 | RX | JQ6500 TX | Audio module serial |

| GPIO 8 | LED | Built-in Blue LED | Status indication |

| 5V | Power In | Mini360 OUT+ | System power |
| GND | Ground | Ground Bus | System ground |


## PN532 RFID Module

| Pin | Connected To | Notes |
|-----|-------------|-------|
| SS   | ESP32 GPIO 7 | Chip select |
| MISO | ESP32 GPIO 6 | Data to ESP32 |
| MOSI | ESP32 GPIO 5 | Data from ESP32 |
| SCK  | ESP32 GPIO 4 | Clock signal |
| VCC  | ESP32 3V3    | Power input (3.3V logic required) |
| GND  | Ground Bus   | Ground reference |

## Relay Module (SRD-05VDC-SL-C)

| Pin | Connected To | Notes |
|-----|-------------|-------|
| VCC | Mini360 5V | Power input |
| GND | Ground Bus | Ground reference |
| IN1 | ESP32 GPIO 0 | Door control |
| IN2 | ESP32 GPIO 1 | Auxiliary |
| IN3 | ESP32 GPIO 2 | Auxiliary |
| IN4 | ESP32 GPIO 3 | Auxiliary |
| COM1-4 | Load Circuit | Common terminals |
| NO1-4 | Load Circuit | Normally open |
| NC1-4 | Load Circuit | Normally closed |

## JQ6500 Audio Module

| Pin | Connected To | Notes |
|-----|--------------|-------|
| VCC | Mini360 5V   | Power input |
| GND | Ground Bus   | Ground reference |
| TX  | ESP32 GPIO 20| Serial data to ESP32 |
| RX  | ESP32 GPIO 21| Serial data from ESP32 |
| SPK+| Speaker +    | 8Ω speaker |
| SPK-| Speaker -    | 8Ω speaker |

## Mini360 Buck Converter

| Pin | Connected To | Notes |
|-----|-------------|-------|
| IN+ | 12V Source | Vehicle/Power supply positive |
| IN- | Ground Bus | Input ground |
| OUT+ | 5V Rail | Regulated 5V output |
| OUT- | Ground Bus | Output ground |

## Ground Bus Connections

All the following components must share a common ground:

- ESP32-C3 SuperMini GND
- PN532 Module GND
- Relay Module GND
- JQ6500 Module GND
- Mini360 IN- and OUT-
- Power source negative terminal
- Speaker negative terminal

## Power Distribution

### 5V Power Rail
Components powered from Mini360 5V output:
- ESP32-C3 SuperMini (via 5V pin)
- Relay Module (via VCC)
- JQ6500 Module (via VCC)

### Power System
5V Components:
- ESP32-C3 SuperMini (power input)
- Relay Module
- JQ6500 Module

3.3V Components:
- PN532 RFID Module (powered from ESP32's 3.3V output)
- All ESP32 GPIO signals

## Notes on Signal Integrity

1. **SPI Bus (RFID Reader):**
   - Keep wires short and equal length
   - Avoid parallel runs with power cables
   - Consider using twisted pairs
   - Maximum recommended length: 10cm
   - All signals are 3.3V logic level

2. **Relay Control Lines:**
   - Can be longer than SPI lines
   - Keep away from RFID antenna
   - Use appropriate wire gauge
   - Consider optocoupler isolation

3. **Audio Lines:**
   - Keep speaker wires twisted
   - Route away from RFID and relay circuits
   - Shield if necessary
   - Maximum recommended length: 30cm

4. **Power Distribution:**
   - Use appropriate wire gauge
   - Star topology from Mini360
   - Minimize voltage drop
   - Consider fuse protection

## Physical Layout Recommendations

1. **Spacing Requirements:**
   - PN532 antenna: 10cm from metal/electronics
   - Relay module: 5cm from RFID components
   - Power converter: Good airflow
   - Speaker: Acoustic separation

## Validation Checklist

Before powering up:
1. [ ] Verify Mini360 output voltage (5.0V ±0.1V)
2. [ ] Check for shorts between power rails
3. [ ] Verify ground connections
4. [ ] Check relay control signal polarities
5. [ ] Verify RFID reader jumper settings
6. [ ] Check speaker polarity
7. [ ] Verify all signal connections
8. [ ] Test continuity of all connections

## Additional Resources

- [ESP32-C3 SuperMini Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [PN532 User Manual](https://www.nxp.com/docs/en/user-guide/141520.pdf)
- [JQ6500 Documentation](https://github.com/sleemanj/JQ6500_Serial)
- [SRD-05VDC-SL-C Relay Specifications](https://components101.com/switches/5v-relay-pinout-working-datasheet)

[Return to Assembly Guide](ASSEMBLY_GUIDE.md) - Return to the Guide