#pragma once
#include <Preferences.h>
#include <config.hpp>
#include "esp_log.h"

static const char* PHONETAG = "PHONETOKEN";
static const char* PHONE_NVS_NS = "phone_tokens";

// Add to config.hpp:
// static constexpr uint8_t MAX_PAIRED_PHONES = 10;
// static constexpr uint8_t PHONE_SECRET_LEN  = 32;
// static constexpr uint8_t PHONE_ID_MAX_LEN  = 36; // UUID string length

class PhoneTokenManager {
public:
    bool hasPairedPhones = false;

    void clearAll() {
        Preferences prefs;
        prefs.begin(PHONE_NVS_NS, false);
        prefs.clear();
        prefs.end();
        hasPairedPhones = false;
        ESP_LOGI(PHONETAG, "All paired phones cleared");
    }

    // Add a new phone — device_id is a UUID string, secret is 32 random bytes
    bool addPhone(const char* deviceId, const uint8_t* secret) {
        if (!deviceId || strlen(deviceId) == 0 || strlen(deviceId) > PHONE_ID_MAX_LEN) {
            ESP_LOGE(PHONETAG, "Invalid device ID");
            return false;
        }

        Preferences prefs;
        prefs.begin(PHONE_NVS_NS, false);
        uint8_t count = prefs.getUChar("p_count", 0);

        // Check if already paired
        if (_findPhone(prefs, deviceId, count) >= 0) {
            prefs.end();
            ESP_LOGW(PHONETAG, "Phone already paired: %s", deviceId);
            return false;
        }

        if (count >= MAX_PAIRED_PHONES) {
            prefs.end();
            ESP_LOGE(PHONETAG, "Max paired phones reached");
            return false;
        }

        // Store device ID string
        char idKey[12];
        snprintf(idKey, sizeof(idKey), "p_id_%u", count);
        prefs.putString(idKey, deviceId);

        // Store secret blob
        char secKey[14];
        snprintf(secKey, sizeof(secKey), "p_secret_%u", count);
        prefs.putBytes(secKey, secret, PHONE_SECRET_LEN);

        prefs.putUChar("p_count", count + 1);
        prefs.end();

        ESP_LOGI(PHONETAG, "Phone paired: %s (slot %u)", deviceId, count);
        hasPairedPhones = true;
        return true;
    }

    // Retrieve the secret for a given device ID — returns false if not found
    bool getSecret(const char* deviceId, uint8_t* secretOut) {
        if (!deviceId || !secretOut) return false;

        Preferences prefs;
        prefs.begin(PHONE_NVS_NS, true);
        uint8_t count = prefs.getUChar("p_count", 0);

        int idx = _findPhone(prefs, deviceId, count);
        if (idx < 0) {
            prefs.end();
            ESP_LOGW(PHONETAG, "Phone not found: %s", deviceId);
            return false;
        }

        char secKey[14];
        snprintf(secKey, sizeof(secKey), "p_secret_%u", idx);
        size_t read = prefs.getBytes(secKey, secretOut, PHONE_SECRET_LEN);
        prefs.end();

        return (read == PHONE_SECRET_LEN);
    }

    bool removePhone(const char* deviceId) {
        if (!deviceId) return false;

        Preferences prefs;
        prefs.begin(PHONE_NVS_NS, false);
        uint8_t count = prefs.getUChar("p_count", 0);

        int removeIdx = _findPhone(prefs, deviceId, count);
        if (removeIdx < 0) {
            prefs.end();
            ESP_LOGW(PHONETAG, "Phone not found to remove: %s", deviceId);
            return false;
        }

        // Shift entries down
        for (uint8_t i = removeIdx; i < count - 1; i++) {
            char srcIdKey[12], dstIdKey[12];
            char srcSecKey[14], dstSecKey[14];
            snprintf(srcIdKey,  sizeof(srcIdKey),  "p_id_%u",     i + 1);
            snprintf(dstIdKey,  sizeof(dstIdKey),  "p_id_%u",     i);
            snprintf(srcSecKey, sizeof(srcSecKey), "p_secret_%u", i + 1);
            snprintf(dstSecKey, sizeof(dstSecKey), "p_secret_%u", i);

            // Shift ID
            char idBuf[PHONE_ID_MAX_LEN + 1];
            String idStr = prefs.getString(srcIdKey, "");
            prefs.putString(dstIdKey, idStr);

            // Shift secret
            uint8_t secBuf[PHONE_SECRET_LEN];
            prefs.getBytes(srcSecKey, secBuf, PHONE_SECRET_LEN);
            prefs.putBytes(dstSecKey, secBuf, PHONE_SECRET_LEN);
        }

        // Remove last (now duplicate) slot
        char lastIdKey[12], lastSecKey[14];
        snprintf(lastIdKey,  sizeof(lastIdKey),  "p_id_%u",     count - 1);
        snprintf(lastSecKey, sizeof(lastSecKey), "p_secret_%u", count - 1);
        prefs.remove(lastIdKey);
        prefs.remove(lastSecKey);

        prefs.putUChar("p_count", count - 1);
        prefs.end();

        ESP_LOGI(PHONETAG, "Phone removed: %s", deviceId);
        hasPairedPhones = (count - 1) > 0;
        return true;
    }

    bool readPhones() {
        Preferences prefs;
        prefs.begin(PHONE_NVS_NS, true);
        uint8_t count = prefs.getUChar("p_count", 0);

        for (uint8_t i = 0; i < count; i++) {
            char idKey[12];
            snprintf(idKey, sizeof(idKey), "p_id_%u", i);
            String id = prefs.getString(idKey, "");
            ESP_LOGD(PHONETAG, "Paired phone #%u: %s", i, id.c_str());
        }

        prefs.end();
        hasPairedPhones = (count > 0);
        if (!hasPairedPhones) ESP_LOGI(PHONETAG, "No paired phones in NVS");
        return hasPairedPhones;
    }

private:
    // Returns index of phone in NVS, or -1 if not found
    // Requires already-open Preferences handle
    int _findPhone(Preferences& prefs, const char* deviceId, uint8_t count) {
        for (uint8_t i = 0; i < count; i++) {
            char idKey[12];
            snprintf(idKey, sizeof(idKey), "p_id_%u", i);
            String stored = prefs.getString(idKey, "");
            if (stored == deviceId) return i;
        }
        return -1;
    }
};