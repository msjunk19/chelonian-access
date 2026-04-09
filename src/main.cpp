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
#include <access_log.hpp>

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

RFIDController rfid;
RelayController relays;
AudioContoller audio;

MacroConfigManager macroConfigManager;
AccessLogger accessLogger;

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

        // phoneTokenManager.clearAll();
        // while(true);

    masterUidManager.readUIDs();
    userUidManager.readUIDs();
    phoneTokenManager.readPhones();

    authManager.restoreTime();
    accessLogger.begin();
    ESP_LOGI(TAG, "Device boot completed");

    // accessServiceSetup();   

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
        case PhoneCommand::UNLOCK: {
            LED_SET_SEQ(UNLOCK);
        //     fireMacro(macroConfigManager.config.tag_macro);
            int8_t idx = macroConfigManager.findByName("Unlock");
            if (idx >= 0) fireMacro(idx);
            break;
        }
        case PhoneCommand::LOCK: {
            LED_SET_SEQ(LOCK);
            int8_t idx = macroConfigManager.findByName("Lock");
            if (idx >= 0) fireMacro(idx);
            break;
        }
        case PhoneCommand::STATUS:
            break;
        case PhoneCommand::TRUNK: {
            LED_SET_SEQ(TRUNK);
            int8_t idx = macroConfigManager.findByName("Trunk");
            if (idx >= 0) fireMacro(idx);
            break;
        }
        case PhoneCommand::PANIC: {
            LED_SET_SEQ(PANIC);
            int8_t idx = macroConfigManager.findByName("Panic");
            if (idx >= 0) fireMacro(idx);
            break;
        }
        default:
            break;
    }
});

    startAP();

    bleManager.begin([](PhoneCommand cmd) {
    switch (cmd) {
        // case PhoneCommand::UNLOCK: 
        //     LED_SET_SEQ(UNLOCK); 
        //     fireMacro(macroConfigManager.config.tag_macro);
        //     accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Unlock command");
        //     break;
        case PhoneCommand::UNLOCK: {
            LED_SET_SEQ(UNLOCK);
            int8_t idx = macroConfigManager.findByName("UNlock");
            if (idx >= 0) fireMacro(idx);
            accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Unlock command");
            break;
        }        
        case PhoneCommand::LOCK: {
            LED_SET_SEQ(LOCK);
            int8_t idx = macroConfigManager.findByName("Lock");
            if (idx >= 0) fireMacro(idx);
            accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Lock command");
            break;
        }
        case PhoneCommand::STATUS: break;
        case PhoneCommand::TRUNK: {
            LED_SET_SEQ(TRUNK);
            int8_t idx = macroConfigManager.findByName("Trunk");
            if (idx >= 0) fireMacro(idx);
            accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Trunk command");
            break;
        }
        case PhoneCommand::PANIC: {
            LED_SET_SEQ(PANIC);
            int8_t idx = macroConfigManager.findByName("Panic");
            if (idx >= 0) fireMacro(idx);
            accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Panic command");
            break;
        }
        default: break;
        }
    });
    
    accessServiceSetup();   



}

void loop() {
    // Call the main service loop
    accessServiceLoop();
    pairingButton.update();

    handleClient();
    bleManager.update();
    bleManager.updatePairingWindow();
}
