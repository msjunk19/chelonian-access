#pragma once
#include <Preferences.h>
#include <config.hpp>
#include "esp_log.h"

static const char* MASTERTAG = "MASTERUID";
static const char* MASTER_NVS_NS = "master_uids"; // NVS namespace

class MasterUIDManager {
public:
    bool hasMasterUIDs = false;

    void clearMasters() {
        Preferences prefs;
        prefs.begin(MASTER_NVS_NS, false);
        prefs.clear(); // wipes entire namespace
        prefs.end();
        hasMasterUIDs = false;
        ESP_LOGI(MASTERTAG, "Master UIDs cleared from NVS");
    }

    void printUID(const uint8_t* uid, uint8_t len, const char* prefix = nullptr) {
        char uidStr[32] = {0};
        for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
            if (i > 0) uidStr[pos++] = ':';
            pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
        }
        if (prefix) {
            ESP_LOGI(MASTERTAG, "%s %s", prefix, uidStr);
        } else {
            ESP_LOGI(MASTERTAG, "%s", uidStr);
        }
    }

    void writeUIDs(uint8_t** uids, uint8_t* lengths, size_t count) {
        Preferences prefs;
        prefs.begin(MASTER_NVS_NS, false);
        uint8_t stored = prefs.getUChar("m_count", 0);

        for (size_t i = 0; i < count; i++) {
            uint8_t len = lengths[i];

            if (len == 0 || len > UID_MAX_LEN) {
                ESP_LOGW(MASTERTAG, "Skipping invalid UID length: %u", len);
                continue;
            }

            if (stored >= MAX_MASTER_UIDS) {
                ESP_LOGE(MASTERTAG, "Master UID storage full!");
                break;
            }

            // Skip if already exists
            if (_checkUID(prefs, uids[i], len, stored)) {
                ESP_LOGI(MASTERTAG, "UID already exists, skipping");
                continue;
            }

            // Store as blob: key = "m_uid_N"
            char key[12];
            snprintf(key, sizeof(key), "m_uid_%u", stored);

            // Pack len + uid bytes into a single blob
            uint8_t blob[UID_MAX_LEN + 1];
            blob[0] = len;
            memcpy(blob + 1, uids[i], len);
            prefs.putBytes(key, blob, len + 1);
            stored++;
        }

        prefs.putUChar("m_count", stored);
        prefs.end();
        ESP_LOGI(MASTERTAG, "Master UIDs written. Total stored: %u", stored);
    }

    bool readUIDs() {
        Preferences prefs;
        prefs.begin(MASTER_NVS_NS, true); // read-only
        uint8_t count = prefs.getUChar("m_count", 0);

        for (uint8_t i = 0; i < count; i++) {
            char key[12];
            snprintf(key, sizeof(key), "m_uid_%u", i);

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
            ESP_LOGI(MASTERTAG, "Master UID #%u (length %u): %s", i, len, uidStr);
        }

        prefs.end();
        hasMasterUIDs = (count > 0);
        if (!hasMasterUIDs) ESP_LOGE(MASTERTAG, "No master UIDs stored in NVS!");
        return hasMasterUIDs;
    }

    bool checkUID(uint8_t* uid, uint8_t len) {
        Preferences prefs;
        prefs.begin(MASTER_NVS_NS, true);
        uint8_t count = prefs.getUChar("m_count", 0);
        bool result = _checkUID(prefs, uid, len, count);
        prefs.end();
        return result;
    }

    bool removeUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;

        Preferences prefs;
        prefs.begin(MASTER_NVS_NS, false);
        uint8_t count = prefs.getUChar("m_count", 0);

        int removeIdx = -1;

        // Find the index to remove
        for (uint8_t i = 0; i < count; i++) {
            char key[12];
            snprintf(key, sizeof(key), "m_uid_%u", i);

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
            ESP_LOGW(MASTERTAG, "UID not found to remove");
            return false;
        }

        // Shift all entries above removeIdx down by one
        for (uint8_t i = removeIdx; i < count - 1; i++) {
            char srcKey[12], dstKey[12];
            snprintf(srcKey, sizeof(srcKey), "m_uid_%u", i + 1);
            snprintf(dstKey, sizeof(dstKey), "m_uid_%u", i);

            uint8_t blob[UID_MAX_LEN + 1];
            size_t read = prefs.getBytes(srcKey, blob, sizeof(blob));
            prefs.putBytes(dstKey, blob, read);
        }

        // Remove the last (now duplicate) slot
        char lastKey[12];
        snprintf(lastKey, sizeof(lastKey), "m_uid_%u", count - 1);
        prefs.remove(lastKey);

        prefs.putUChar("m_count", count - 1);
        prefs.end();
        ESP_LOGI(MASTERTAG, "Master UID removed. Remaining: %u", count - 1);
        return true;
    }

private:
    // Internal check — reuses an already-open Preferences handle
    bool _checkUID(Preferences& prefs, uint8_t* uid, uint8_t len, uint8_t count) {
        for (uint8_t i = 0; i < count; i++) {
            char key[12];
            snprintf(key, sizeof(key), "m_uid_%u", i);

            uint8_t blob[UID_MAX_LEN + 1];
            size_t read = prefs.getBytes(key, blob, sizeof(blob));
            if (read < 2) continue;

            if (blob[0] == len && memcmp(blob + 1, uid, len) == 0) return true;
        }
        return false;
    }
};



// EEPROM version, working
// #pragma once
// #include <EEPROM.h>
// #include <config.hpp>
// #include "esp_log.h"

// //Logging levels corrected, only serial print in printUID, check later

// static const char* MASTERTAG = "MASTERUID";  // Add TAG definition


// class MasterUIDManager {
// public:
//     bool hasMasterUIDs = false;

//     // Clear only the master region
//     void clearMasters() {
//         for (uint16_t addr = MASTER_START; addr < MASTER_START + MASTER_SIZE; addr++) {
//             EEPROM.write(addr, 0xFF);
//         }
//         EEPROM.commit();
//         ESP_LOGI(MASTERTAG, "Master User EEPROM Cleared");
//     }

//     // Print a UID nicely, potentially remove after fixing logging
//     // ESP_LOG version
//     void printUID(const uint8_t* uid, uint8_t len, const char* prefix = nullptr) {
//         char uidStr[32] = {0};
//         for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//             if (i > 0) uidStr[pos++] = ':';
//             pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//         }
//         if (prefix) {
//             ESP_LOGI(MASTERTAG, "%s %s", prefix, uidStr);
//         } else {
//             ESP_LOGI(MASTERTAG, "%s", uidStr);
//         }
//     }
//     // void printUID(uint8_t* uid, uint8_t len) {
//     //     char uidStr[50] = "";
//     //     for (uint8_t i = 0; i < len; i++) {
//     //         char buf[4];
//     //         sprintf(buf, "%02X ", uid[i]);
//     //         strcat(uidStr, buf);
//     //     }
//     //     Serial.println(uidStr);
//     // }

//     // Write multiple UIDs to the master region
//     void writeUIDs(uint8_t** uids, uint8_t* lengths, size_t count) {
//         uint16_t addr = findFirstFreeAddress();

//         for (size_t i = 0; i < count; i++) {
//             uint8_t len = lengths[i];

//             // Skip invalid UID length
//             if (len == 0 || len > UID_MAX_LEN) {
//                 ESP_LOGW(MASTERTAG, "Skipping invalid UID length: %u", len);
//                 continue;
//             }

//             // Check for space
//             if (addr + 1 + len > MASTER_START + MASTER_SIZE) {
//                 ESP_LOGE(MASTERTAG, "Not enough space to append UID!");
//                 break;
//             }

//             // Skip if UID already exists
//             if (checkUID(uids[i], len)) {
//                 char uidStr[32] = {0};
//                 for (uint8_t j = 0, pos = 0; j < len && pos + 3 < sizeof(uidStr); j++) {
//                     if (j > 0) uidStr[pos++] = ':';
//                     pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uids[i][j]);
//                 }
//                 ESP_LOGI(MASTERTAG, "UID already exists, skipping: %s", uidStr);
//                 continue;
// }

//             // Write length + bytes
//             EEPROM.write(addr++, len);
//             for (int j = 0; j < len; j++) {
//                 EEPROM.write(addr++, uids[i][j]);
//             }
//         }

//         EEPROM.commit();
//         ESP_LOGI(MASTERTAG, "Master UIDs written/appended to EEPROM!");

//     }

//     // Read all master UIDs
//     bool readUIDs() {
//         uint16_t addr = MASTER_START;
//         int index = 0;

//         while (addr < MASTER_START + MASTER_SIZE) {
            
//         // Read UID length
//         uint8_t len = EEPROM.read(addr++);
//         if (len == 0xFF || len == 0) break; // end marker

//         if (len > UID_MAX_LEN) {
//             ESP_LOGE(MASTERTAG, "Invalid UID length detected, aborting scan.");
//             break;
//         }

//         // Read UID bytes
//         uint8_t uid[UID_MAX_LEN];
//         for (uint8_t i = 0; i < len; i++) {
//             uid[i] = EEPROM.read(addr++);
//         }

//         // Format UID into a buffer
//         char uidStr[32] = {0};
//         for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//             if (i > 0) uidStr[pos++] = ':';
//             pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//         }

//         // Single-line log for master UID
//         ESP_LOGI(MASTERTAG, "Master UID #%u (length %u): %s", index++, len, uidStr);

//         }

//         hasMasterUIDs = (index > 0);
//         if (!hasMasterUIDs) ESP_LOGE(MASTERTAG, "No master UIDs stored in EEPROM!");

//         return hasMasterUIDs;
//     }

//     // Check if a UID exists
//     bool checkUID(uint8_t* uid, uint8_t len) {
//         if (len == 0 || len > UID_MAX_LEN) return false;

//         uint16_t addr = MASTER_START;
//         while (addr < MASTER_START + MASTER_SIZE) {
//             uint8_t storedLen = EEPROM.read(addr++);
//             if (storedLen == 0xFF || storedLen == 0) break;
//             if (storedLen > UID_MAX_LEN) {
//                 ESP_LOGE(MASTERTAG, "Invalid stored UID length detected, aborting check.");
//                 break;
//             }

//             if (storedLen == len) {
//                 bool match = true;
//                 for (int i = 0; i < len; i++) {
//                     if (EEPROM.read(addr + i) != uid[i]) {
//                         match = false;
//                         break;
//                     }
//                 }
//                 if (match) return true;
//             }

//             addr += (storedLen > 0 ? storedLen : 1); // always advance at least 1
//         }

//         return false;
//     }

//     // Remove a UID
//     bool removeUID(uint8_t* uid, uint8_t len) {
//         if (len == 0 || len > UID_MAX_LEN) return false;

//         uint16_t addr = MASTER_START;
//         while (addr < MASTER_START + MASTER_SIZE) {
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
//                 match = false;
//                 }
//             }

//             if (match) {
//                 uint16_t nextAddr = addr + storedLen;
//                 // Shift remaining UIDs down
//                 while (nextAddr < MASTER_START + MASTER_SIZE) {
//                     EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
//                 }
//                 // Fill remaining bytes with 0xFF
//                 while (uidStart < MASTER_START + MASTER_SIZE) {
//                     EEPROM.write(uidStart++, 0xFF);
//                 }
//                 EEPROM.commit();
//                 char uidStr[32] = {0};
//                 for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//                     if (i > 0) uidStr[pos++] = ':';
//                     pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//                 }
//                 ESP_LOGI(MASTERTAG, "UID removed: %s", uidStr);
//                 return true;
//             }

//             addr += (storedLen > 0 ? storedLen : 1);
//         }

//         char uidStr[32] = {0};
//         for (uint8_t i = 0, pos = 0; i < len && pos + 3 < sizeof(uidStr); i++) {
//             if (i > 0) uidStr[pos++] = ':';
//             pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
//         }
//         ESP_LOGW(MASTERTAG, "UID Not Found To Remove: %s", uidStr);
//         return false;
//     }

// private:
//     // Find first free address in master region
//     uint16_t findFirstFreeAddress() {
//         uint16_t addr = MASTER_START;
//         while (addr < MASTER_START + MASTER_SIZE) {
//             uint8_t len = EEPROM.read(addr);
//             if (len == 0xFF || len == 0) break;
//             if (len > UID_MAX_LEN) {
//                 ESP_LOGE(MASTERTAG, "Invalid UID length detected in findFirstFreeAddress, stopping.");
//                 break;
//             }
//             addr += (1 + len);
//         }
//         return addr;
//     }
// };