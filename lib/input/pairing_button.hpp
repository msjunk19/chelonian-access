#pragma once
#include <Arduino.h>
#include <esp_log.h>
#include <led_states.hpp>
#include <wifi_auth_endpoints.hpp>
#include <config.hpp>
#include <pin_mapping.hpp>

static const char* BTNTAG = "PAIRBUTTON";

// static constexpr uint8_t  PAIRING_BUTTON_PIN    = 9;
// static constexpr uint32_t PAIRING_HOLD_MS       = 3000;
// static constexpr uint32_t FACTORY_RESET_HOLD_MS = 10000;

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
                    ESP_LOGI(BTNTAG, "Pairing triggered on hold");
                    _onPairingHold();
                    LED_SET_SEQ(SYSTEM_PAIR);
                }
            }
        } else {
            if (_wasPressed) {
                uint32_t heldMs = now - _pressStart;
                ESP_LOGV(BTNTAG, "Button released after %lums", heldMs);
                // if (_onPairingHold){

                // }
                // else if (_resetFired) {
                //     // Factory reset already fired — do nothing on release
                //     // ESP_LOGW(BTNTAG, "Factory reset release ignored");
                // } 
                // // else if (heldMs >= PAIRING_HOLD_MS) {
                // //     // Released in pairing window — open pairing
                // //     ESP_LOGI(BTNTAG, "Pairing triggered on release");
                // //     _onPairingHold();
                // //     LED_SET_SEQ(SYSTEM_PAIR);
                // // }
                // // Released before 3s — do nothing
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