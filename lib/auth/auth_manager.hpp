#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <sys/time.h>
#include <mbedtls/md.h>
#include <esp_random.h>
#include <esp_log.h>
#include <phone_token_manager.hpp>

static const char* AUTHTAG = "AUTH";

static constexpr uint32_t AUTH_TIMESTAMP_WINDOW = 30;

enum class PhoneCommand : uint8_t {
    UNLOCK  = 0x01,
    LOCK    = 0x02,
    STATUS  = 0x03,
    UNPAIR  = 0x04,
    TRUNK   = 0x05,
    PANIC   = 0x06,
};

struct AuthPayload {
    char     deviceId[PHONE_ID_MAX_LEN + 1];
    uint32_t timestamp;
    uint8_t  command;
    uint8_t  hmac[32];
};

class AuthManager {
public:
    AuthManager(PhoneTokenManager& tokenManager) : _tokens(tokenManager) {}

    void generateSecret(uint8_t* secretOut) {
        esp_fill_random(secretOut, PHONE_SECRET_LEN);
    }

    bool verify(const AuthPayload& payload) {
        uint8_t secret[PHONE_SECRET_LEN];
        if (!_tokens.getSecret(payload.deviceId, secret)) {
            ESP_LOGW(AUTHTAG, "Unknown device: %s", payload.deviceId);
            return false;
        }

        uint8_t expected[32];
        _computeHMAC(secret, payload.deviceId, payload.timestamp, 
                     payload.command, expected);

        if (!_constantTimeCompare(expected, payload.hmac, 32)) {
            ESP_LOGW(AUTHTAG, "HMAC mismatch for device: %s", payload.deviceId);
            return false;
        }

        ESP_LOGI(AUTHTAG, "Auth OK — device: %s cmd: 0x%02X", 
                 payload.deviceId, payload.command);
        return true;
    }

    void syncTime(uint32_t phoneTimestamp) {
        // Calculate epoch time at boot based on current millis
        Preferences prefs;
        prefs.begin("auth", true);
        uint32_t boot_ms = prefs.getUInt("boot_ms", 0);
        prefs.end();
        
        if (boot_ms > 0) {
            uint32_t current_ms = millis();
            uint32_t elapsed_s = (current_ms >= boot_ms) ? (current_ms - boot_ms) / 1000 : 0;
            uint32_t epoch_at_boot = phoneTimestamp - elapsed_s;
            
            prefs.begin("auth", false);
            prefs.putUInt("saved_epoch", epoch_at_boot); // Store corrected boot time
            prefs.putUInt("boot_ms", 0); // Clear boot_ms marker
            prefs.end();
            
            ESP_LOGI(AUTHTAG, "Time synced: phone=%lu, boot_epoch=%lu (elapsed %lus)", 
                phoneTimestamp, epoch_at_boot, elapsed_s);
        } else {
            // First sync ever or boot_ms was cleared
            prefs.begin("auth", false);
            prefs.putUInt("saved_epoch", phoneTimestamp);
            prefs.putULong("saved_ms", millis());
            prefs.end();
            
            ESP_LOGI(AUTHTAG, "Time synced from phone: %lu", phoneTimestamp);
        }
        
        // Set system time
        struct timeval tv = { (time_t)phoneTimestamp, 0 };
        settimeofday(&tv, nullptr);
    }

    void restoreTime() {
        Preferences prefs;
        prefs.begin("auth", true);
        uint32_t boot_ms = prefs.getUInt("boot_ms", 0);
        prefs.end();

        if (boot_ms > 0 && millis() >= boot_ms) {
            uint32_t elapsed_s = (millis() - boot_ms) / 1000;
            
            // Can't restore accurate time without phone sync
            // Just log that we're waiting for sync
            ESP_LOGI(AUTHTAG, "Time waiting for sync (elapsed %lu seconds since boot)", elapsed_s);
        } else {
            ESP_LOGI(AUTHTAG, "No boot_ms found, waiting for phone time sync");
        }
    }

    uint32_t getCurrentTime() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (uint32_t)tv.tv_sec;
    }

    bool isTimeSynced() {
        Preferences prefs;
        prefs.begin("auth", true);
        uint32_t saved = prefs.getUInt("saved_epoch", 0);
        prefs.end();
        return saved != 0;
    }

private:
    PhoneTokenManager& _tokens;

    void _computeHMAC(const uint8_t* secret,
                      const char* deviceId,
                      uint32_t timestamp,
                      uint8_t command,
                      uint8_t* hmacOut) {
        uint8_t message[PHONE_ID_MAX_LEN + 4 + 1];
        size_t idLen = strlen(deviceId);
        memcpy(message, deviceId, idLen);

        message[idLen + 0] = (timestamp >> 24) & 0xFF;
        message[idLen + 1] = (timestamp >> 16) & 0xFF;
        message[idLen + 2] = (timestamp >>  8) & 0xFF;
        message[idLen + 3] = (timestamp >>  0) & 0xFF;

        message[idLen + 4] = command;
        size_t msgLen = idLen + 5;

        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        mbedtls_md_hmac_starts(&ctx, secret, PHONE_SECRET_LEN);
        mbedtls_md_hmac_update(&ctx, message, msgLen);
        mbedtls_md_hmac_finish(&ctx, hmacOut);
        mbedtls_md_free(&ctx);
    }

    bool _constantTimeCompare(const uint8_t* a, const uint8_t* b, size_t len) {
        uint8_t diff = 0;
        for (size_t i = 0; i < len; i++) {
            diff |= a[i] ^ b[i];
        }
        return diff == 0;
    }
};