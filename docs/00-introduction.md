# Chelonian Access – Introduction

**Chelonian Access** is a modular ESP32-C3–based **vehicle access control system**.  

It is designed to:  
- Unlock doors via RFID cards  
- Provide optional LED and audio feedback  
- Track access events via serial logging  

> Optional future enhancements include locking doors, starting vehicles, WiFi/BLE remote access, and expanded relay control.

---

## Design Goals

1. **Vehicle Access First** – core functionality ensures doors unlock reliably.  
2. **Modularity** – RFID, relay, and audio components are separated for easier testing and expansion.  
3. **Testability** – includes hardware abstraction layer and unit testing for logic validation.  
4. **Ease of Use** – setup and operation should be straightforward for hobbyists and small-scale installations.

---

## Audience

This system is ideal for:  
- Makers or developers who want a **customizable vehicle access system**  
- Users looking for **RFID-based door unlock** functionality  
- Projects requiring **modular, testable embedded firmware**