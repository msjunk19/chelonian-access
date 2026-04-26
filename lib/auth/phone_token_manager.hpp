#pragma once
#include <Preferences.h>
#include <config.hpp>
#include "esp_log.h"
#include "encrypted_token_storage.hpp"

static const char* PHONETAG = "PHONETOKEN";
static const char* PHONE_NVS_NS = "phone_tokens";

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

    /**
     * Add phone with encrypted token storage
     * Uses AES-256-GCM for authenticated encryption
     */
    bool addPhoneEncrypted(const char* deviceId, const uint8_t* encryptedToken) {
        if (!deviceId || strlen(deviceId) == 0 || strlen(deviceId) > PHONE_ID_MAX_LEN) {
            ESP_LOGE(PHONETAG, "Invalid device ID");
            return false;
        }

        if (!encryptedToken) {
            ESP_LOGE(PHONETAG, "Null encrypted token");
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

        // Store encrypted token blob
        // Format: [IV(12) | Ciphertext(32) | Tag(16)] = 60 bytes
        char tokKey[14];
        snprintf(tokKey, sizeof(tokKey), "p_token_%u", count);
        size_t encSize = EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE;
        prefs.putBytes(tokKey, encryptedToken, encSize);

        // Store version flag (v2 = encrypted)
        char verKey[12];
        snprintf(verKey, sizeof(verKey), "p_ver_%u", count);
        prefs.putUChar(verKey, 2);  // Version 2 = encrypted

        prefs.putUChar("p_count", count + 1);
        prefs.end();

        ESP_LOGI(PHONETAG, "Phone paired (encrypted): %s (slot %u)", deviceId, count);
        hasPairedPhones = true;
        return true;
    }

    /**
     * Get encrypted token for a device ID
     * Returns decrypted token in secretOut
     */
    bool getPhoneEncrypted(const char* deviceId, uint8_t* encryptedTokenOut) {
        if (!deviceId || !encryptedTokenOut) return false;

        Preferences prefs;
        prefs.begin(PHONE_NVS_NS, true);
        uint8_t count = prefs.getUChar("p_count", 0);

        int idx = _findPhone(prefs, deviceId, count);
        if (idx < 0) {
            prefs.end();
            ESP_LOGW(PHONETAG, "Phone not found: %s", deviceId);
            return false;
        }

        // Retrieve encrypted token blob
        char tokKey[14];
        snprintf(tokKey, sizeof(tokKey), "p_token_%u", idx);
        size_t encSize = EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE;
        size_t read = prefs.getBytes(tokKey, encryptedTokenOut, encSize);
        prefs.end();

        if (read != encSize) {
            ESP_LOGE(PHONETAG, "Failed to read encrypted token (read %d, expected %d)", read, encSize);
            return false;
        }

        return true;
    }


    /**
     * Retrieve and decrypt the secret for a given device ID
     */
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

        // Retrieve encrypted token
        uint8_t encryptedToken[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
        char tokKey[14];
        snprintf(tokKey, sizeof(tokKey), "p_token_%u", idx);
        size_t encSize = EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE;
        size_t read = prefs.getBytes(tokKey, encryptedToken, encSize);
        prefs.end();

        if (read != encSize) {
            ESP_LOGE(PHONETAG, "Failed to read encrypted token for %s", deviceId);
            return false;
        }

        // Decrypt
        extern EncryptedTokenStorage encryptedStorage;
        if (!encryptedStorage.decryptToken(encryptedToken, secretOut)) {
            ESP_LOGE(PHONETAG, "Failed to decrypt token for %s", deviceId);
            return false;
        }

        return true;
    }

    // /**
    //  * Retrieve and decrypt the secret for a given device ID
    //  * (For backward compatibility and testing)
    //  */
    // bool getSecret(const char* deviceId, uint8_t* secretOut) {
    //     if (!deviceId || !secretOut) return false;

    //     Preferences prefs;
    //     prefs.begin(PHONE_NVS_NS, true);
    //     uint8_t count = prefs.getUChar("p_count", 0);

    //     int idx = _findPhone(prefs, deviceId, count);
    //     if (idx < 0) {
    //         prefs.end();
    //         ESP_LOGW(PHONETAG, "Phone not found: %s", deviceId);
    //         return false;
    //     }

    //     // Check version
    //     char verKey[12];
    //     snprintf(verKey, sizeof(verKey), "p_ver_%u", idx);
    //     uint8_t version = prefs.getUChar(verKey, 1);  // Default to v1 for old entries

    //     if (version == 2) {
    //         // Encrypted token - retrieve and decrypt
    //         uint8_t encryptedToken[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
    //         char tokKey[14];
    //         snprintf(tokKey, sizeof(tokKey), "p_token_%u", idx);
    //         size_t encSize = EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE;
    //         size_t read = prefs.getBytes(tokKey, encryptedToken, encSize);
    //         prefs.end();

    //         if (read != encSize) {
    //             ESP_LOGE(PHONETAG, "Failed to read encrypted token");
    //             return false;
    //         }

    //         // Decrypt
    //         extern EncryptedTokenStorage encryptedStorage;
    //         if (!encryptedStorage.decryptToken(encryptedToken, secretOut)) {
    //             ESP_LOGE(PHONETAG, "Failed to decrypt token for %s", deviceId);
    //             return false;
    //         }

    //         return true;
    //     } else {
    //         // Legacy plaintext token (migration path)
    //         ESP_LOGW(PHONETAG, "Using legacy plaintext token for %s - should encrypt during next boot", deviceId);
    //         char secKey[14];
    //         snprintf(secKey, sizeof(secKey), "p_secret_%u", idx);
    //         size_t read = prefs.getBytes(secKey, secretOut, PHONE_SECRET_LEN);
    //         prefs.end();

    //         return (read == PHONE_SECRET_LEN);
    //     }
    // }

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
            char srcTokKey[14], dstTokKey[14];
            char srcVerKey[12], dstVerKey[12];

            snprintf(srcIdKey, sizeof(srcIdKey), "p_id_%u", i + 1);
            snprintf(dstIdKey, sizeof(dstIdKey), "p_id_%u", i);
            snprintf(srcTokKey, sizeof(srcTokKey), "p_token_%u", i + 1);
            snprintf(dstTokKey, sizeof(dstTokKey), "p_token_%u", i);
            snprintf(srcVerKey, sizeof(srcVerKey), "p_ver_%u", i + 1);
            snprintf(dstVerKey, sizeof(dstVerKey), "p_ver_%u", i);

            // Shift ID
            String idStr = prefs.getString(srcIdKey, "");
            prefs.putString(dstIdKey, idStr);

            // Shift encrypted token
            uint8_t tokBuf[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
            size_t tokSize = EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE;
            size_t read = prefs.getBytes(srcTokKey, tokBuf, tokSize);
            if (read > 0) {
                prefs.putBytes(dstTokKey, tokBuf, read);
            }

            // Shift version
            uint8_t version = prefs.getUChar(srcVerKey, 1);
            prefs.putUChar(dstVerKey, version);
        }

        // Remove last (now duplicate) slot
        char lastIdKey[12], lastTokKey[14], lastVerKey[12];
        snprintf(lastIdKey, sizeof(lastIdKey), "p_id_%u", count - 1);
        snprintf(lastTokKey, sizeof(lastTokKey), "p_token_%u", count - 1);
        snprintf(lastVerKey, sizeof(lastVerKey), "p_ver_%u", count - 1);
        prefs.remove(lastIdKey);
        prefs.remove(lastTokKey);
        prefs.remove(lastVerKey);

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

            char verKey[12];
            snprintf(verKey, sizeof(verKey), "p_ver_%u", i);
            uint8_t version = prefs.getUChar(verKey, 1);

            const char* encStatus = (version == 2) ? "encrypted" : "plaintext";
            ESP_LOGD(PHONETAG, "Paired phone #%u: %s (%s)", i, id.c_str(), encStatus);
        }

        prefs.end();
        hasPairedPhones = (count > 0);
        if (!hasPairedPhones) ESP_LOGI(PHONETAG, "No paired phones in NVS");
        return hasPairedPhones;
    }

private:
    // Returns index of phone in NVS, or -1 if not found
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