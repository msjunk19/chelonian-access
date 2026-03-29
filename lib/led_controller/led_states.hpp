#pragma once
#include "led_controller.h"

// LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10

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
constexpr LEDState SYSTEM_PAIR       = { PATTERN_FAST_BLINK, 60000, LEDColor::BLUE };
constexpr LEDState FACTORY_RESET     = { PATTERN_FAST_BLINK, 5000, LEDColor::RED };


// constexpr LEDState SYSTEM_IDLE       = { PATTERN_FAST_BLINK, 0, LEDColor::AMBER };

// constexpr LEDState SYSTEM_SCANNING   = { PATTERN_FAST_BLINK, 0, LEDColor::BLUE };
// constexpr LEDState WIFI_CONNECTING   = { PATTERN_DOUBLE_BLINK, 0, LEDColor::YELLOW };
// constexpr LEDState WIFI_CONNECTED    = { PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN };
constexpr LEDState WAITING_TIMEOUT   = { PATTERN_SLOW_BLINK, 0, LEDColor::ORANGE };
// constexpr LEDState POWER_ON_FADE     = { PATTERN_BREATHING, 3000, LEDColor::WHITE };
constexpr LEDState POWERING_ON       = { PATTERN_SLOW_BLINK, 3000, LEDColor::BLUE };
constexpr LEDState UNKNOWN_UID_TYPE  = { PATTERN_DOUBLE_BLINK, 0, LEDColor::ORANGE };

// Access Control Indicators
constexpr LEDState MASTER_CARD       = { PATTERN_FAST_BLINK, 0, LEDColor::PURPLE };
constexpr LEDState ACCESS_GRANTED    = { PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN };
// constexpr LEDState ACCESS_DENIED     = { PATTERN_FIVE_BLINK, 0, LEDColor::RED };

constexpr LEDState ACCESS_DENIED     = { PATTERN_FIVE_BLINK, 0, LEDColor::RED };

constexpr LEDState UNLOCK            = { PATTERN_SLOW_BLINK, 2000, LEDColor::GREEN };


// constexpr LEDState DOOR_UNLOCKED     = { PATTERN_SOLID, 5000, LEDColor::GREEN };

// constexpr LEDState BRUTE_FORCE_LOCK  = { PATTERN_SOS, 0, LEDColor::RED };

// Error Indicators
// constexpr LEDState ERROR_RFID        = { PATTERN_SOS, 0, LEDColor::RED };
// constexpr LEDState ERROR_AUDIO       = { PATTERN_FAST_BLINK, 0, LEDColor::PURPLE };
// constexpr LEDState ERROR_WIFI        = { PATTERN_DOUBLE_BLINK, 0, LEDColor::YELLOW };

// Card Programming
constexpr LEDState PROGRAMMING_MODE  = { PATTERN_FIVE_BLINK, 0, LEDColor::PURPLE };
constexpr LEDState MASTER_CARD_SET   = { PATTERN_SOLID, 2000, LEDColor::PURPLE };

constexpr LEDState USER_ADDED        = { PATTERN_TRIPLE_BLINK, 0, LEDColor::GREEN };
constexpr LEDState USER_REMOVED      = { PATTERN_FIVE_BLINK, 0, LEDColor::RED };

constexpr LEDState PLACEHOLDER        = { PATTERN_SOLID, 2000, LEDColor::YELLOW };


// --- Optional helper macro for cleaner syntax ---
#define LED_SET_SEQ(state) led.setPattern((state).pattern, (state).duration, (state).color)
#define LED_ENQUEUE(state) led.enqueuePattern((state).pattern, (state).duration, (state).color)
#define LED_CANCEL() led.cancel()