#pragma once
#include <EEPROM.h>
#include <config.hpp>
#include "esp_log.h"

//Logging levels corrected, only serial print in printUID, check later

static const char* USERTAG = "USERUID";  // Add TAG definition

class UserUIDManager {
public:
    bool hasUserUIDs = false;

    //Clear User Region
    void clearUsers() {
        for (uint16_t addr = USER_START; addr < USER_START + USER_SIZE; addr++) {
            EEPROM.write(addr, 0xFF);
        }
        EEPROM.commit();
        ESP_LOGI(USERTAG, "User EEPROM Cleared");
    }

    // Print a UID nicely
    void printUID(uint8_t* uid, uint8_t len) {
        char uidStr[50] = "";
        for (uint8_t i = 0; i < len; i++) {
            char buf[4];
            sprintf(buf, "%02X ", uid[i]);
            strcat(uidStr, buf);
        }
        Serial.println(uidStr);
    }

        // Read all master UIDs
        bool readUIDs() {
            uint16_t addr = USER_START;
            int index = 0;

            while (addr < USER_START + USER_SIZE) {

            uint8_t len = EEPROM.read(addr++);
            if (len == 0xFF || len == 0) break; // end marker

            if (len > UID_MAX_LEN) {
                ESP_LOGE(USERTAG, "Invalid UID length detected, aborting scan.");
                break;
            }

            // Read UID bytes
            uint8_t uid[UID_MAX_LEN];
            for (uint8_t i = 0; i < len; i++) {
                uid[i] = EEPROM.read(addr++);
            }

            // Format UID into a buffer
            char uidStr[32] = {0};
            for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
                if (i > 0) uidStr[pos++] = ':';
                pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
            }

            // Single-line log for master UID
            ESP_LOGI(USERTAG, "Master UID #%u (length %u): %s", index++, len, uidStr);
            
            }

            hasUserUIDs = (index > 0);
            if (!hasUserUIDs) ESP_LOGE(USERTAG, "No User UIDs stored in EEPROM!");

            return hasUserUIDs;
        }
        
        bool checkUID(uint8_t* uid, uint8_t len) {
            if (len == 0 || len > UID_MAX_LEN) return false;

            uint16_t addr = USER_START;
            while (addr < USER_START + USER_SIZE) {
                uint8_t storedLen = EEPROM.read(addr++);
                if (storedLen == 0xFF || storedLen == 0) break;
                if (storedLen > UID_MAX_LEN) {
                    ESP_LOGE(USERTAG, "Invalid stored UID length detected, aborting check.");

                    break;
                }

                if (storedLen == len) {
                    bool match = true;
                    for (int i = 0; i < len; i++) {
                        if (EEPROM.read(addr + i) != uid[i]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) return true;
                }

                addr += (storedLen > 0 ? storedLen : 1);
            }

            return false;
        }

        bool addUID(uint8_t* uid, uint8_t len) {
            if (checkUID(uid, len)) return false;

            uint16_t addr = findFirstFreeAddress();

            if (addr + 1 + len > USER_START + USER_SIZE) return false;

            EEPROM.write(addr++, len);
            for (int i = 0; i < len; i++) {
                EEPROM.write(addr++, uid[i]);
            }

            EEPROM.commit();
            char uidStr[32] = {0};
            for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
                if (i > 0) uidStr[pos++] = ':';
                pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
            }
            ESP_LOGI(USERTAG, "User UID Added: %s", uidStr);
            return true;
        }

    bool removeUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;

        uint16_t addr = USER_START;
        while (addr < USER_START + USER_SIZE) {
            uint16_t uidStart = addr;
            uint8_t storedLen = EEPROM.read(addr++);
            if (storedLen == 0xFF || storedLen == 0) break;
            if (storedLen > UID_MAX_LEN) {
                ESP_LOGE(MASTERTAG, "Invalid UID length detected, aborting removal.");
                break;
            }

            bool match = (storedLen == len);
            for (int i = 0; i < len && match; i++) {
                if (EEPROM.read(addr + i) != uid[i]) {
                    match = false;
                }
            }

            if (match) {
                uint16_t nextAddr = addr + storedLen;

                while (nextAddr < USER_START + USER_SIZE) {
                    EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
                }

                while (uidStart < USER_START + USER_SIZE) {
                    EEPROM.write(uidStart++, 0xFF);
                }

                EEPROM.commit();
                // EEPROM.commit();
                char uidStr[32] = {0};
                for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
                    if (i > 0) uidStr[pos++] = ':';
                    pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
                }
                ESP_LOGI(USERTAG, "User UID Removed: %s", uidStr);
                return true;
            }

            addr += storedLen;
        }

        return false;
    }

private:
    uint16_t findFirstFreeAddress() {
        uint16_t addr = USER_START;

        while (addr < USER_START + USER_SIZE) {
            uint8_t len = EEPROM.read(addr);
            if (len == 0xFF || len == 0) break;
            if (len > UID_MAX_LEN) {
                ESP_LOGE(USERTAG, "Invalid UID length detected in findFirstFreeAddress, stopping.");

                break;
            }
            addr += (1 + len);
        }
        return addr;
    }
};