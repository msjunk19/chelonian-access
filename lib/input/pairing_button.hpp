#pragma once
#include <Arduino.h>
#include <esp_log.h>
#include <led_states.hpp>
#include <wifi_auth_endpoints.hpp>
// #include <led_states.hpp>

static const char* BTNTAG = "PAIRBUTTON";


static constexpr uint8_t  PAIRING_BUTTON_PIN   = 9;
static constexpr uint32_t PAIRING_HOLD_MS       = 3000;  // 3s hold to open pairing
// static constexpr uint32_t FACTORY_RESET_HOLD_MS = 10000; // 10s hold — TODO
// LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10 . this needs to move to a global

class PairingButton {
public:
    void begin(std::function<void()> onPairingHold) {
        _onPairingHold = onPairingHold;
        pinMode(PAIRING_BUTTON_PIN, INPUT_PULLUP);
        ESP_LOGI(BTNTAG, "Pairing button initialized on GPIO %d", PAIRING_BUTTON_PIN);
    }

    // Call this every loop()
    void update() {
        bool pressed = (digitalRead(PAIRING_BUTTON_PIN) == LOW);
        uint32_t now = millis();

        if (pressed) {
            if (!_wasPressed) {
                // Button just went down — record press start
                _pressStart = now;
                _wasPressed = true;
                _actionFired = false;
                ESP_LOGV(BTNTAG, "Button pressed");
            } else {
                // Button held — check thresholds in descending order
                uint32_t heldMs = now - _pressStart;

                // TODO: factory reset placeholder
                // if (!_actionFired && heldMs >= FACTORY_RESET_HOLD_MS) {
                //     _actionFired = true;
                //     ESP_LOGW(BTNTAG, "Factory reset hold detected — not yet implemented");
                // }

            if (!_actionFired && heldMs >= PAIRING_HOLD_MS) {
                _actionFired = true;
                ESP_LOGI(BTNTAG, "Pairing hold detected (%lums)", heldMs);
                openPairingWindow();      // WiFi pairing
                // bleManager.openPairingWindow(); // BLE pairing
                _onPairingHold();
            }
            }
        } else {
            if (_wasPressed) {
                ESP_LOGV(BTNTAG, "Button released after %lums", now - _pressStart);
            }
            _wasPressed   = false;
            _actionFired  = false;
        }
    }

private:
    bool     _wasPressed  = false;
    bool     _actionFired = false;
    uint32_t _pressStart  = 0;
    std::function<void()> _onPairingHold;
};