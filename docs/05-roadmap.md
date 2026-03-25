# Roadmap

This document outlines **planned and potential future features** for this project.

> None of the features listed here are guaranteed or currently implemented unless explicitly stated elsewhere.

---

## Short-Term Goals

These are features that align closely with the current system and are the most likely next steps.

### Door Locking (Secondary Relay)
- Add support for **locking doors** in addition to unlocking
- Use a second relay or timed signal logic
- Integrate with existing access flow

---

### Expanded Relay Usage
- Utilize additional relays (2–4) for:
  - Multiple door control
  - Auxiliary outputs

---

### Improved User Storage
- Expand flash/EEPROM handling
- Support:
  - More user cards
  - Better memory management
  - More reliable persistence

---

## Mid-Term Goals

### Vehicle Start Control
- Use relay output to trigger ignition/start system
- Requires:
  - Careful timing control
  - Safety considerations
  - Optional multi-step authentication

---

### Configurable Behavior
- Allow configuration of:
  - Relay timing
  - Delay durations
  - Access rules

- Could be implemented via:
  - Serial interface
  - Config file in flash

---

### Enhanced Feedback System
- More flexible LED behavior
- Additional audio cues
- Customizable feedback patterns

---

## Long-Term Goals

### Wireless Connectivity (WiFi / BLE)
- Remote configuration and monitoring
- Potential features:
  - Mobile app integration
  - Remote unlock
  - OTA updates

---

### Web Interface
- Browser-based configuration portal
- Manage:
  - User cards
  - System settings
  - Logs

---

### Remote Logging
- Store logs in flash or send over network
- Enable:
  - Event history tracking
  - Debugging without serial connection

---

## Experimental / Under Consideration

These features are ideas and may not be implemented.

- Multi-factor authentication (RFID + BLE/phone)
- Encrypted UID handling
- Secure communication protocols
- Sensor integration (door state, motion detection)

---

## Design Philosophy for Future Features

All future development should follow:

- **Vehicle access first** – core functionality must remain stable  
- **Non-blocking design** – avoid delays that impact responsiveness  
- **Modularity** – features should integrate cleanly with existing controllers  
- **Testability** – new features should be testable with mocks  

---

## Summary

The roadmap focuses on evolving Chelonian Access from a:

> **Basic RFID unlock system → Full vehicle access control platform**

While maintaining simplicity, reliability, and modular design.