# Getting Started with Chelonian Access

This guide will help you set up **vehicle access control** on the ESP32-C3 and verify that your system works.

---

## 1. Hardware Requirements

| Component | Notes |
|-----------|-------|
| ESP32-C3 SuperMini | Main MCU |
| PN532 | RFID reader (I2C or SPI) |
| 4-channel relay | Relay 1 controls door unlock |
| JQ6500 | Audio feedback module (optional; speaker can be disconnected) |
| Mini360 buck converter | Stable 3.3V/5V power supply |
| LEDs | Optional visual feedback |

> Audio and LED feedback are optional; leaving them disconnected does not break core functionality.

---

## 2. Wiring Overview

- **PN532 RFID Reader**: I2C or SPI interface  
- **Relay 1**: Controls vehicle door lock/unlock  
- **JQ6500 Audio Module**: Optional speaker output  
- **LEDs**: Optional visual indicators  
- **Power**: Mini360 regulates 3.3V/5V for stable operation

> Pin numbers depend on your ESP32-C3 layout and board variant. Refer to the schematics in `docs/02-hardware-overview.md`.

---

## 3. Software Setup

   - Install [PlatformIO](https://platformio.org/) or Arduino IDE
   - Clone the repository:
     ```bash
     git clone https://github.com/msjunk19/chelonian-access.git
     cd chelonian-access
     ```
   - Build and upload to ESP32-C3:
     ```bash
     platformio run --target upload
     ```
   - Verify that the serial console logs startup messages and reads card UIDs

---

## 4. Basic Operation

1. **Scan a valid card** → Relay 1 activates → LED/audio feedback → Event logged.  
2. **Scan an invalid card** → Delay before next attempt → LED/audio feedback → Event logged.  
3. **Master card management** → Add/remove user cards stored in flash/EEPROM.  

---

## 5. Next Steps

- Verify door unlock functionality with your relay.  
- Customize LED/audio feedback if desired.  
- Review `/docs/02-hardware-overview.md` for detailed wiring diagrams.  
- Explore unit tests in `/tests` for logic validation.