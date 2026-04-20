#include <Arduino.h>
#include <access_service.h>
#include "esp_log.h"
#include "exception_handler.h"
#include <globals.hpp>
#include <config.hpp>
#include <eeprom_utils.hpp>
#include <wifi_manager.hpp>
#include "wifi_auth_endpoints_hardened.hpp"

#include <pairing_button.hpp>
#include <auth_manager.hpp>
#include <ble_manager.hpp>
#include <factory_reset.hpp>
#include <macro_config.hpp>
#include <access_log.hpp>
#include "esp_system.h"
#include <boot_logger.hpp>

#include "rate_limiter.hpp"
#include "encrypted_token_storage.hpp"
#include "nonce_manager.hpp"

// LED Selection, only use one. 
// LEDController led(PN_LED); //Single Color LED on pin 8
LEDController led(0, true, PN_NEOPIXEL);  // definition lives here

// Global security instances
RateLimiter rateLimiter;
NonceManager nonceManager;
EncryptedTokenStorage encryptedStorage;

// Instances — defined once here, extern'd everywhere else
MasterUIDManager masterUidManager;
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

void debugTime()
{
    time_t now = time(nullptr);

    Serial.println("---- TIME DEBUG ----");
    Serial.print("millis(): ");
    Serial.println(millis());

    Serial.print("time(nullptr): ");
    Serial.println((uint32_t)now);

    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo))
    {
        Serial.printf("Readable: %04d-%02d-%02d %02d:%02d:%02d\n",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
    }
    else
    {
        Serial.println("localtime failed");
    }

    Serial.println("--------------------");
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    /* NEW: Initialize encrypted storage */
    if (!encryptedStorage.begin()) {
        ESP_LOGE("MAIN", "Failed to initialize encrypted storage!");
        delay(1000);
        ESP.restart();
    }

    /* NEW: Initialize nonce manager */
    nonceManager.clear();  // Optional: start fresh

    ESP_LOGI("MAIN", "Nonce manager initialized");

    /* NEW: Initialize rate limiter */
    ESP_LOGI("MAIN", "Rate limiter initialized");

    accessLogger.begin();
    logBootReason();


    setupGlobalExceptionHandler();

    ESP_LOGI(TAG, "Chelonian Access Service starting");

    ESP_LOGI("BOOT", "Reset reason: %d", esp_reset_reason());

    /* ---------------- LOAD STORED DATA ---------------- */

    masterUidManager.readUIDs();
    userUidManager.readUIDs();
    phoneTokenManager.readPhones();
    
    // phoneTokenManager.clearAll();


    /* ---------------- PAIRING BUTTON ---------------- */

    pairingButton.begin(
        []()
        {
            openPairingWindow();
            bleManager.openPairingWindow();
        },
        []()
        {
            LED_CANCEL();
            factoryReset();
        }
    );

    /* ---------------- WEB SERVER & AUTH ENDPOINTS ---------------- */

    // Mount LittleFS BEFORE setting up routes
    if (!safeLittleFSBegin()) {
        ESP_LOGE(TAG, "LittleFS mount failed! HTML pages will not be available.");
    }

    setupWebServer([](PhoneCommand cmd)
    {
        switch (cmd)
        {
            case PhoneCommand::UNLOCK:
            {
                LED_SET_SEQ(UNLOCK);
                int8_t idx = macroConfigManager.findByName("Unlock");
                if (idx >= 0) fireMacro(idx);
                break;
            }

            case PhoneCommand::LOCK:
            {
                LED_SET_SEQ(LOCK);
                int8_t idx = macroConfigManager.findByName("Lock");
                if (idx >= 0) fireMacro(idx);
                break;
            }

            case PhoneCommand::TRUNK:
            {
                LED_SET_SEQ(TRUNK);
                int8_t idx = macroConfigManager.findByName("Trunk");
                if (idx >= 0) fireMacro(idx);
                break;
            }

            case PhoneCommand::PANIC:
            {
                LED_SET_SEQ(PANIC);
                int8_t idx = macroConfigManager.findByName("Panic");
                if (idx >= 0) fireMacro(idx);
                break;
            }

            case PhoneCommand::STATUS:
            default:
                break;
        }
    });

    /* ---------------- NETWORK ---------------- */

    startAP();

    /* ---------------- BLE ---------------- */

    bleManager.begin([](PhoneCommand cmd)
    {
        switch (cmd)
        {
            case PhoneCommand::UNLOCK:
            {
                LED_SET_SEQ(UNLOCK);
                int8_t idx = macroConfigManager.findByName("Unlock");
                if (idx >= 0) fireMacro(idx);

                accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Unlock command");
                break;
            }

            case PhoneCommand::LOCK:
            {
                LED_SET_SEQ(LOCK);
                int8_t idx = macroConfigManager.findByName("Lock");
                if (idx >= 0) fireMacro(idx);

                accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Lock command");
                break;
            }

            case PhoneCommand::TRUNK:
            {
                LED_SET_SEQ(TRUNK);
                int8_t idx = macroConfigManager.findByName("Trunk");
                if (idx >= 0) fireMacro(idx);

                accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Trunk command");
                break;
            }

            case PhoneCommand::PANIC:
            {
                LED_SET_SEQ(PANIC);
                int8_t idx = macroConfigManager.findByName("Panic");
                if (idx >= 0) fireMacro(idx);

                accessLogger.logAccess(LogSource::BLE, LogResult::SUCCESS, "BLE", "Panic command");
                break;
            }

            default:
                break;
        }
    });

    /* ---------------- ACCESS SERVICE ---------------- */
    delay(2000);
    accessServiceSetup();

    // ESP_LOGI(TAG, "System boot complete");
}

void loop() {
    accessServiceLoop();
    pairingButton.update();

    handleClient();
    bleManager.update();
    bleManager.updatePairingWindow();
}