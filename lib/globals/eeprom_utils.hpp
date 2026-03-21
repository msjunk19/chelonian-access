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