#pragma once
#include <Arduino.h>
#include "esp_log.h"
#include <led_states.hpp>
#include <master_uid_manager.h>
#include <user_uid_manager.h>
#include <phone_token_manager.hpp>
#include <wifi_manager.hpp>

static const char* RSTAG = "FACTORY";

extern MasterUIDManager masterUidManager;
extern UserUIDManager userUidManager;
extern PhoneTokenManager phoneTokenManager;

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

    relayConfigManager.clear();
    ESP_LOGI(RSTAG, "Relay configuration cleared");

    ESP_LOGW(RSTAG, "Factory reset complete — rebooting");
    // delay(2000); // let LED sequence play briefly
    ESP.restart();
}