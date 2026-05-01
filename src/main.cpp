#include <Arduino.h>
#include <esp_log.h>
#include <esp_system.h>

#include "access_service.h"
#include "exception_handler.h"
#include "globals.hpp"
#include "config.hpp"
#include "eeprom_utils.hpp"
#include "wifi_manager.hpp"
#include "wifi_auth_endpoints_hardened.hpp"
#include "pairing_button.hpp"
#include "auth_manager.hpp"
#include "factory_reset.hpp"
#include "macro_config.hpp"
#include "access_log.hpp"
#include "boot_logger.hpp"
#include "rate_limiter.hpp"
#include "encrypted_token_storage.hpp"
#include "nonce_manager.hpp"
#include "usb_macro_mode.hpp"
#include "usb_handler.hpp"
#include "Preferences.h"

// ========== NEW: Feature toggles and modules ==========
#include "features.hpp"
#include "command_handler.hpp"
#include "wifi_module.hpp"
#include "ble_module.hpp"
// ====================================================


// BootMode bootMode = BootMode::NORMAL;

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

// #define HAS_WIRELESS (defined(ENABLE_WIFI_AP) || defined(ENABLE_BLE))

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
    // void handleSerialStatus() {
    //     if (!Serial.available()) return;

    //     String cmd = Serial.readStringUntil('\n');
    //     cmd.trim();

    // if (cmd == "GET_STATUS") {

    //     if (bootMode == BootMode::USB_CONFIG) {
    //         Serial.println("{\"ok\":true,\"mode\":\"usb\"}");
    //         return;
    //     }

    //     // NORMAL mode
    //     #if defined(ENABLE_WIFI_AP) || defined(ENABLE_BLE)

    //         // 🔥 Wireless firmware build
    //         Serial.println(
    //             "{\"ok\":false,"
    //             "\"mode\":\"wireless\","
    //             "\"error\":\"usb_disabled\","
    //             "\"message\":\"Device is configured for wireless mode. USB configuration is not supported.\"}"
    //         );

    //     #else

    //         // 🔥 USB-capable firmware, just not in USB mode
    //         Serial.println(
    //             "{\"ok\":false,"
    //             "\"mode\":\"normal\","
    //             "\"error\":\"not_in_usb_mode\","
    //             "\"message\":\"Hold pairing button and reboot to enter USB mode.\"}"
    //         );

    //     #endif

    //     return;
    // }
    // }

void setup()
{
    Serial.begin(115200);
    Serial.setRxBufferSize(1024);  // Increase from default 256 to 1024
    delay(500);

    macroConfigManager.load();
    delay(1000);

    led.begin();
    delay(1000);

    Preferences prefs;
    prefs.begin("system", true);
    Serial.println("Reading boot_mode from NVS...");
    // uint8_t mode = prefs.getUChar("boot_mode", 255);
    uint8_t mode = prefs.getUChar("boot_mode", (uint8_t)BootMode::NORMAL);
    bootMode = (BootMode)mode;
    Serial.printf("boot_mode raw = %u\n", mode);
    // Serial.println(prefs.getUChar("boot_mode"));
    // bootMode = (BootMode)prefs.getUChar("boot_mode", (uint8_t)BootMode::NORMAL);
    // Serial.print("Boot Mode: (%lu)", bootMode);

    prefs.end();

    // bootMode = BootMode::USB_CONFIG;
    // // bootMode = BootMode::NORMAL;


    Serial.printf("Boot mode = %s\n",
    bootMode == BootMode::USB_CONFIG
        ? "USB_CONFIG"
        : "NORMAL");

    if (bootMode == BootMode::USB_CONFIG) {
        delay(500); // IMPORTANT: let USB enumerate
        Serial.println("Entering USB Macro Config Mode...");
        ESP_LOGI(TAG, "USB mode active");
        LED_SET_SEQ(SYSTEM_USB);

        // Reset boot mode so next reboot is normal
        pairingButton.setBootMode(BootMode::NORMAL);

        delay(1000);
        return;
    }

    rfid.begin();
    delay(2000);

    /* NEW: Initialize encrypted storage */
    if (!encryptedStorage.begin()) {
        ESP_LOGE("MAIN", "Failed to initialize encrypted storage!");
        delay(1000);
        ESP.restart();
    }

    accessLogger.begin();
    
    logBootReason();
    ESP_LOGI("BOOT", "Reset reason: %d", esp_reset_reason());


    setupGlobalExceptionHandler();


    /* ========== LOAD STORED DATA ========== */
    masterUidManager.readUIDs();
    userUidManager.readUIDs();

    /* Conditionally load network-dependent managers */
    #if defined(ENABLE_WIFI_AP) || defined(ENABLE_BLE)
    phoneTokenManager.readPhones();
    nonceManager.clear();  // Optional: start fresh
    ESP_LOGI("MAIN", "Nonce manager initialized");
    #endif

    /* ========== PAIRING BUTTON ========== */
    pairingButton.begin(
        []() {
            #ifdef ENABLE_WIFI_AP
            openPairingWindow();
            #endif
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
    pairingButton.update();

    if (bootMode == BootMode::USB_CONFIG) {
        UsbCommandHandler::loop(macroConfigManager);
        delay(5);
        return;
    }
        
    accessServiceLoop();

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