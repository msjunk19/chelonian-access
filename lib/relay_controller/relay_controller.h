#pragma once

#include <config.hpp>
#include <pin_mapping.hpp>
#include <Arduino.h>
#include <array>

// const unsigned long RELAY_COUNT = 4;

class RelayController {
public:
    RelayController(uint8_t relay1_pin = RELAY_1, uint8_t relay2_pin = RELAY_2, uint8_t relay3_pin = RELAY_3,
                    uint8_t relay4_pin = RELAY_4);
    void begin();
    void setRelay(uint8_t relay, bool state);
    void setAllRelays(bool state);
    bool getRelayState(uint8_t relay) const;

private:
    struct RelayPin {
        uint8_t pin;
        bool state;
    };

    static constexpr uint8_t NUM_RELAYS = RELAY_COUNT;
    std::array<RelayPin, NUM_RELAYS> m_relays{};

    static bool isValidRelay(uint8_t relay);
};
