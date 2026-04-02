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
