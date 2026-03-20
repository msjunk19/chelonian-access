---
title: Chelonian Access - Modern RFID Entry System
layout: home
nav_order: 1
---

{: .fs-9 }

A modern, modular RFID keyless entry system built on the ESP32-C3 SuperMini platform.
{: .fs-6 .fw-300 }

[Get started](#getting-started){: .btn .btn-primary .fs-5 .mb-4 .mb-md-0 .mr-2 } [View on GitHub](https://github.com/msjunk19/chelonian-access){: .btn .fs-5 .mb-4 .mb-md-0 }

---

## Getting Started

### Quick Links

- [Project Overview](PROJECT_OVERVIEW.MD) - Complete system architecture and capabilities
- [Hardware Guide](HARDWARE_GUIDE.MD) - Detailed component specifications
- [Current Features](CURRENT_FEATURES.MD) - Implemented vs. possible features
- [Feature Roadmap](features/feature-roadmap.md) - Comprehensive enhancement guide
- [Development Setup](guides/DEVELOPMENT_SETUP.MD) - Detailed Guide to local development
- [Firmware Installation](guides/software/01_ESP32_FIRMWARE_INSTALLATION.MD) - ESP32 firmware installation guide
- [JQ6500 File Transfer](02_JQ6500_AUDIO_INSTALLATION.MD) - Guide for putting audio files on JQ6500


### About the Project

This project transforms the original [SimpleSexy](https://github.com/chiplocks/SIMPLE-SEXY) concept into a modern, robust, and secure RFID access control system. With its enhanced security features and modular design, it provides a flexible foundation for various access control applications.

## Key Features

- **RFID Access Control** - Support for 4-byte and 7-byte UIDs
- **Audio Feedback** - 6 different sound effects for various states
- **Relay Control** - 4 relay outputs with automatic timeout
- **Modular Architecture** - Clean OOP design with comprehensive unit testing

## Hardware Components

| Component | Model | Purpose |
|-----------|-------|---------|
| **Microcontroller** | ESP32-C3 SuperMini | Main controller with WiFi/BLE |
| **RFID Reader** | PN532 NFC Module | Card reading |
| **Relay Module** | 4-Channel SRD-05VDC | Actuator control |
| **Audio Module** | JQ6500 MP3 Player | Sound feedback |
| **Power Supply** | Mini360 Buck Converter | 12V→5V conversion |

---
