# 🐢 Chelonian Access - Modern RFID Entry System

[![PlatformIO CI](https://github.com/dapperdivers/chelonian-access/workflows/PlatformIO%20CI/badge.svg)](https://github.com/dapperdivers/chelonian-access/actions)
[![codecov](https://codecov.io/gh/dapperdivers/chelonian-access/branch/main/graph/badge.svg)](https://codecov.io/gh/dapperdivers/chelonian-access)

![Chelonian Access System](Simple%20Sexy%20AC%20V2.png)

A modern, secure RFID access control system, now known as Chelonian Access (formerly Simple Sexy AC), built on the ESP32-C3 SuperMini platform. This project takes the original SimpleSexyAC concept and transforms it into a robust, testable, and expandable access control system.

## 🚀 Getting Started

- [Development Environment Setup Guide](docs/guides/DEVELOPMENT_SETUP.md) - Complete guide to setting up your development environment
- [Hardware Setup Guide](docs/HARDWARE_GUIDE.md) - Instructions for assembling and connecting hardware
- [Project Overview](docs/PROJECT_OVERVIEW.md) - Architecture and design overview
- [Current Features](docs/CURRENT_FEATURES.md) - Detailed feature documentation

## ✨ Current Features

### 🔐 **RFID Access Control**

- RFID card reading with support for 4-byte and 7-byte UIDs
- Validates against stored authorized cards (hardcoded)
- Brute-force protection with progressive delays (1-68 seconds)
- 10-second door unlock on valid card presentation
- Anti-passback protection through delay mechanism

### 🔊 **Audio Feedback System**

- 6 different sound effects for various states:
  - Power-up confirmation
  - "Are you still there?" prompt after 10 seconds
  - Access granted sound
  - Three levels of access denied sounds
- Volume control via JQ6500 MP3 player module

### 🚪 **Relay Control**

- 4 relay outputs available (active LOW)
- Relay 1 activates for 10 seconds on valid card
- Automatic deactivation after timeout
- Relays 2-4 available but currently unused

### 🏗️ **Software Architecture**

- **Modular OOP Design** - Clean separation of concerns
- **Comprehensive Unit Testing** - Full test coverage with Unity framework
- **Mock Objects** - Hardware abstraction for testing
- **PlatformIO Based** - Modern embedded development workflow

## 📊 Project Status

| Aspect | Status | Details |
|--------|--------|---------|
| **Core Functionality** | ✅ Complete | RFID reading, relay control, audio feedback |
| **Code Quality** | ✅ Excellent | Modular OOP, unit tested, well-documented |
| **Hardware Utilization** | ⚠️ 20% | Significant untapped potential |
| **Documentation** | ✅ Extensive | 14 feature guides + architecture docs |
| **Power Efficiency** | ❌ Basic | Deep sleep modes not implemented |
| **Configuration** | ❌ Hardcoded | Flash storage not utilized |

## 📚 Documentation

### Core Documentation

- **[Project Overview](docs/PROJECT_OVERVIEW.md)** - Complete system architecture and capabilities
- **[Hardware Guide](docs/HARDWARE_GUIDE.md)** - Detailed component specifications
- **[Current Features](docs/CURRENT_FEATURES.md)** - Implemented vs. possible features

### Enhancement Guides

- **[Feature Roadmap](docs/features/feature-roadmap.md)** - Comprehensive guide to potential enhancements
- **[Feature Documentation](docs/features/)** - Detailed implementation guides for each enhancement

## 🚀 Quick Start

### Prerequisites

- PlatformIO Core or IDE
- ESP32-C3 SuperMini board
- PN532 NFC/RFID module
- 4-Channel relay module (SRD-05VDC-SL-C)
- JQ6500 MP3 player module + speaker (optional for audio)
- Mini360 DC-DC buck converter (12V to 5V)

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/msjunk19/chelonian-access.git
   cd chelonian-access
   ```

2. Build the project:

   ```bash
   pio run
   ```

3. Upload to your ESP32-C3:

   ```bash
   pio run -t upload
   ```

4. Run unit tests:

   ```bash
   pio test
   ```

## 🛠️ Hardware Components

| Component | Model | Current Use | Potential |
|-----------|-------|-------------|-----------|
| **Microcontroller** | ESP32-C3 SuperMini | Basic I/O only | WiFi, BLE, deep sleep available |
| **RFID Reader** | PN532 NFC Module | Read UIDs only | Write cards, encryption possible |
| **Relay Module** | 4-Channel SRD-05VDC | 1 relay (door) | 3 unused channels |
| **Audio Module** | JQ6500 MP3 Player | 6 sound effects | Multiple folders, status available |
| **Power Supply** | Mini360 Buck Converter | 12V→5V conversion | Efficient, stable |

## 🚧 Untapped Hardware Capabilities

The ESP32-C3 SuperMini has many built-in features that are not yet utilized:

- **WiFi 802.11 b/g/n** - Remote management, OTA updates
- **Bluetooth 5.0 BLE** - Smartphone as key, configuration
- **4MB Flash Storage** - Persistent settings, logging
- **Deep Sleep Mode** - Ultra-low 43μA power consumption
- **Touch Sensor Support** - Alternative input methods
- **Hardware Encryption** - Secure communications

## 📈 Potential Additions

### Zero-Cost Software Enhancements

These enhancements require no additional hardware and can significantly improve the system:

1. **[LED Status Indicators](docs/features/01-led-status-indicators.md)** - Use built-in blue LED for visual feedback
2. **[Low Power Sleep Mode](docs/features/11-low-power-sleep-mode.md)** - Enable 43μA deep sleep for battery operation
3. **[Master Card Programming](docs/features/06-master-card-programming.md)** - Add/remove cards without reprogramming
4. **[Flash Storage](docs/features/12-flash-persistence.md)** - Persistent settings using 4MB onboard flash

### Additional Enhancement Categories

The [Feature Roadmap](docs/features/feature-roadmap.md) contains detailed guides for many more potential enhancements, including:

- **Wireless Features** - WiFi portal, BLE integration, OTA updates, MQTT support
- **Enhanced Security** - Multiple access levels, logging, time-based access, emergency override
- **Advanced Features** - Battery backup, dual authentication, scheduled relay control

All enhancements are documented with implementation guides, hardware requirements, and code examples.

## 🏗️ Project Structure

```txt
chelonian-access/
├── docs/                    # Comprehensive documentation
│   ├── features/           # Enhancement implementation guides
│   ├── PROJECT_OVERVIEW.md # System architecture
│   ├── HARDWARE_GUIDE.md   # Component details
│   └── CURRENT_FEATURES.md # Feature comparison
├── include/                # Header files
│   ├── rfid_controller.h
│   ├── relay_controller.h
│   └── audio_controller.h
├── src/                    # Implementation files
│   ├── main.cpp
│   ├── rfid_controller.cpp
│   ├── relay_controller.cpp
│   └── audio_controller.cpp
├── lib/                    # Mock libraries for testing
│   └── mocks/
├── test/                   # Comprehensive unit tests
│   └── test_native/
└── platformio.ini         # Build configuration
```

## 🔧 Configuration

Currently, authorized UIDs must be hardcoded in `src/rfid_controller.cpp`:

```cpp
void RFIDController::initializeDefaultUIDs() {
    // Add your UIDs here
    std::array<uint8_t, 4> testUID4B = {0xB4, 0x12, 0x34, 0x56};
    addUID4B(testUID4B.data());
}
```

Future versions will support flash storage and master card programming.


## 🧪 Testing

The project includes comprehensive unit tests for all modules: (must add support for new code)

```bash
# Run all tests
pio test

# Run specific test
pio test -f test_relay

# Test with verbose output
pio test -v
```

## ⚠️ Important Warnings

> **IMPORTANT**: The ESP32-C3 operates at 3.3V logic levels. The system uses 5V power through the Mini360 converter for relay compatibility.

> **CRITICAL**: Ensure proper voltage settings on the Mini360 buck converter (5V) before connecting to the system.

## 🤝 Contributing

Contributions are welcome! Areas where help is especially appreciated:

- Implementing any of the features from the roadmap
- Testing WiFi/BLE features
- Web dashboard development
- Mobile app development (iOS/Android)
- PCB design for permanent installation
- 3D printed enclosure designs
- Security auditing

Please check the [Feature Roadmap](docs/features/feature-roadmap.md) for areas to contribute.

## 📄 License

MIT License - See [LICENSE](.github/LICENSE.md) for details

## 🙏 Acknowledgments

- Simple Sexy PCB created by **Chimpo**
- Original Software: [SIMPLE-SEXY](https://github.com/chiplocks/SIMPLE-SEXY)
- RFID implants: [Dangerous Things](https://dangerousthings.com/)
- Base Software: [Chelonian-Access](https://github.com/dapperdivers/chelonian-access)
---

**Made with ❤️ for the Dangerous Things community**
