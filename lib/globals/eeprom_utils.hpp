#pragma once
#include <EEPROM.h>
// #include <config.hpp>

static void clearEntireEEPROM() {
    EEPROM.begin(512);
    for (int i = 0; i < 512; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared!");
}