# Current Features

This document describes the **currently implemented and working features** of Chelonian Access in detail.

> Only features listed here are considered stable and part of the current system.

---

## RFID Access Control

### Card Reading
- Supports reading RFID cards via PN532
- Handles both **4-byte and 7-byte UIDs**
- Continuously scans for card presence in the main loop

---

### Card Validation
- Compares scanned UID against stored user list
- Supports:
  - **Master card**
  - **User cards**

---

## User Management

### Master Card Programming
- Master card enables programming mode
- While active:
  - Scanning a new card → **adds user**
  - Scanning an existing card → **removes user**

---

### Storage (Flash / EEPROM)
- User cards are stored in non-volatile memory
- Data persists across power cycles
- Limited number of user slots (implementation-dependent)

---

## Relay Control

### Door Unlock
- Valid card triggers **Relay 1**
- Relay activates for a fixed duration
- Automatically deactivates after timeout

---

### Relay Availability
- System supports **4 relays**
- Currently:
  - Relay 1 → used for unlock
  - Relays 2–4 → reserved for future use

---

## Security Features

### Progressive Delay (Brute-Force Protection)
- Invalid attempts increase delay before next read
- Delay duration increases per attempt
- Prevents rapid repeated access attempts

---

### Anti-Passback
- Prevents repeated rapid use of the same card
- Requires time gap between valid scans

---

## Feedback Systems

### Audio Feedback (JQ6500)
- Provides audible system responses:
  - Startup sound
  - Access granted
  - Access denied
  - Inactivity prompt

- Controlled via UART commands

- Optional:
  - Can be disabled by removing speaker or audio files

---

### LED Feedback
- Provides visual system status:
  - Access granted
  - Access denied
  - Idle / system state

- Implementation may vary depending on hardware setup

---

## Inactivity Handling

- System tracks user inactivity
- After a timeout period:
  - Plays audio prompt (if enabled)
  - Can trigger LED indication

---

## Logging

### Serial Output
The system logs key events via serial:

- Card reads (UIDs)
- Access granted / denied
- Invalid attempt count
- Timeout durations

Used for:
- Debugging
- System monitoring
- Development validation

---

## System Architecture Features

### Modular Controllers
- RFID, Relay, Audio, and LED handled separately
- Improves maintainability and testing

---

### Non-Blocking Operation
- Uses `millis()` for timing
- Avoids blocking delays
- Allows concurrent system behavior

---

### Hardware Abstraction Layer
- Enables mocking of hardware components
- Allows testing without physical devices

---

### Unit Testing
- Uses Unity test framework
- Includes mocks for:
  - RFID
  - Relay
  - Audio

---

## Limitations

- Limited number of stored user cards
- Only one relay actively used
- No remote access or configuration
- No persistent logging beyond serial output

---

## Summary

Chelonian Access currently provides a complete **RFID-based vehicle access system** with:

- Reliable card validation
- Master card user management
- Secure relay activation
- Audio/LED feedback
- Basic security protections

Future features are documented separately in the roadmap.