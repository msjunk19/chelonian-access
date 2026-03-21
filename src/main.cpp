#include <Arduino.h>
#include <access_service.h>
#include "esp_log.h"
#include "exception_handler.h"
#include <globals.hpp>
#include <config.hpp>
#include <eeprom_utils.hpp>

// MasterUIDManager uidManager; // global
MasterUIDManager masterUidManager; //global updated

static const char* TAG = "Main";

void setup() {
    setupGlobalExceptionHandler();

    Serial.begin(115200);
    delay(1000);

    ESP_LOGE(TAG, "Chelonian Access Service");
    ESP_LOGE(TAG, "Version 1.0.0");
    ESP_LOGE(TAG, "Copyright (C) 2023 Derek Molloy");
    ESP_LOGE(TAG, "Licensed under the MIT License");
    ESP_LOGE(TAG, "Starting up!");

    delay(1000);

    // Init EEPROM, Check for Master UIDs
    EEPROM.begin(EEPROM_SIZE);

    // clearEntireEEPROM();
    // while (true); // stop here so it doesn't run normal code
    


    masterUidManager.readUIDs();

    accessServiceSetup();

}

void loop() {
    // Call the main service loop
    accessServiceLoop();
}
