#pragma once
#include <stdint.h>


static constexpr uint8_t UID_MAX_LEN = 7; // maximum UID length allowed


// Total EEPROM size
static constexpr uint16_t EEPROM_SIZE = 256;

// Split regions
static constexpr uint16_t MASTER_START = 0;
static constexpr uint16_t MASTER_SIZE  = 64;

static constexpr uint16_t USER_START   = 64;
static constexpr uint16_t USER_SIZE    = 192;

// Relay Related Variables
const unsigned long RELAY_COUNT = 4; //How many relays are connected

constexpr uint32_t RELAY1_DURATION = 1000; //How long does the relay stay active
constexpr uint32_t RELAY2_DURATION = 1000; //How long does the relay stay active
constexpr uint32_t RELAY3_DURATION = 1000; //How long does the relay stay active
constexpr uint32_t RELAY4_DURATION = 1000; //How long does the relay stay active


// Timeouts
const unsigned long IMPATIENCE_TIMEOUT= 10000; //How long before you get the waiting notifications

const unsigned long WARNING_TIMEOUT   = 10000; //How long before you are warned that programming mode will time out
const unsigned long EXIT_TIMEOUT      = 15000; //How long before programming mode times out

const unsigned long MASTER_HOLD_TIME  = 2000; //How long do you have to hold the card to the reader to enter user programming mode
const unsigned long MASTER_AWAY_DELAY  = 300; //How long does the reader have to not see the master before it decides you arent holding it to the reader anymore

// Set a short timeout (150 ms) to avoid blocking the loop, 20ms works but needs to be longer to read implants. 150 is working currently. Adjust as needed for implant.
constexpr uint16_t RFID_READ_TIMEOUT_MS = 150;
