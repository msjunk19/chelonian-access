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

#include <factory_reset.hpp>
#include <macro_config.hpp>
#include <access_log.hpp>
#include "esp_system.h"
#include <boot_logger.hpp>

#include "rate_limiter.hpp"
#include "encrypted_token_storage.hpp"
#include "nonce_manager.hpp"

#include <usb_macro_mode.hpp>
#include <usb_handler.hpp>

// ========== NEW: Feature toggles and modules ==========
#include "features.hpp"
#include "command_handler.hpp"
#include "wifi_module.hpp"
#include "ble_module.hpp"
// ====================================================

enum class BootMode {
    NORMAL,
    USB_CONFIG
};

BootMode bootMode = BootMode::NORMAL;

// LED Selection, only use one. 
LEDController led(0, true, PN_NEOPIXEL);

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

RFIDController rfid;
RelayController relays;
AudioContoller audio;

AccessLogger accessLogger;

static const char* TAG = "Main";

// ========== NEW: Module instances ==========
WiFiModule& wifiModule = WiFiModule::getInstance();
BLEModule& bleModule = BLEModule::getInstance();
// ==========================================

// void setupUSBMode() {
//     ESP_LOGI(TAG, "USB Configuration Mode initialized");
//     ESP_LOGI(TAG, "Waiting for NVS configuration commands...");
// }
 
// void loopUSBMode() {
//     // TODO: Handle incoming serial commands for NVS editing
//     delay(100);
// }   

// void debugTime()
// {
//     time_t now = time(nullptr);

//     Serial.println("---- TIME DEBUG ----");
//     Serial.print("millis(): ");
//     Serial.println(millis());

//     Serial.print("time(nullptr): ");
//     Serial.println((uint32_t)now);

//     struct tm timeinfo;
//     if (localtime_r(&now, &timeinfo))
//     {
//         Serial.printf("Readable: %04d-%02d-%02d %02d:%02d:%02d\n",
//             timeinfo.tm_year + 1900,
//             timeinfo.tm_mon + 1,
//             timeinfo.tm_mday,
//             timeinfo.tm_hour,
//             timeinfo.tm_min,
//             timeinfo.tm_sec);
//     }
//     else
//     {
//         Serial.println("localtime failed");
//     }

//     Serial.println("--------------------");
// }

void setup()
{
    Serial.begin(115200);
    Serial.setRxBufferSize(1024);  // Increase from default 256 to 1024
    delay(500);

    macroConfigManager.load();
    delay(1000);

    // bootMode = BootMode::USB_CONFIG;
    bootMode = BootMode::NORMAL;


    Serial.printf("Boot mode = %s\n",
    bootMode == BootMode::USB_CONFIG
        ? "USB_CONFIG"
        : "NORMAL");

    if (bootMode == BootMode::USB_CONFIG) {
        delay(500); // IMPORTANT: let USB enumerate
        Serial.println("Entering USB Macro Config Mode...");
        ESP_LOGI(TAG, "USB mode active");
        delay(1000);
        return;
    }

    rfid.begin();
    rfid.printFirmwareVersion();
    delay(2000);

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

    /* ========== LOAD STORED DATA ========== */
    masterUidManager.readUIDs();
    userUidManager.readUIDs();
    phoneTokenManager.readPhones();

    /* ========== PAIRING BUTTON ========== */
    pairingButton.begin(
        []() {
            openPairingWindow();
            #ifdef ENABLE_BLE
            bleManager.openPairingWindow();
            #endif
        },
        []() {
            LED_CANCEL();
            factoryReset();
        }
    );

    /* ========== COMMUNICATION MODULES ========== */
    
    #ifdef ENABLE_WIFI_AP
    wifiModule.begin();
    #endif

    #ifdef ENABLE_BLE
    bleModule.begin();
    #endif

    /* ========== ACCESS SERVICE ========== */
    delay(2000);
    accessServiceSetup();

    ESP_LOGI(TAG, "System boot complete");
}

void loop() {
    if (bootMode == BootMode::USB_CONFIG) {
        UsbCommandHandler::loop(macroConfigManager);
        delay(5);
        return;
    }
        
    accessServiceLoop();
    pairingButton.update();

    // ========== COMMUNICATION MODULE UPDATES ==========
    #ifdef ENABLE_WIFI_AP
    wifiModule.update();
    #endif

    #ifdef ENABLE_BLE
    bleModule.update();
    bleModule.updatePairingWindow();
    #endif
    // ==================================================
}