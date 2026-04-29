#pragma once

#include <Arduino.h>
#include <esp_log.h>

#include "config.hpp"
#include "led_states.hpp"
#include "pin_mapping.hpp"
// #include <wifi_auth_endpoints.hpp>
#include "wifi_auth_endpoints_hardened.hpp"


static const char* BTNTAG = "PAIRBUTTON";

class PairingButton {
public:
    void begin(std::function<void()> onPairingHold,
               std::function<void()> onFactoryReset) {
        _onPairingHold  = onPairingHold;
        _onFactoryReset = onFactoryReset;
        pinMode(PAIRING_BUTTON_PIN, INPUT_PULLUP);
        ESP_LOGI(BTNTAG, "Pairing button initialized on GPIO %d", PAIRING_BUTTON_PIN);
    }

    void update() {
        bool pressed = (digitalRead(PAIRING_BUTTON_PIN) == LOW);
        uint32_t now = millis();

        if (pressed) {
            if (!_wasPressed) {
                _pressStart  = now;
                _wasPressed  = true;
                _resetFired  = false;
                ESP_LOGV(BTNTAG, "Button pressed");
            } else {
                uint32_t heldMs = now - _pressStart;

                // Check factory reset first (10s takes priority)
                if (!_resetFired && heldMs >= FACTORY_RESET_HOLD_MS) {
                    _resetFired = true;
                    ESP_LOGW(BTNTAG, "Factory reset hold detected (%lums)", heldMs);
                    _onFactoryReset();
                }
                // Then pairing (3s)
                else if (heldMs >= PAIRING_HOLD_MS) {
                    ESP_LOGI(BTNTAG, "Pairing triggered on hold (%lums)", heldMs);
                    _onPairingHold();
                    LED_SET_SEQ(SYSTEM_PAIR);
                }
            }
        } else {
            if (_wasPressed) {
                uint32_t heldMs = now - _pressStart;
                ESP_LOGV(BTNTAG, "Button released after %lums", heldMs);
            }
            _wasPressed = false;
            _resetFired = false;
        }
    }

private:
    bool     _wasPressed = false;
    bool     _resetFired = false;
    uint32_t _pressStart = 0;

    std::function<void()> _onPairingHold;
    std::function<void()> _onFactoryReset;
};