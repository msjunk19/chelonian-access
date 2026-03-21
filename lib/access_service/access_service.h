#pragma once

#include <stdint.h>
#include "audio_controller.h"
#include "relay_controller.h"
#include "rfid_controller.h"
#include "led_controller.h"

#define PN_LED (8)
#define PN_NEOPIXEL (10)

// State and configuration
extern RFIDController rfid;
extern RelayController relays;
extern AudioContoller audio;

// --- New: encapsulated state for the loop ---
struct AccessLoopState {
    uint8_t lastMasterUID[7] = {0};
    uint8_t lastMasterUIDLen = 0;
    bool masterPresent = false;
    unsigned long masterStartTime = 0;
    unsigned long masterLastSeen = 0;

    bool scanned = false;
    bool audioQueued = false;
    uint8_t queuedSound = 0;

    unsigned long startTime = millis();
    bool impatient = false;
    bool impatientEnabled = true;

    unsigned long invalidTimeoutEnd = 0;
};

extern uint8_t invalidAttempts;
extern bool scanned;
extern bool impatient;
extern unsigned long relayActivatedTime;
extern bool relayActive;

constexpr uint8_t MAXIMUM_INVALID_ATTEMPTS = 13;
extern const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS];

constexpr uint32_t RELAY1_DURATION = 1000;
constexpr uint32_t RELAY2_DURATION = 1000;
constexpr uint8_t RELAY1_PIN = 0;
constexpr uint8_t RELAY2_PIN = 1;

// Relay state enum
enum RelayState { RELAY_IDLE, RELAY1_ACTIVE, RELAY2_PENDING, RELAY2_ACTIVE };
extern RelayState currentRelayState;



// Service logic
void accessServiceSetup();
void accessServiceLoop();
void handleRelaySequence();
void activateRelays();

// --- Helper functions ---
bool handleMasterPresenceTimeout(AccessLoopState &state);
bool validateUIDLength(uint8_t uidLength);
void handleMasterCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state);
void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state);
void handleImpatienceTimer(AccessLoopState &state);