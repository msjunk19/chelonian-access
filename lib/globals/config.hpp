// #pragma once
// #include <stdint.h>

// static constexpr uint16_t EEPROM_SIZE = 128;

#pragma once
#include <stdint.h>

// Total EEPROM size
static constexpr uint16_t EEPROM_SIZE = 256;

// Split regions
static constexpr uint16_t MASTER_START = 0;
static constexpr uint16_t MASTER_SIZE  = 64;

static constexpr uint16_t USER_START   = 64;
static constexpr uint16_t USER_SIZE    = 192;