#include <Arduino.h>
#include <access_service.h>
#include "esp_log.h"
#include "exception_handler.h"
#include <globals.hpp>
#include <config.hpp>
#include <eeprom_utils.hpp>
// #include <setup_ap.h>
#include <wifi_manager.hpp>
#include "webserver_manager.h"
#include <pairing_button.hpp>

// Instances — defined once here, extern'd everywhere else

MasterUIDManager masterUidManager; //global updated
UserUIDManager userUidManager; 

PhoneTokenManager phoneTokenManager;
AuthManager authManager(phoneTokenManager);

PairingButton pairingButton;


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

    pairingButton.begin();

    startAP();


    // Setup web server routes (for first-time configuration)
    // setupWebServer();
    setupWebServer([](PhoneCommand cmd) {
    switch (cmd) {
        case PhoneCommand::UNLOCK:
            // your relay unlock call
            break;
        case PhoneCommand::LOCK:
            // your relay lock call
            break;
        case PhoneCommand::STATUS:
            break;
        default:
            break;
    }
});
    
    Serial.println("Setup complete. AP running.");


    masterUidManager.readUIDs();
    userUidManager.readUIDs();

    accessServiceSetup();


        
    phoneTokenManager.readPhones();
    
    // openPairingWindow(); // TEMP — remove after testing

}

void loop() {
    // Call the main service loop
    pairingButton.update();
    accessServiceLoop();
    handleClient();
}
