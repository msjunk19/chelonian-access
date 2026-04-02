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
