#pragma once
#include <stdint.h>

// Pins for Internal LED (8) and Neopixel (10)
#define PN_LED (8)
#define PN_NEOPIXEL (10)

// ESP32-C3 SPI pins for PN532
#define PN532_SCK (4)
#define PN532_MISO (5)
#define PN532_MOSI (6)
#define PN532_SS (7)


// Total EEPROM size
static constexpr uint16_t EEPROM_SIZE = 256;

// Split regions
static constexpr uint16_t MASTER_START = 0;
static constexpr uint16_t MASTER_SIZE  = 64;

static constexpr uint16_t USER_START   = 64;
static constexpr uint16_t USER_SIZE    = 192;