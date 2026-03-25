# 🐢 Chelonian Access - Modern RFID Entry System

![Chelonian Access System](Simple%20Sexy%20AC%20V2.png)
# Chelonian Access

This project, and the docs are in development, there is a lot here to go through.

**Chelonian Access** is a modular, ESP32-C3–based **vehicle access control system**.  
It is designed to unlock vehicle doors (or other access points) via **RFID cards**, with optional audio and LED feedback. Additional features like locking doors or starting a vehicle are planned for future updates.

---

## Purpose

- Primary goal: **vehicle access control** — unlock doors via RFID cards.  
- Current capabilities:  
  - Reading and validating RFID cards (master + user cards)  
  - Adding/removing user cards via **master card**  
  - Activating **door unlock relay** (Relay 1)  
  - LED and optional audio feedback  
  - Serial logging of events and invalid attempts  

- Planned future capabilities:  
  - Locking doors, starting vehicles  
  - Multiple relay support  
  - WiFi/BLE remote access  
  - Dynamic flash storage for unlimited user cards  

---

## Features (Implemented / Verified)

| Feature | Status | Notes |
|---------|--------|-------|
| RFID card reading (4-byte & 7-byte UIDs) | ✅ | Supports master and user cards |
| Card validation | ✅ | Checks against stored authorized cards |
| Add/remove user cards (master card only) | ✅ | Limited number of user slots in flash/EEPROM |
| Brute-force protection | ✅ | Progressive delays after invalid attempts |
| Door unlock on valid card | ✅ | Relay 1 triggers unlock interval |
| Audio feedback | ✅ | Optional; integrated via JQ6500 module |
| LED feedback | ✅ | Optional visual indication of status |
| Relay outputs | ✅ | 4 channels; currently only Relay 1 used |
| Automatic relay deactivation | ✅ | Turns off after unlock interval |
| Modular OOP firmware | ✅ | Separate controllers for RFID, relay, audio |
| Unit testing framework | ✅ | Mocks for testing logic |
| Hardware abstraction layer | ✅ | Allows testing without physical devices |
| Inactivity prompt | ✅ | Audio/LED prompts after idle timeout |
| Logging via serial | ✅ | Tracks card reads, access events, invalid attempts |
| WiFi/BLE connectivity | ❌ | Planned |
| Vehicle start via relay | ❌ | Planned |
| Locking doors / multiple relays | ❌ | Planned |

> Only the above features are implemented. All other ideas are future enhancements.

---

## Hardware Requirements

| Component | Notes |
|-----------|-------|
| ESP32-C3 SuperMini | MCU |
| PN532 | RFID reader (SPI interface) |
| 4-channel relay module | Relay 1 triggers door unlock |
| JQ6500 | Audio module (optional; speaker can be disconnected) |
| Mini360 buck converter | Stable 3.3V/5V power supply |
| LEDs | Optional visual feedback integrated in PCB |

> Audio feedback is optional — leave the speaker unconnected or use empty audio files if you want to disable it.

---

## Quick Start

### 1. Software Setup

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

### 2. Testing the System

1. **Scan a valid card**  
   - Relay 1 activates to unlock the door  
   - LED/audio feedback indicates access granted  

2. **Scan an invalid card**  
   - Progressive delay prevents immediate retries  
   - LED/audio feedback indicates access denied  

3. **Add/remove user cards** (via master card)  
   - Only master card can manage user cards  
   - Stored in flash/EEPROM with limited slots  

---

### 3. Next Steps / Roadmap

| Feature | Status |
|---------|--------|
| Lock doors via relay | Planned |
| Start vehicles via relay | Planned |
| Multiple relays for additional doors | Planned |
| WiFi/BLE remote access | Planned |

---

## Documentation Structure

Full documentation is organized in `/docs`:

- [00-introduction.md](docs/00-introduction.md)
- [01-getting-started.md](docs/01-getting-started.md)
- [02-hardware-overview.md](docs/02-hardware-overview.md)
- [03-software-architecture.md](docs/03-software-architecture.md)
- [04-current-features.md](docs/04-current-features.md)
- [05-roadmap.md](docs/05-roadmap.md)
- [06-contributing.md](docs/06-contributing.md)
- [07-faq.md](docs/07-faq.md) *(not yet implemented)*
- [08-troubleshooting.md](docs/08-troubleshooting.md) *(not yet implemented)*

- **Current features**: Only stable, tested functionality  
- **Roadmap**: Planned enhancements, clearly marked  
- **Getting started & hardware overview**: Setup instructions for new contributors  

---

## Contribution

We welcome contributions! Please refer to `docs/06-contributing.md` for workflow, coding standards, and feature submission guidelines.

---

## License

[MIT License](LICENSE)
