#include "rfid_controller.h"
#include <array>
#include <cstring>

#include <Adafruit_PN532.h>
#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "RFID";  // Add TAG definition

RFIDController::RFIDController(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss)
    : m_nfc(new Adafruit_PN532(clk, miso, mosi, ss)) {
    // Using SPI interface with Adafruit_PN532
}

RFIDController::~RFIDController() {
    delete m_nfc;
}

bool RFIDController::begin() {
    if (!m_nfc->begin()) {  // Check if begin was successful
        ESP_LOGE(TAG, "PN532 initialization failed!");
        return false;
    } else {
        ESP_LOGE(TAG, "PN532 initialized successfully");
    }

    // Set SCK pin drive strength to be faster (assuming default SCK is GPIO5 for ESP32C3)
    // This is a workaround for STM32F40x/F41x silicon limitation, might help with ESP32C3 as well
    // gpio_set_drive_capability(GPIO_NUM_4, GPIO_DRIVE_CAP_3);

    uint32_t versiondata = m_nfc->getFirmwareVersion();
    if (!versiondata) {
        // Try to reset the PN532
        m_nfc->reset();
        // Wait a bit for the reset to take effect
        delay(100);
        versiondata = m_nfc->getFirmwareVersion();
        ESP_LOGE(TAG, "Firmware Version after reset: %X", versiondata);
    }

    if (versiondata == 0) {
        ESP_LOGE(TAG, "Didn't find PN532 board or get firmware version failed!");
        return false;
    }

    ESP_LOGE(TAG, "PN532 initialized successfully. Firmware Version: %X", versiondata);

    // Configure board to read RFID tags
    m_nfc->SAMConfig();
    return true;
}

//Adjusted readCard to prevent blocking the loop indefinitely unless a card is presented. 
bool RFIDController::readCard(uint8_t* uid, uint8_t* uidLength) {
    // Set a short timeout (150 ms) to avoid blocking the loop, 20ms works but needs to be longer to read implants. 150 is working currently. Adjust as needed for implant.
    constexpr uint16_t TIMEOUT_MS = 150;
    bool result = m_nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength, TIMEOUT_MS);

    if (result) {
        ESP_LOGE(TAG, "%lu ms - Card detected: UID=", millis());
        char uidStr[50] = "";
        for (uint8_t i = 0; i < *uidLength; i++) {
            char hexBuf[5];
            sprintf(hexBuf, "%02X:", uid[i]);
            strcat(uidStr, hexBuf);
        }
        uidStr[strlen(uidStr) - 1] = '\0';  // Remove last colon
        ESP_LOGE(TAG, "%s", uidStr);
    } else {
        // ESP_LOGE(TAG, "No card detected");
    }

    return result;
}

bool RFIDController::validateUID(const uint8_t* uid, uint8_t uidLength) {
    ESP_LOGE(TAG, "%lu ms - Validating UID=", millis());
    char uidStrValidate[50] = "";
    for (uint8_t i = 0; i < uidLength; i++) {
        char hexBuf[5];
        sprintf(hexBuf, "%02X:", uid[i]);
        strcat(uidStrValidate, hexBuf);
    }
    uidStrValidate[strlen(uidStrValidate) - 1] = '\0';  // Remove last colon
    ESP_LOGE(TAG, "%s", uidStrValidate);

    if (uidLength == 4) {
        for (uint8_t i = 0; i < m_num4BUIDs; i++) {
            if (compare4BUID(m_uids4B[i].data(), uid)) {
                ESP_LOGE(TAG, " - Authentication successful (4B)");
                return true;
            }
        }
    } else if (uidLength == 7) {
        for (uint8_t i = 0; i < m_num7BUIDs; i++) {
            if (compare7BUID(m_uids7B[i].data(), uid)) {
                ESP_LOGE(TAG, " - Authentication successful (7B)");
                return true;
            }
        }
    } else {
        ESP_LOGW(TAG, " - Authentication failed: Invalid UID length");
        return false;
    }

    ESP_LOGW(TAG, " - Authentication failed: No matching UID");
    return false;
}

void RFIDController::addUID4B(const uint8_t* uid) {
    if (m_num4BUIDs < MAX_4B_UIDS) {
        memcpy(m_uids4B[m_num4BUIDs].data(), uid, 4);
        m_num4BUIDs++;
    }
}

void RFIDController::addUID7B(const uint8_t* uid) {
    if (m_num7BUIDs < MAX_7B_UIDS) {
        memcpy(m_uids7B[m_num7BUIDs].data(), uid, 7);
        m_num7BUIDs++;
    }
}

uint32_t RFIDController::getFirmwareVersion() {
    return m_nfc->getFirmwareVersion();
}

void RFIDController::printFirmwareVersion() {
    uint32_t versiondata = getFirmwareVersion();
    if (versiondata != 0u) {
        ESP_LOGE(TAG, "Found chip PN5");
        ESP_LOGE(TAG, "%02X", (versiondata >> 24) & 0xFF);  // Log the value
        ESP_LOGE(TAG, "Firmware ver. %u.%u", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    }
}

void RFIDController::initializeDefaultUIDs() {
    // Initialize with test UIDs as specified
    // std::array<uint8_t, 4> testUID4B = {0xB4, 0x12, 0x34, 0x56}; //Default test UID
    std::array<uint8_t, 4> testUID4B = {0xCC, 0x0F, 0x12, 0x07}; // my test UID

    std::array<uint8_t, 7> testUiD7B1 = {0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    std::array<uint8_t, 7> testUiD7B2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD};
    std::array<uint8_t, 7> testUiD7B3 = {0x04, 0x4A, 0x8C, 0x32, 0x0A, 0x54, 0x80};

    addUID4B(testUID4B.data());
    addUID7B(testUiD7B1.data());
    addUID7B(testUiD7B2.data());
    addUID7B(testUiD7B3.data());
}

bool RFIDController::compare4BUID(const uint8_t* uid1, const uint8_t* uid2) {
    return memcmp(uid1, uid2, 4) == 0;
}

bool RFIDController::compare7BUID(const uint8_t* uid1, const uint8_t* uid2) {
    return memcmp(uid1, uid2, 7) == 0;
}
