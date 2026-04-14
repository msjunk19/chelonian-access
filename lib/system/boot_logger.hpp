#pragma once
#include <esp_log.h>
#include <esp_system.h>
#include <access_log.hpp>

static const char* BOOTTAG = "BOOT_LOG";

inline void logBootReason() {
    static bool bootLogged = false;
    if (bootLogged) return;
    
    esp_reset_reason_t resetReason = esp_reset_reason();
    const char* msg = nullptr;
    
    // Use numeric values to avoid macro conflicts
    switch ((int)resetReason) {
        case 0:  // ESP_RST_UNKNOWN
            msg = "Reflash/OTA update";
            break;
        case 1:  // ESP_RST_POWERON //Physical Reset Button 
            msg = "Power on (cold boot)";
            break;
        case 3:  // ESP_RST_EXT //Reset from WiFi AP Button
            msg = "External reset (manual/flash)";
            break;
        case 4:  // ESP_RST_SW
            msg = "Software reset";
            break;
        case 12: // ESP_RST_BROWNOUT
            msg = "Brownout reset";
            break;
        case 5:  // ESP_RST_INT_WDT
        case 7:  // ESP_RST_TASK_WDT
            msg = "Watchdog reset";
            break;
        default:
            msg = "Unknown reset";
            break;
    }
    
    if (msg) {
        ESP_LOGI(BOOTTAG, "%s", msg);
        // accessLogger.logSystem(LogSource::RFID, LogResult::SUCCESS, "System", msg);
        accessLogger.logSystem(LogSource::SYSTEM, LogResult::SUCCESS, "System", msg);

    }
    
    bootLogged = true;
}