#pragma once
#include <stdint.h>


// Total EEPROM size
static constexpr uint16_t EEPROM_SIZE = 256;

// Split regions
static constexpr uint16_t MASTER_START = 0;
static constexpr uint16_t MASTER_SIZE  = 64;

static constexpr uint16_t USER_START   = 64;
static constexpr uint16_t USER_SIZE    = 192;

// Relay Related Variables
const unsigned long RELAY_COUNT = 4;

constexpr uint32_t RELAY1_DURATION = 1000;
constexpr uint32_t RELAY2_DURATION = 1000;
constexpr uint32_t RELAY3_DURATION = 1000;
constexpr uint32_t RELAY4_DURATION = 1000;


// Timeouts
const unsigned long IMPATIENCE_TIMEOUT= 10000;

const unsigned long WARNING_TIMEOUT   = 10000;
const unsigned long EXIT_TIMEOUT      = 15000;

const unsigned long MASTER_HOLD_TIME  = 2000;

