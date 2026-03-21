#include <Arduino.h>
#include <access_service.h>
#include "esp_log.h"
#include "exception_handler.h"
#include <globals.hpp>
#include <config.hpp>

MasterUIDManager uidManager; // global

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

    // Call the setup function for the access service
    delay(1000);

    // Init EEPROM, Check for Master UIDs
    EEPROM.begin(EEPROM_SIZE);
    uidManager.readUIDs();

    //Testing EEPROM Read
    // MasterUIDManager uidManager;
    // // Read back all stored UIDs
    // uidManager.readUIDs();

    // MasterUIDManager uidManager;


    // uint8_t uid1[4] = {0x12, 0x34, 0x56, 0x78};
    // uint8_t uid2[7] = {0x90, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45};
    // uint8_t* uids[] = {uid1, uid2};
    // uint8_t lengths[] = {sizeof(uid1), sizeof(uid2)};

    // Remove UID1
    // uidManager.removeUID(uid1, sizeof(uid1));
    // uidManager.readUIDs();
    // uidManager.writeUIDs(uids, lengths, 2);
    // uidManager.clearAll();
    // uidManager.readUIDs();

    accessServiceSetup();
}

void loop() {
    // Call the main service loop
    accessServiceLoop();
}
