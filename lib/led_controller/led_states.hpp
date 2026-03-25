#pragma once
#include "led_controller.h"

// --- LED State Mapping ---
// Duration 0 means indefinite (until another pattern is set)
struct LEDState {
    LEDPattern pattern;
    uint32_t duration;  // milliseconds, 0 = indefinite
    uint32_t color;
};

// System / Status Indicators
constexpr LEDState SYSTEM_READY      = { PATTERN_SLOW_BLINK, 0, LEDColor::WHITE };
// constexpr LEDState SYSTEM_IDLE       = { PATTERN_FAST_BLINK, 0, LEDColor::YELLOW };
constexpr LEDState SYSTEM_IDLE       = { PATTERN_FAST_BLINK, 0, LEDColor::ORANGE };
constexpr LEDState SYSTEM_SCANNING   = { PATTERN_FAST_BLINK, 0, LEDColor::BLUE };
constexpr LEDState WIFI_CONNECTING   = { PATTERN_DOUBLE_BLINK, 0, LEDColor::YELLOW };
constexpr LEDState WIFI_CONNECTED    = { PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN };
constexpr LEDState WAITING_TIMEOUT   = { PATTERN_SLOW_BLINK, 0, LEDColor::ORANGE };
constexpr LEDState POWER_ON_FADE     = { PATTERN_BREATHING, 3000, LEDColor::WHITE };
constexpr LEDState POWERING_ON       = { PATTERN_SLOW_BLINK, 3000, LEDColor::BLUE };
constexpr LEDState UNKNOWN_UID_TYPE  = { PATTERN_DOUBLE_BLINK, 0, LEDColor::ORANGE };

// Access Control Indicators
constexpr LEDState ACCESS_GRANTED    = { PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN };
constexpr LEDState ACCESS_DENIED     = { PATTERN_FIVE_BLINK, 0, LEDColor::RED };
constexpr LEDState DOOR_UNLOCKED     = { PATTERN_SOLID, 5000, LEDColor::GREEN };
constexpr LEDState BRUTE_FORCE_LOCK  = { PATTERN_SOS, 0, LEDColor::RED };

// Error Indicators
constexpr LEDState ERROR_RFID        = { PATTERN_SOS, 0, LEDColor::RED };
constexpr LEDState ERROR_AUDIO       = { PATTERN_FAST_BLINK, 0, LEDColor::PURPLE };
constexpr LEDState ERROR_WIFI        = { PATTERN_DOUBLE_BLINK, 0, LEDColor::YELLOW };

// Card Read States
constexpr LEDState MASTER_CARD_SET   = { PATTERN_SOLID, 2000, LEDColor::PURPLE };
constexpr LEDState PROGRAMMING_MODE  = { PATTERN_FIVE_BLINK, 0, LEDColor::PURPLE };
 
constexpr LEDState MASTER_CARD       = { PATTERN_FAST_BLINK, 0, LEDColor::PURPLE };
constexpr LEDState USER_ADDED        = { PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN };
constexpr LEDState USER_REMOVED      = { PATTERN_FIVE_BLINK, 0, LEDColor::RED };



constexpr LEDState PLACEHOLDER        = { PATTERN_SOLID, 2000, LEDColor::YELLOW };


// --- Optional helper macro for cleaner syntax ---
#define LED_SET_SEQ(state) led.setPattern((state).pattern, (state).duration, (state).color)
#define LED_ENQUEUE(state) led.enqueuePattern((state).pattern, (state).duration, (state).color)




//Working Section, improperly plays all LED sequences AFTER Audio, Needs debugging. Only needed for queued/sequential patterns. Current integration is working as desired without the ability to run more than one pattern sequentially
// #pragma once
// #include "led_controller.h"

// // --- Declare global LEDController (defined elsewhere, e.g., main.cpp) ---
// extern LEDController led;

// // --- Sub-state definition for multi-pattern sequences ---
// struct LEDSubState {
//     LEDPattern pattern;
//     uint32_t duration;  // milliseconds, 0 = indefinite
//     uint32_t color;
// };

// // --- LED state holding a sequence of sub-states ---
// struct LEDState {
//     const LEDSubState* sequence;
//     uint8_t length;
// };

// // ------------------------
// // --- Define sequences ---
// // ------------------------

// // System / status indicators
// static constexpr LEDSubState SYSTEM_READY_SEQ[] = {
//     {PATTERN_SLOW_BLINK, 0, LEDColor::WHITE}       // slow breathing until changed
// };
// static constexpr LEDSubState SYSTEM_SCANNING_SEQ[] = {
//     {PATTERN_FAST_BLINK, 0, LEDColor::BLUE}
// };
// static constexpr LEDSubState WIFI_CONNECTING_SEQ[] = {
//     {PATTERN_DOUBLE_BLINK, 0, LEDColor::YELLOW}
// };
// static constexpr LEDSubState WIFI_CONNECTED_SEQ[] = {
//     {PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN},
//     {PATTERN_SOLID, 2000, LEDColor::GREEN} 
// };
// static constexpr LEDSubState WAITING_TIMEOUT_SEQ[] = {
//     {PATTERN_SLOW_BLINK, 0, LEDColor::ORANGE}
// };
// static constexpr LEDSubState POWER_ON_FADE_SEQ[] = {
//     {PATTERN_BREATHING, 3000, LEDColor::WHITE}
// };

// // Access control indicators
// static constexpr LEDSubState UNKNOWN_UID_TYPE_SEQ[] = {
//     {PATTERN_DOUBLE_BLINK, 600, LEDColor::ORANGE}
// };
// static constexpr LEDSubState ACCESS_GRANTED_SEQ[] = {
//     {PATTERN_TRIPLE_BLINK, 600, LEDColor::GREEN},
//     {PATTERN_SOLID,        1000, LEDColor::GREEN}
// };
// static constexpr LEDSubState ACCESS_DENIED_SEQ[] = {
//     {PATTERN_FIVE_BLINK, 600, LEDColor::RED}
// };
// static constexpr LEDSubState DOOR_UNLOCKED_SEQ[] = {
//     {PATTERN_SOLID, 5000, LEDColor::GREEN}
// };
// static constexpr LEDSubState BRUTE_FORCE_LOCK_SEQ[] = {
//     {PATTERN_SOS, 10000, LEDColor::RED}
// };

// // Error indicators
// static constexpr LEDSubState ERROR_RFID_SEQ[] = {
//     {PATTERN_SOS, 0, LEDColor::RED}
// };
// static constexpr LEDSubState ERROR_AUDIO_SEQ[] = {
//     {PATTERN_FAST_BLINK, 0, LEDColor::PURPLE}
// };
// static constexpr LEDSubState ERROR_WIFI_SEQ[] = {
//     {PATTERN_DOUBLE_BLINK, 0, LEDColor::YELLOW}
// };

// // ------------------------
// // --- Map states ---
// // ------------------------
// constexpr LEDState SYSTEM_READY      = { SYSTEM_READY_SEQ,      1 };
// constexpr LEDState SYSTEM_SCANNING   = { SYSTEM_SCANNING_SEQ,   1 };
// constexpr LEDState WIFI_CONNECTING   = { WIFI_CONNECTING_SEQ,   1 };
// constexpr LEDState WIFI_CONNECTED    = { WIFI_CONNECTED_SEQ,    2 };
// constexpr LEDState WAITING_TIMEOUT   = { WAITING_TIMEOUT_SEQ,   1 };
// constexpr LEDState POWER_ON_FADE     = { POWER_ON_FADE_SEQ,     1 };

// constexpr LEDState UNKNOWN_UID_TYPE   ={ UNKNOWN_UID_TYPE_SEQ,  1 };
// constexpr LEDState ACCESS_GRANTED    = { ACCESS_GRANTED_SEQ,    2 };
// constexpr LEDState ACCESS_DENIED     = { ACCESS_DENIED_SEQ,     1 };
// constexpr LEDState DOOR_UNLOCKED     = { DOOR_UNLOCKED_SEQ,     1 };
// constexpr LEDState BRUTE_FORCE_LOCK  = { BRUTE_FORCE_LOCK_SEQ,  1 };

// constexpr LEDState ERROR_RFID        = { ERROR_RFID_SEQ,        1 };
// constexpr LEDState ERROR_AUDIO       = { ERROR_AUDIO_SEQ,       1 };
// constexpr LEDState ERROR_WIFI        = { ERROR_WIFI_SEQ,        1 };


// // ------------------------
// // --- Helper macros ---
// // ------------------------

// // Enqueue a sequence (queue-based, old method)
// #define LED_SET_SEQ(state) \
//     for(uint8_t i=0; i<(state).length; i++) \
//         led.enqueuePattern((state).sequence[i].pattern, \
//                            (state).sequence[i].duration, \
//                            (state).sequence[i].color)

// // Force start a sequence immediately (new method)
// inline void LED_RUN_SEQ_IMMEDIATE(const LEDState& state) {
//     if (state.length == 0) return;

//     // Start first pattern immediately
//     led.setPattern(state.sequence[0].pattern,
//                    state.sequence[0].duration,
//                    state.sequence[0].color);

//     // Enqueue the rest
//     for (uint8_t i = 1; i < state.length; i++) {
//         led.enqueuePattern(state.sequence[i].pattern,
//                            state.sequence[i].duration,
//                            state.sequence[i].color);
//     }
// }

// #define LED_SET_IMM(state) LED_RUN_SEQ_IMMEDIATE(state)