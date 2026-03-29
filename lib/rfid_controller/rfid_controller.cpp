#include "rfid_controller.h"
#include <array>
#include <cstring>

#include <Adafruit_PN532.h>
#include <Arduino.h>
#include "esp_log.h"
#include "globals.hpp"

//Logging levels corrected

static const char* TAG = "RFID";  // Add TAG definition

    // Using SPI interface with Adafruit_PN532, all pin mapping in globals/pin_mapping.h
RFIDController::RFIDController(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss)
    : m_nfc(new Adafruit_PN532(clk, miso, mosi, ss)) {
}

RFIDController::~RFIDController() {
    delete m_nfc;
}

    bool RFIDController::begin() {
        ESP_LOGI(TAG, "Calling m_nfc->begin()...");
        m_nfc->begin();
        ESP_LOGI(TAG, "m_nfc->begin() complete, calling getFirmwareVersion()...");
        
        uint32_t versiondata = m_nfc->getFirmwareVersion();
        ESP_LOGI(TAG, "getFirmwareVersion() returned: 0x%08X", versiondata);
        
        if (versiondata == 0) {
            ESP_LOGW(TAG, "First attempt failed, trying reset...");
            m_nfc->reset();
            delay(100);
            ESP_LOGI(TAG, "Calling getFirmwareVersion() after reset...");
            versiondata = m_nfc->getFirmwareVersion();
            ESP_LOGI(TAG, "getFirmwareVersion() after reset returned: 0x%08X", versiondata);
        }

        if (versiondata == 0) {
            ESP_LOGE(TAG, "PN532 not detected or firmware read failed!");
            return false;
        }

        ESP_LOGI(TAG, "PN532 initialized successfully");
        ESP_LOGI(TAG, "PN5 (IC: %02X) Firmware %u.%u",
            (versiondata >> 24) & 0xFF,
            (versiondata >> 16) & 0xFF,
            (versiondata >> 8) & 0xFF); 

        m_nfc->SAMConfig();
        return true;
    }

//Adjusted readCard to prevent blocking the loop indefinitely unless a card is presented. 
bool RFIDController::readCard(uint8_t* uid, uint8_t* uidLength) {
    bool result = m_nfc->readPassiveTargetID(
        PN532_MIFARE_ISO14443A,
        uid,
        uidLength,
        RFID_READ_TIMEOUT_MS
    );

    if (result) {
        char uidStr[50] = "";
        for (uint8_t i = 0; i < *uidLength; i++) {
            char hexBuf[5];
            sprintf(hexBuf, "%02X:", uid[i]);
            strcat(uidStr, hexBuf);
        }
        uidStr[strlen(uidStr) - 1] = '\0';

        ESP_LOGD(TAG, "%lu ms - Card UID: %s", millis(), uidStr);
    }

    return result;
}


bool RFIDController::validateUID(const uint8_t* uid, uint8_t uidLength) {
    // Build UID string once
    char uidStr[50] = "";
    for (uint8_t i = 0; i < uidLength; i++) {
        char hexBuf[5];
        sprintf(hexBuf, "%02X:", uid[i]);
        strcat(uidStr, hexBuf);
    }
    uidStr[strlen(uidStr) - 1] = '\0';

    ESP_LOGD(TAG, "%lu ms - Validating UID: %s", millis(), uidStr);

    // Check Master UIDs first
    if (masterUidManager.checkUID((uint8_t*)uid, uidLength)) {
        ESP_LOGI(TAG, "Authentication successful (Master UID)");
        return true;
    }

    // Then check normal User UIDs
    if (userUidManager.checkUID((uint8_t*)uid, uidLength)) {
        ESP_LOGI(TAG, "Authentication successful (User UID)");
        return true;
    }

    ESP_LOGW(TAG, "Authentication failed (UID: %s)", uidStr);
    return false;
}

uint32_t RFIDController::getFirmwareVersion() {
    return m_nfc->getFirmwareVersion();
}

void RFIDController::printFirmwareVersion() {
    uint32_t versiondata = getFirmwareVersion();
    ESP_LOGI(TAG, "PN5 (IC: %02X) Firmware %u.%u",
            (versiondata >> 24) & 0xFF,
            (versiondata >> 16) & 0xFF,
            (versiondata >> 8) & 0xFF);
}

bool RFIDController::isResponding() {
    uint32_t v = m_nfc->getFirmwareVersion();
    // ESP_LOGV(TAG, "isResponding() getFirmwareVersion() = 0x%08X", v);
    return v != 0;
}

bool RFIDController::reinitialize() {
    ESP_LOGW(TAG, "PN532 not responding — attempting reinitialization");
    
    // Completely destroy and recreate the NFC object to reset SPI state
    delete m_nfc;
    delay(500);
    m_nfc = new Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    delay(500);
    
    bool result = begin();
    if (result) {
        ESP_LOGI(TAG, "PN532 reinitialized successfully");
    } else {
        ESP_LOGE(TAG, "PN532 reinitialization failed — will retry");
    }
    return result;
}