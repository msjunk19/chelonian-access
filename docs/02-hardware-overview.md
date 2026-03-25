# Hardware Overview

This document describes the hardware used in **Chelonian Access** and how each component contributes to the vehicle access control system.

---

## System Overview

The system is built around an **ESP32-C3 SuperMini**, which handles:

- RFID card reading
- Access validation
- Relay control (door locks)
- Audio and LED feedback
- Serial logging

---

## Core Components

| Component | Purpose |
|-----------|--------|
| ESP32-C3 SuperMini | Main controller |
| PN532 RFID Reader | Reads RFID cards (user + master) |
| 4-Channel Relay Module | Controls door locks (Relay 1 used currently) |
| JQ6500 Audio Module | Provides audio feedback (optional) |
| Mini360 Buck Converter | Regulates input voltage to stable levels |
| LEDs | Neopixel OR single Color | Visual feedback (optional) |

---

## Component Details

### ESP32-C3 SuperMini
- Central controller for all logic
- Handles communication with RFID reader
- Controls relays, audio, and LEDs
- Provides serial debugging output

---

### PN532 RFID Reader
- Used to read RFID cards and UIDs
- Serial SPI communication
- Detects both master and user cards

---

### Relay Module (4-Channel)
- Used to control vehicle door locks
- **Relay 1** is currently used for unlock
- Additional relays are reserved for future features:
  - Locking doors
  - Vehicle ignition/start
  - Additional access points

> ⚠️ Ensure relay module is compatible with your voltage and current requirements.

---

### JQ6500 Audio Module (Optional)
- Plays audio feedback for system events
- Examples:
  - Access granted
  - Access denied
  - Inactivity prompt

**Disabling audio:**
- Do not connect a speaker

---

### Mini360 Buck Converter
- Steps down input voltage to stable 5V
- Required for reliable operation in vehicle environments

---

### LEDs (Optional)
- Neopixel OR Single Color LED
- Provide visual feedback:
  - Access granted
  - Access denied
  - System statuses

---

## Wiring Overview

> ⚠️ Exact pin assignments depend on your configuration. This section describes logical connections.

### RFID (PN532)
- Connected via SPI
- Communicates with ESP32 for card reads

### Relay Module
- Relay 1 input connected to ESP32 GPIO
- Controls door unlock mechanism

### Audio Module (JQ6500)
- Connected via UART (serial communication) to the ESP32-C3
- Outputs to speaker (optional)

### Power
- Input voltage regulated by Mini360
- Ensure stable power for ESP32 and relay module

---


## Safety Notes

- Verify relay ratings before connecting to vehicle systems
- Ensure proper grounding
- Test system outside of vehicle before permanent installation
- Use appropriate fusing where necessary

---

## Future Hardware Expansion

- Additional relays for:
  - Locking doors
  - Vehicle ignition
- Wireless modules (WiFi/BLE)
- Sensors (door state, motion, etc.)

> These are not currently implemented but are part of the planned roadmap.