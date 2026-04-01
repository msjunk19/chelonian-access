#pragma once
#include <relay_controller.h>
#include <macro_config.hpp>
#include "esp_log.h"

static const char* EXECUTORTAG = "MACRO_EXEC";

extern RelayController relays;
extern MacroConfigManager macroConfigManager;

// Forward declarations — state lives in accessservice
enum RelayState;
struct AccessLoopState;
extern AccessLoopState state;

// Fire all relays in a step's bitmask
static inline void _fireStep(const RelayStep& step) {
    for (uint8_t i = 0; i < 4; i++) {
        if (step.relay_mask & (1 << i)) {
            relays.setRelay(i, true);
        }
    }
}

// Turn off all relays in a step's bitmask
static inline void _clearStep(const RelayStep& step) {
    for (uint8_t i = 0; i < 4; i++) {
        if (step.relay_mask & (1 << i)) {
            relays.setRelay(i, false);
        }
    }
}

// Trigger a macro by index
static inline void triggerMacro(AccessLoopState& state, uint8_t macroIndex) {
    if (state.relayActive) {
        ESP_LOGW(EXECUTORTAG, "Relay already active, ignoring trigger");
        return;
    }

    if (macroIndex >= macroConfigManager.config.macro_count) {
        ESP_LOGE(EXECUTORTAG, "Invalid macro index: %u", macroIndex);
        return;
    }

    Macro& m = macroConfigManager.config.macros[macroIndex];

    if (m.step_count == 0) {
        ESP_LOGW(EXECUTORTAG, "Macro %u has no steps", macroIndex);
        return;
    }

    // Copy macro into state so config changes mid-sequence are safe
    state.activeMacro  = m;
    state.currentStep  = 0;
    state.relayActive  = true;

    // Fire step 0 immediately
    _fireStep(state.activeMacro.steps[0]);
    state.relayActivatedTime = millis();
    state.currentRelayState  = RELAY_STEP_ACTIVE;

    ESP_LOGI(EXECUTORTAG, "Macro started: %s (%u steps)", m.name, m.step_count);
}

// Call this from accessServiceLoop every iteration
static inline void handleRelaySequence(AccessLoopState& state) {
    if (state.currentRelayState == RELAY_IDLE) return;

    RelayStep& step = state.activeMacro.steps[state.currentStep];

    switch (state.currentRelayState) {

        case RELAY_STEP_ACTIVE:
            if (millis() - state.relayActivatedTime >= step.duration) {
                _clearStep(step);

                if (state.currentStep + 1 < state.activeMacro.step_count) {
                    // more steps — start gap
                    state.relayActivatedTime = millis();
                    state.currentRelayState  = RELAY_STEP_GAP;
                    ESP_LOGI(EXECUTORTAG, "Step %u complete — gap started", state.currentStep);
                } else {
                    // sequence complete
                    state.currentRelayState = RELAY_IDLE;
                    state.relayActive       = false;
                    ESP_LOGI(EXECUTORTAG, "Macro complete: %s", state.activeMacro.name);
                }
            }
            break;

        case RELAY_STEP_GAP:
            if (millis() - state.relayActivatedTime >= step.gap) {
                state.currentStep++;
                _fireStep(state.activeMacro.steps[state.currentStep]);
                state.relayActivatedTime = millis();
                state.currentRelayState  = RELAY_STEP_ACTIVE;
                ESP_LOGI(EXECUTORTAG, "Step %u started", state.currentStep);
            }
            break;

        default:
            ESP_LOGE(EXECUTORTAG, "Unhandled relay state: %d", state.currentRelayState);
            state.currentRelayState = RELAY_IDLE;
            state.relayActive       = false;
            break;
    }
}