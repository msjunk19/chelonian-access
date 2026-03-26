#include <Arduino.h>
#include <access_service.h>
#include "esp_log.h"
#include "exception_handler.h"
#include <globals.hpp>
#include <config.hpp>
#include <eeprom_utils.hpp>
#include <setup_ap.h>
#include "webserver_manager.h"

MasterUIDManager masterUidManager; //global updated
UserUIDManager userUidManager; 

static const char* TAG = "Main";

void setup() {
    setupGlobalExceptionHandler();

    Serial.begin(115200);
    delay(1000);

    ESP_LOGV(TAG, "Chelonian Access Service");
    ESP_LOGV(TAG, "Version 1.0.0");
    ESP_LOGV(TAG, "Copyright (C) 2023 Derek Molloy");
    ESP_LOGV(TAG, "Licensed under the MIT License");
    ESP_LOGV(TAG, "Starting up!");

    ESP_LOGE("TEST", "ERROR");
    ESP_LOGW("TEST", "WARN");
    ESP_LOGI("TEST", "INFO");
    ESP_LOGD("TEST", "DEBUG");
    ESP_LOGV("TEST", "VERBOSE");

    delay(1000);

    // masterUidManager.clearMasters();
    // while(true);

    startAP();


    // Setup web server routes (for first-time configuration)
    setupWebServer();
    
    Serial.println("Setup complete. AP running.");


    masterUidManager.readUIDs();
    userUidManager.readUIDs();

    accessServiceSetup();

}

void loop() {
    // Call the main service loop
    accessServiceLoop();
    handleClient();
}
