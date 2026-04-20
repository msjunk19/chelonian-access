#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/sha256.h>
#include <mbedtls/cipher.h>
#include "esp_log.h"
#include "esp_random.h"
#include <config.hpp>

static const char* CRYPTO_TAG = "CRYPTO";

/**
 * Encrypted token storage using AES-256-GCM
 * Provides authenticated encryption for sensitive tokens
 */
class EncryptedTokenStorage {
public:
    static constexpr uint8_t KEY_SIZE = 32;      // 256-bit key
    static constexpr uint8_t IV_SIZE = 12;       // 96-bit IV (standard for GCM)
    static constexpr uint8_t TAG_SIZE = 16;      // 128-bit authentication tag
    static constexpr uint8_t ENCRYPTED_SIZE = PHONE_SECRET_LEN + TAG_SIZE;

    EncryptedTokenStorage() : keyInitialized(false) {
        memset(masterKey, 0, KEY_SIZE);
    }

    /**
     * Initialize with device master key
     * Derives key from device ID or uses stored key
     */
    bool begin() {
        if (!deriveDeviceKey()) {
            ESP_LOGE(CRYPTO_TAG, "Failed to derive device key");
            return false;
        }

        // DEBUG: Log derived key
        ESP_LOGI(CRYPTO_TAG, "Derived master key: ");
        for (int i = 0; i < 32; i++) {
            printf("%02x", masterKey[i]);
        }
        printf("\n");

        keyInitialized = true;
        ESP_LOGI(CRYPTO_TAG, "Encrypted token storage initialized");
        return true;
    }

    /**
     * Encrypt a token for storage
     * Returns: encrypted token (PHONE_SECRET_LEN + TAG_SIZE bytes)
     */
    bool encryptToken(const uint8_t* plainToken, uint8_t* encryptedOut) {
        if (!keyInitialized) {
            ESP_LOGE(CRYPTO_TAG, "Key not initialized");
            return false;
        }

        // Generate random IV
        uint8_t iv[IV_SIZE];
        esp_fill_random(iv, IV_SIZE);

        // Prepare for encryption
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);

        int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, masterKey, KEY_SIZE * 8);
        if (ret != 0) {
            ESP_LOGE(CRYPTO_TAG, "GCM setkey failed: %d", ret);
            mbedtls_gcm_free(&gcm);
            return false;
        }

        // Encrypt token
        uint8_t* ciphertext = encryptedOut;
        uint8_t* tag = encryptedOut + PHONE_SECRET_LEN;

        ret = mbedtls_gcm_crypt_and_tag(
            &gcm,
            MBEDTLS_GCM_ENCRYPT,
            PHONE_SECRET_LEN,
            iv, IV_SIZE,
            nullptr, 0,              // No additional data
            plainToken,
            ciphertext,
            TAG_SIZE,
            tag
        );

        mbedtls_gcm_free(&gcm);

        if (ret != 0) {
            ESP_LOGE(CRYPTO_TAG, "Encryption failed: %d", ret);
            return false;
        }

        // Prepend IV to encrypted output: [IV(12) | Ciphertext(32) | Tag(16)]
        // But we need to return in format that fits in ENCRYPTED_SIZE
        // So store: [Ciphertext(32) | Tag(16)] and store IV separately
        memmove(encryptedOut + IV_SIZE, encryptedOut, PHONE_SECRET_LEN + TAG_SIZE);
        memcpy(encryptedOut, iv, IV_SIZE);

        return true;
    }

    /**
     * Decrypt a token from storage
     * Expects: [IV(12) | Ciphertext(32) | Tag(16)]
     */
    bool decryptToken(const uint8_t* encrypted, uint8_t* decryptedOut) {
        if (!keyInitialized) {
            ESP_LOGE(CRYPTO_TAG, "Key not initialized");
            return false;
        }

        const uint8_t* iv = encrypted;
        const uint8_t* ciphertext = encrypted + IV_SIZE;
        const uint8_t* tag = encrypted + IV_SIZE + PHONE_SECRET_LEN;

        // Prepare for decryption
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);

        int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, masterKey, KEY_SIZE * 8);
        if (ret != 0) {
            ESP_LOGE(CRYPTO_TAG, "GCM setkey failed: %d", ret);
            mbedtls_gcm_free(&gcm);
            return false;
        }

        // Decrypt and verify
        ret = mbedtls_gcm_auth_decrypt(
            &gcm,
            PHONE_SECRET_LEN,
            iv, IV_SIZE,
            nullptr, 0,              // No additional data
            (uint8_t*)tag, TAG_SIZE,
            (uint8_t*)ciphertext,
            decryptedOut
        );

        mbedtls_gcm_free(&gcm);

        if (ret != 0) {
            ESP_LOGE(CRYPTO_TAG, "Decryption/authentication failed: %d", ret);
            return false;
        }

        return true;
    }

    /**
     * Get total encrypted size (IV + ciphertext + tag)
     */
    static uint16_t getEncryptedSize() {
        return IV_SIZE + PHONE_SECRET_LEN + TAG_SIZE;
    }

private:
    uint8_t masterKey[KEY_SIZE];
    bool keyInitialized;

    /**
     * Derive device key from hardware identifiers
     * This should use secure storage in production
     */
    bool deriveDeviceKey() {
        uint8_t seed[48];
        uint8_t pos = 0;

        // Get chip ID
        uint32_t chipId = ESP.getEfuseMac();
        memcpy(seed + pos, &chipId, 4);
        pos += 4;

        // Get MAC address
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        memcpy(seed + pos, mac, 6);
        pos += 6;

        // Get some random data to add entropy
        esp_fill_random(seed + pos, 48 - pos);

        // Derive key using SHA256
        // For now, use a simpler approach: hash the seed
        mbedtls_sha256((const unsigned char*)seed, sizeof(seed), masterKey, 0);
        // 0 = SHA-256, 1 = SHA-224
        return true;
    }
};

// Global encrypted storage instance
extern EncryptedTokenStorage encryptedStorage;