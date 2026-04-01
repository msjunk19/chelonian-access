#pragma once
#include <stdint.h>


static constexpr uint8_t UID_MAX_LEN = 7; // maximum UID length allowed

// Default AP credentials
// constexpr char DEFAULT_SSID[] = "ACCESS-CTRL-XXXX";
constexpr char DEFAULT_SSID[] = "AC_FALLBACK_AP";
constexpr char DEFAULT_PASS[] = "default1234";

constexpr char DOMAIN[] = "chelonian.local";


// static constexpr uint8_t MAX_MASTER_UIDS = 10;
// static constexpr uint8_t MAX_USER_UIDS = 24; 
//save NVS
static constexpr uint8_t MAX_MASTER_UIDS = 2;
static constexpr uint8_t MAX_USER_UIDS = 10;

// Relay Related Variables
const unsigned long RELAY_COUNT = 4; //How many relays are connected

// constexpr uint32_t RELAY1_DURATION = 1000; //How long does the relay stay active
// constexpr uint32_t RELAY2_DURATION = 1000; //How long does the relay stay active
// constexpr uint32_t RELAY3_DURATION = 1000; //How long does the relay stay active
// constexpr uint32_t RELAY4_DURATION = 1000; //How long does the relay stay active


// Timeouts
const unsigned long IMPATIENCE_TIMEOUT= 10000; //How long before you get the waiting notifications

const unsigned long WARNING_TIMEOUT   = 10000; //How long before you are warned that programming mode will time out
const unsigned long EXIT_TIMEOUT      = 15000; //How long before programming mode times out

const unsigned long MASTER_HOLD_TIME  = 2000; //How long do you have to hold the card to the reader to enter user programming mode
const unsigned long MASTER_AWAY_DELAY  = 300; //How long does the reader have to not see the master before it decides you arent holding it to the reader anymore

static const uint32_t DETECTION_COOLDOWN_MS = 1000; // adjust (500–1500ms works well)

// Set a short timeout (150 ms) to avoid blocking the loop, 20ms works but needs to be longer to read implants. 150 is working currently. Adjust as needed for implant.
constexpr uint16_t RFID_READ_TIMEOUT_MS = 150;

const unsigned long AUDIO_DEFAULT_VOLUME = 20;


// Phone auth
static constexpr uint8_t MAX_PAIRED_PHONES = 10;
static constexpr uint8_t PHONE_SECRET_LEN  = 32;
static constexpr uint8_t PHONE_ID_MAX_LEN  = 36; // standard UUID length


#define IBEACON_MAJOR    0x0001
#define IBEACON_MINOR    0x0001
// #define IBEACON_TX_POWER 0xC5
#define IBEACON_TX_POWER 0xA0  // -96 dBm — very large region