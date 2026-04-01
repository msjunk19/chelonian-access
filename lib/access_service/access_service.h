#pragma once
#include <stdint.h>
#include <audio_controller.h>
#include <relay_controller.h>
#include <rfid_controller.h>
#include <config.hpp>
#include <macro_config.hpp>
#include <macro_executor.hpp>
#include <access_state.hpp>

// State and configuration
extern RFIDController rfid;
extern RelayController relays;
extern AudioContoller audio;
extern MacroConfigManager macroConfigManager;


constexpr uint8_t MAXIMUM_INVALID_ATTEMPTS = 13;
extern const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS];

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
void markUserActivity(AccessLoopState &state);
void disableImpatience(AccessLoopState &state);
void fireMacro(uint8_t macroIndex);