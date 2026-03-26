#pragma once
#include <Preferences.h>
#include <config.hpp>
#include "esp_log.h"

static const char* USERTAG = "USERUID";
static const char* USER_NVS_NS = "user_uids"; // NVS namespace

class UserUIDManager {
public:
    bool hasUserUIDs = false;

    void clearUsers() {
        Preferences prefs;
        prefs.begin(USER_NVS_NS, false);
        prefs.clear();
        prefs.end();
        hasUserUIDs = false;
        ESP_LOGI(USERTAG, "User UIDs cleared from NVS");
    }

    void printUID(const uint8_t* uid, uint8_t len, const char* prefix = nullptr) {
        char uidStr[32] = {0};
        for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
            if (i > 0) uidStr[pos++] = ':';
            pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
        }
        if (prefix) {
            ESP_LOGI(USERTAG, "%s %s", prefix, uidStr);
        } else {
            ESP_LOGI(USERTAG, "%s", uidStr);
        }
    }

    bool readUIDs() {
        Preferences prefs;
        prefs.begin(USER_NVS_NS, true);
        uint8_t count = prefs.getUChar("u_count", 0);

        for (uint8_t i = 0; i < count; i++) {
            char key[12];
            snprintf(key, sizeof(key), "u_uid_%u", i);

            uint8_t blob[UID_MAX_LEN + 1];
            size_t read = prefs.getBytes(key, blob, sizeof(blob));
            if (read < 2) continue;

            uint8_t len = blob[0];
            uint8_t* uid = blob + 1;

            char uidStr[32] = {0};
            for (uint8_t j = 0, pos = 0; j < len && pos + 3 < sizeof(uidStr); j++) {
                if (j > 0) uidStr[pos++] = ':';
                pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[j]);
            }
            ESP_LOGI(USERTAG, "User UID #%u (length %u): %s", i, len, uidStr);
        }

        prefs.end();
        hasUserUIDs = (count > 0);
        if (!hasUserUIDs) ESP_LOGE(USERTAG, "No User UIDs stored in NVS!");
        return hasUserUIDs;
    }

    bool checkUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;
        Preferences prefs;
        prefs.begin(USER_NVS_NS, true);
        uint8_t count = prefs.getUChar("u_count", 0);
        bool result = _checkUID(prefs, uid, len, count);
        prefs.end();
        return result;
    }

    bool addUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;

        Preferences prefs;
        prefs.begin(USER_NVS_NS, false);
        uint8_t count = prefs.getUChar("u_count", 0);

        if (_checkUID(prefs, uid, len, count)) {
            prefs.end();
            ESP_LOGI(USERTAG, "UID already exists, skipping");
            return false;
        }

        if (count >= MAX_USER_UIDS) {
            prefs.end();
            ESP_LOGE(USERTAG, "User UID storage full!");
            return false;
        }

        char key[12];
        snprintf(key, sizeof(key), "u_uid_%u", count);

        uint8_t blob[UID_MAX_LEN + 1];
        blob[0] = len;
        memcpy(blob + 1, uid, len);
        prefs.putBytes(key, blob, len + 1);

        prefs.putUChar("u_count", count + 1);
        prefs.end();

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

        Preferences prefs;
        prefs.begin(USER_NVS_NS, false);
        uint8_t count = prefs.getUChar("u_count", 0);

        int removeIdx = -1;

        for (uint8_t i = 0; i < count; i++) {
            char key[12];
            snprintf(key, sizeof(key), "u_uid_%u", i);

            uint8_t blob[UID_MAX_LEN + 1];
            size_t read = prefs.getBytes(key, blob, sizeof(blob));
            if (read < 2) continue;

            if (blob[0] == len && memcmp(blob + 1, uid, len) == 0) {
                removeIdx = i;
                break;
            }
        }

        if (removeIdx < 0) {
            prefs.end();
            ESP_LOGW(USERTAG, "UID not found to remove");
            return false;
        }

        // Shift entries above removeIdx down by one
        for (uint8_t i = removeIdx; i < count - 1; i++) {
            char srcKey[12], dstKey[12];
            snprintf(srcKey, sizeof(srcKey), "u_uid_%u", i + 1);
            snprintf(dstKey, sizeof(dstKey), "u_uid_%u", i);

            uint8_t blob[UID_MAX_LEN + 1];
            size_t read = prefs.getBytes(srcKey, blob, sizeof(blob));
            prefs.putBytes(dstKey, blob, read);
        }

        // Remove last (now duplicate) slot
        char lastKey[12];
        snprintf(lastKey, sizeof(lastKey), "u_uid_%u", count - 1);
        prefs.remove(lastKey);

        prefs.putUChar("u_count", count - 1);
        prefs.end();

        char uidStr[32] = {0};
        for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
            if (i > 0) uidStr[pos++] = ':';
            pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
        }
        ESP_LOGI(USERTAG, "User UID Removed: %s", uidStr);
        return true;
    }

private:
    bool _checkUID(Preferences& prefs, uint8_t* uid, uint8_t len, uint8_t count) {
        for (uint8_t i = 0; i < count; i++) {
            char key[12];
            snprintf(key, sizeof(key), "u_uid_%u", i);

            uint8_t blob[UID_MAX_LEN + 1];
            size_t read = prefs.getBytes(key, blob, sizeof(blob));
            if (read < 2) continue;

            if (blob[0] == len && memcmp(blob + 1, uid, len) == 0) return true;
        }
        return false;
    }
};



// #pragma once
// #include <EEPROM.h>
// #include <config.hpp>
// #include "esp_log.h"

// //Logging levels corrected, only serial print in printUID, check later

// static const char* USERTAG = "USERUID";  // Add TAG definition

// class UserUIDManager {
// public:
//     bool hasUserUIDs = false;

//     //Clear User Region
//     void clearUsers() {
//         for (uint16_t addr = USER_START; addr < USER_START + USER_SIZE; addr++) {
//             EEPROM.write(addr, 0xFF);
//         }
//         EEPROM.commit();
//         ESP_LOGI(USERTAG, "User EEPROM Cleared");
//     }

//     // Print a UID nicely
//     void printUID(uint8_t* uid, uint8_t len) {
//         char uidStr[50] = "";
//         for (uint8_t i = 0; i < len; i++) {
//             char buf[4];
//             sprintf(buf, "%02X ", uid[i]);
//             strcat(uidStr, buf);
//         }
//         Serial.println(uidStr);
//     }

//         // Read all master UIDs
//         bool readUIDs() {
//             uint16_t addr = USER_START;
//             int index = 0;

//             while (addr < USER_START + USER_SIZE) {

//             uint8_t len = EEPROM.read(addr++);
//             if (len == 0xFF || len == 0) break; // end marker

//             if (len > UID_MAX_LEN) {
//                 ESP_LOGE(USERTAG, "Invalid UID length detected, aborting scan.");
//                 break;
//             }

//             // Read UID bytes
//             uint8_t uid[UID_MAX_LEN];
//             for (uint8_t i = 0; i < len; i++) {
//                 uid[i] = EEPROM.read(addr++);
//             }

//             // Format UID into a buffer
//             char uidStr[32] = {0};
//             for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//                 if (i > 0) uidStr[pos++] = ':';
//                 pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//             }

//             // Single-line log for User UID
//             ESP_LOGI(USERTAG, "User UID #%u (length %u): %s", index++, len, uidStr);
            
//             }

//             hasUserUIDs = (index > 0);
//             if (!hasUserUIDs) ESP_LOGE(USERTAG, "No User UIDs stored in EEPROM!");

//             return hasUserUIDs;
//         }
        
//         bool checkUID(uint8_t* uid, uint8_t len) {
//             if (len == 0 || len > UID_MAX_LEN) return false;

//             uint16_t addr = USER_START;
//             while (addr < USER_START + USER_SIZE) {
//                 uint8_t storedLen = EEPROM.read(addr++);
//                 if (storedLen == 0xFF || storedLen == 0) break;
//                 if (storedLen > UID_MAX_LEN) {
//                     ESP_LOGE(USERTAG, "Invalid stored UID length detected, aborting check.");

//                     break;
//                 }

//                 if (storedLen == len) {
//                     bool match = true;
//                     for (int i = 0; i < len; i++) {
//                         if (EEPROM.read(addr + i) != uid[i]) {
//                             match = false;
//                             break;
//                         }
//                     }
//                     if (match) return true;
//                 }

//                 addr += (storedLen > 0 ? storedLen : 1);
//             }

//             return false;
//         }

//         bool addUID(uint8_t* uid, uint8_t len) {
//             if (checkUID(uid, len)) return false;

//             uint16_t addr = findFirstFreeAddress();

//             if (addr + 1 + len > USER_START + USER_SIZE) return false;

//             EEPROM.write(addr++, len);
//             for (int i = 0; i < len; i++) {
//                 EEPROM.write(addr++, uid[i]);
//             }

//             EEPROM.commit();
//             char uidStr[32] = {0};
//             for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//                 if (i > 0) uidStr[pos++] = ':';
//                 pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//             }
//             ESP_LOGI(USERTAG, "User UID Added: %s", uidStr);
//             return true;
//         }

//     bool removeUID(uint8_t* uid, uint8_t len) {
//         if (len == 0 || len > UID_MAX_LEN) return false;

//         uint16_t addr = USER_START;
//         while (addr < USER_START + USER_SIZE) {
//             uint16_t uidStart = addr;
//             uint8_t storedLen = EEPROM.read(addr++);
//             if (storedLen == 0xFF || storedLen == 0) break;
//             if (storedLen > UID_MAX_LEN) {
//                 ESP_LOGE(MASTERTAG, "Invalid UID length detected, aborting removal.");
//                 break;
//             }

//             bool match = (storedLen == len);
//             for (int i = 0; i < len && match; i++) {
//                 if (EEPROM.read(addr + i) != uid[i]) {
//                     match = false;
//                 }
//             }

//             if (match) {
//                 uint16_t nextAddr = addr + storedLen;

//                 while (nextAddr < USER_START + USER_SIZE) {
//                     EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
//                 }

//                 while (uidStart < USER_START + USER_SIZE) {
//                     EEPROM.write(uidStart++, 0xFF);
//                 }

//                 EEPROM.commit();
//                 // EEPROM.commit();
//                 char uidStr[32] = {0};
//                 for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//                     if (i > 0) uidStr[pos++] = ':';
//                     pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//                 }
//                 ESP_LOGI(USERTAG, "User UID Removed: %s", uidStr);
//                 return true;
//             }

//             addr += storedLen;
//         }

//         return false;
//     }

// private:
//     uint16_t findFirstFreeAddress() {
//         uint16_t addr = USER_START;

//         while (addr < USER_START + USER_SIZE) {
//             uint8_t len = EEPROM.read(addr);
//             if (len == 0xFF || len == 0) break;
//             if (len > UID_MAX_LEN) {
//                 ESP_LOGE(USERTAG, "Invalid UID length detected in findFirstFreeAddress, stopping.");

//                 break;
//             }
//             addr += (1 + len);
//         }
//         return addr;
//     }
// };