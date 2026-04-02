#include <Arduino.h>
#include <access_service.h>
#include "esp_log.h"
#include "exception_handler.h"
#include <globals.hpp>
#include <config.hpp>
#include <eeprom_utils.hpp>
// #include <setup_ap.h>
#include <wifi_manager.hpp>
// #include <webserver_manager.h>
#include <pairing_button.hpp>
#include <auth_manager.hpp>
#include <ble_manager.hpp>
#include <factory_reset.hpp>
#include <macro_config.hpp>

// LED Selection, only use one. 
// LEDController led(PN_LED); //Single Color LED on pin 8
LEDController led(0, true, PN_NEOPIXEL);  // definition lives here

// Instances — defined once here, extern'd everywhere else
MasterUIDManager masterUidManager; //global updated
UserUIDManager userUidManager; 

PhoneTokenManager phoneTokenManager;
AuthManager authManager(phoneTokenManager);

PairingButton pairingButton;
BLEManager bleManager;


MacroConfigManager macroConfigManager;

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

    // phoneTokenManager.clearAll();
    // masterUidManager.clearMasters();
    // while(true);

    masterUidManager.readUIDs();
    userUidManager.readUIDs();
    phoneTokenManager.readPhones();

    accessServiceSetup();   

    pairingButton.begin(
    []() {
        openPairingWindow();
        bleManager.openPairingWindow();
    },
    []() {
        // ESP_LOGW("FACTORY", "Factory reset triggered — not yet implemented");
        LED_CANCEL();
        factoryReset();
    }
);
setupWebServer([](PhoneCommand cmd) {
    switch (cmd) {
        case PhoneCommand::UNLOCK:
            LED_SET_SEQ(UNLOCK);
            fireMacro(macroConfigManager.config.tag_macro);
            break;
        case PhoneCommand::LOCK:
            break;
        case PhoneCommand::STATUS:
            break;
        default:
            break;
    }
});

    startAP();
    // ESP_LOGI("WiFi AP", "Setup complete. AP running.");

    bleManager.begin([](PhoneCommand cmd) {
    switch (cmd) {
        // case PhoneCommand::UNLOCK: LED_SET_SEQ(UNLOCK); activateRelays(); break;
        case PhoneCommand::UNLOCK: LED_SET_SEQ(UNLOCK); fireMacro(macroConfigManager.config.tag_macro); break;
        
        case PhoneCommand::LOCK:   break;
        case PhoneCommand::STATUS: break;
        default: break;
        }
    });



}

void loop() {
    // Call the main service loop
    accessServiceLoop();
    pairingButton.update();

    handleClient();
    bleManager.update();
    bleManager.updatePairingWindow();
}
