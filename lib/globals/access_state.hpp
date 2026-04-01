// globals/access_state.hpp
#pragma once
#include <macro_config.hpp>

enum RelayState {
    RELAY_IDLE,
    RELAY_STEP_ACTIVE,
    RELAY_STEP_GAP
};

struct AccessLoopState {
    // Master card state
    uint8_t lastMasterUID[7] = {0};
    uint8_t lastMasterUIDLen = 0;
    bool masterPresent = false;
    unsigned long masterStartTime = 0;
    unsigned long masterLastSeen = 0;

    // Scan + audio
    bool scanned = false;
    bool audioQueued = false;
    uint8_t queuedSound = 0;

    // Timing
    unsigned long startTime = 0;
    unsigned long lastActivityTime = 0;

    // Impatience system
    bool impatient = false;
    bool impatientEnabled = true;

    // Invalid handling
    uint8_t invalidAttempts = 0;
    unsigned long invalidTimeoutEnd = 0;

    // Relay state
    RelayState currentRelayState = RELAY_IDLE;
    unsigned long relayActivatedTime = 0;
    bool relayActive = false;
    uint8_t currentStep = 0;
    Macro activeMacro = {};

    // Programming mode
    bool userProgrammingModeActive = false;
    unsigned long userProgLastActivityTime = 0;
    bool userProgWarningGiven = false;
};
