#pragma once
#include <stdint.h>

const unsigned long RELAY_COUNT = 4;

const unsigned long IMPATIENCE_TIMEOUT= 10000;

const unsigned long WARNING_TIMEOUT   = 10000;
const unsigned long EXIT_TIMEOUT      = 15000;

const unsigned long MASTER_HOLD_TIME  = 2000;

// Total EEPROM size
static constexpr uint16_t EEPROM_SIZE = 256;

// Split regions
static constexpr uint16_t MASTER_START = 0;
static constexpr uint16_t MASTER_SIZE  = 64;

static constexpr uint16_t USER_START   = 64;
static constexpr uint16_t USER_SIZE    = 192;

