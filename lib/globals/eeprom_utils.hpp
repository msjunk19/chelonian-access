#pragma once
#include <EEPROM.h>
#include <config.hpp>

static void clearEntireEEPROM() {
    for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();

    Serial.println("FULL EEPROM CLEARED");
}


inline void clearAPCredentials() {
    // Clear magic byte
    EEPROM.write(AP_MAGIC_ADDR, 0xFF);

    // Clear SSID (32 bytes)
    for (uint16_t i = 0; i < 32; i++) {
        EEPROM.write(AP_SSID_ADDR + i, 0xFF);
    }

    // Clear password (32 bytes)
    for (uint16_t i = 0; i < 32; i++) {
        EEPROM.write(AP_PASS_ADDR + i, 0xFF);
    }

    EEPROM.commit();
    ESP_LOGI("AP_SETUP", "AP credentials cleared");
}
