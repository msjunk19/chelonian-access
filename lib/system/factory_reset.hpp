#pragma once
#include <Arduino.h>
#include "esp_log.h"
#include <led_states.hpp>
#include <globals.hpp>

static const char* RSTAG = "FACTORY";

inline void factoryReset() {
    ESP_LOGW(RSTAG, "Factory reset initiated — wiping all stored data");

    // Visual feedback — SOS pattern so user knows reset is happening
    LED_SET_SEQ(FACTORY_RESET);

    // Wipe everything
    masterUidManager.clearMasters();
    ESP_LOGI(RSTAG, "Master UIDs cleared");

    userUidManager.clearUsers();
    ESP_LOGI(RSTAG, "User UIDs cleared");

    phoneTokenManager.clearAll();
    ESP_LOGI(RSTAG, "Paired phones cleared");

    clearAPCredentials();
    ESP_LOGI(RSTAG, "WiFi credentials cleared");

    macroConfigManager.clear();
    ESP_LOGI(RSTAG, "Macro configuration cleared");

    ESP_LOGW(RSTAG, "Factory reset complete — rebooting");

    ESP.restart();
}