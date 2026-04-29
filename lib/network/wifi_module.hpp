#pragma once

/**
 * @file wifi_module.hpp
 * @brief WiFi AP and web server module
 * 
 * This module encapsulates all WiFi/AP functionality.
 * Include this header and call the functions to enable WiFi features.
 */

#include "features.hpp"

#ifdef ENABLE_WIFI_AP

#include "globals.hpp"

#include "led_states.hpp"
#include "wifi_manager.hpp"
#include "access_service.h"
#include "command_handler.hpp"

class WiFiModule {
public:
    static WiFiModule& getInstance() {
        static WiFiModule instance;
        return instance;
    }

    /**
     * @brief Initialize WiFi AP and web server
     * Must be called during setup()
     */
    void begin() {
        ESP_LOGI(TAG, "Initializing WiFi AP...");

        // Mount LittleFS BEFORE setting up routes
        if (!safeLittleFSBegin()) {
            ESP_LOGE(TAG, "LittleFS mount failed! HTML pages will not be available.");
        }

        setupWebServer([this](PhoneCommand cmd) {
            this->handleWebCommand(cmd);
        });

        // Start the AP
        startAP();

        ESP_LOGI(TAG, "WiFi AP initialized");
    }

    /**
     * @brief Handle WiFi-based commands
     */
    void handleWebCommand(PhoneCommand cmd) {
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
            case PhoneCommand::STATUS:
            default:
                break;
        }
    }

    /**
     * @brief Call from main loop to handle clients
     */
    void update() {
        handleClient();
    }

private:
    WiFiModule() = default;
    static constexpr const char* TAG = "WiFiModule";
};

#else  // !ENABLE_WIFI_AP

// Stub implementation when WiFi is disabled
class WiFiModule {
public:
    static WiFiModule& getInstance() {
        static WiFiModule instance;
        return instance;
    }

    void begin() {
        ESP_LOGI(TAG, "WiFi AP disabled at build time");
    }

    void update() {
        // No-op
    }

private:
    WiFiModule() = default;
    static constexpr const char* TAG = "WiFiModule";
};

#endif  // ENABLE_WIFI_AP