#pragma once

/**
 * @file ble_module.hpp
 * @brief Bluetooth Low Energy module
 * 
 * This module encapsulates all BLE functionality.
 * Include this header and call the functions to enable BLE features.
 */

#include "features.hpp"

#ifdef ENABLE_BLE
#include "globals.hpp"
#include "led_states.hpp"
#include "ble_manager.hpp"
#include "macro_executor.hpp"
#include "command_handler.hpp"

BLEManager bleManager;


class BLEModule {
public:
    static BLEModule& getInstance() {
        static BLEModule instance;
        return instance;
    }

    /**
     * @brief Initialize BLE and start advertising
     * Must be called during setup()
     */
    void begin() {
        ESP_LOGI(TAG, "Initializing BLE...");

        bleManager.begin([this](PhoneCommand cmd) {
            this->handleBleCommand(cmd);
        });

        ESP_LOGI(TAG, "BLE initialized");
    }

    /**
     * @brief Handle BLE-based commands
     */
    void handleBleCommand(PhoneCommand cmd) {
        switch (cmd) {
            case PhoneCommand::UNLOCK: {
                LED_SET_SEQ(UNLOCK);
                int8_t idx = macroConfigManager.findByName("Unlock");
                if (idx >= 0) fireMacro(idx);
                break;
            }
            case PhoneCommand::LOCK: {
                LED_SET_SEQ(LOCK);
                int8_t idx = macroConfigManager.findByName("Lock");
                if (idx >= 0) fireMacro(idx);
                break;
            }
            case PhoneCommand::TRUNK: {
                LED_SET_SEQ(TRUNK);
                int8_t idx = macroConfigManager.findByName("Trunk");
                if (idx >= 0) fireMacro(idx);
                break;
            }
            case PhoneCommand::PANIC: {
                LED_SET_SEQ(PANIC);
                int8_t idx = macroConfigManager.findByName("Panic");
                if (idx >= 0) fireMacro(idx);
                break;
            }
            default:
                break;
        }
    }

    /**
     * @brief Call from main loop to process BLE events
     */
    void update() {
        bleManager.update();
    }

    /**
     * @brief Call from main loop to update pairing window
     */
    void updatePairingWindow() {
        bleManager.updatePairingWindow();
    }

private:
    BLEModule() = default;
    static constexpr const char* TAG = "BLEModule";
};

#else  // !ENABLE_BLE

// Stub implementation when BLE is disabled
class BLEModule {
public:
    static BLEModule& getInstance() {
        static BLEModule instance;
        return instance;
    }

    void begin() {
        ESP_LOGI(TAG, "BLE disabled at build time");
    }

    void update() {
        // No-op
    }

    void updatePairingWindow() {
        // No-op
    }

private:
    BLEModule() = default;
    static constexpr const char* TAG = "BLEModule";
};

#endif  // ENABLE_BLE