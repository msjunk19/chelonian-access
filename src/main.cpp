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

#include "esp_system.h"

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

// static bool bootLogged = false;

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

    accessLogger.begin();
    Serial.print("Log count after begin: ");
    Serial.println(accessLogger.getCount());

    setupGlobalExceptionHandler();



    ESP_LOGI(TAG, "Chelonian Access Service starting");

    ESP_LOGI("BOOT", "Reset reason: %d", esp_reset_reason());

    static int setup_calls = 0;
    setup_calls++;
    Serial.print("setup() #");
    Serial.println(setup_calls);
    Serial.println(millis());
    delay(10);  // ensure serial flush


    /* ---------------- LOAD STORED DATA ---------------- */

    masterUidManager.readUIDs();
    userUidManager.readUIDs();
    phoneTokenManager.readPhones();

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

    /* ---------------- WEB SERVER ---------------- */

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

    /* ---------------- LOGGING ---------------- */



    /* ---------------- ACCESS SERVICE ---------------- */

    accessServiceSetup();

    
    debugTime();



    // /* Log boot exactly once */
    // static bool bootLogged = false;

    // if (!bootLogged) {
    //     bootLogged = true;

    //     char msg[64];
    //     snprintf(msg, sizeof(msg),
    //         "Device boot (reason=%d)",
    //         esp_reset_reason());

    //     accessLogger.logSystem(
    //         LogSource::RFID,
    //         LogResult::SUCCESS,
    //         "System",
    //         msg
    //     );
    // }
    for (int i = 0; i < accessLogger.getCount(); i++) {
        LogEntry e;
        accessLogger.getEntry(i, e);
        Serial.print("Entry ");
        Serial.print(i);
        Serial.print(": ts=");
        Serial.print(e.timestamp);
        Serial.print(" mode=");
        Serial.print(e.timeMode);
        Serial.print(" id=");
        Serial.print(e.identifier);
        Serial.print(" msg=");
        Serial.println(e.message);
    }

    ESP_LOGI(TAG, "System boot complete");

    
}

void loop() {
    // Call the main service loop
    accessServiceLoop();
    pairingButton.update();

    handleClient();
    bleManager.update();
    bleManager.updatePairingWindow();
    static int loop_calls = 0;
loop_calls++;
if (loop_calls <= 5) {
    Serial.print("loop() #");
    Serial.println(loop_calls);
}

}
