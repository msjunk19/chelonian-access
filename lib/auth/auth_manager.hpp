#pragma once
#include <Arduino.h>
#include <sys/time.h>
#include <mbedtls/md.h>
#include <esp_random.h>
#include <esp_log.h>

#include <phone_token_manager.hpp>
#include <config.hpp>
#include <access_log.hpp>

static const char* AUTHTAG = "AUTH";

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

    AuthManager(PhoneTokenManager& tokenManager)
        : _tokens(tokenManager)
    {
        _bootMillis = millis();
    }

    /* ---------------- SECRET GENERATION ---------------- */

    void generateSecret(uint8_t* secretOut) {
        esp_fill_random(secretOut, PHONE_SECRET_LEN);
    }

    /* ---------------- AUTH VERIFY ---------------- */

    bool verify(const AuthPayload& payload) {

        uint8_t secret[PHONE_SECRET_LEN];

        if (!_tokens.getSecret(payload.deviceId, secret)) {
            ESP_LOGW(AUTHTAG, "Unknown device: %s", payload.deviceId);
            return false;
        }

        if (_timeSynced) {

            uint32_t now = getCurrentTime();

            if (payload.timestamp > now + AUTH_TIMESTAMP_WINDOW ||
                payload.timestamp < now - AUTH_TIMESTAMP_WINDOW)
            {
                ESP_LOGW(AUTHTAG, "Timestamp outside window");
                return false;
            }
        }

        uint8_t expected[32];

        _computeHMAC(secret,
                     payload.deviceId,
                     payload.timestamp,
                     payload.command,
                     expected);

        if (!_constantTimeCompare(expected, payload.hmac, 32)) {
            ESP_LOGW(AUTHTAG, "HMAC mismatch for %s", payload.deviceId);
            return false;
        }

        ESP_LOGI(AUTHTAG,
                 "Auth OK device=%s cmd=0x%02X",
                 payload.deviceId,
                 payload.command);

        return true;
    }

    /* ---------------- TIME SYNC ---------------- */

    void syncTime(uint32_t phoneEpoch)
    {
        uint32_t elapsed = (millis() - _bootMillis) / 1000;
        uint32_t bootEpoch = phoneEpoch - elapsed;

        struct timeval tv;
        tv.tv_sec = phoneEpoch;
        tv.tv_usec = 0;

        settimeofday(&tv, nullptr);

        accessLogger.setSystemTime(phoneEpoch);

        _timeSynced = true;

        ESP_LOGI(AUTHTAG,
                "Time sync: phone=%lu elapsed=%lus",
                phoneEpoch,
                elapsed);
    }

    // /* ---------------- RESTORE ---------------- */

    // void restoreTime()
    // {
    //     // Every boot starts unsynced
    //     _timeSynced = false;
    //     ESP_LOGI(AUTHTAG, "Waiting for time sync");
    // }

    /* ---------------- UTILITIES ---------------- */

    uint32_t getCurrentTime() {

        struct timeval tv;
        gettimeofday(&tv, nullptr);

        return (uint32_t)tv.tv_sec;
    }

    bool isTimeSynced() {
        return _timeSynced;
    }

    
private:

    PhoneTokenManager& _tokens;

    uint32_t _bootMillis = 0;
    bool _timeSynced = false;

    /* ---------------- HMAC ---------------- */

    void _computeHMAC(const uint8_t* secret,
                      const char* deviceId,
                      uint32_t timestamp,
                      uint8_t command,
                      uint8_t* hmacOut)
    {
        uint8_t message[PHONE_ID_MAX_LEN + 5];

        size_t idLen = strlen(deviceId);

        memcpy(message, deviceId, idLen);

        message[idLen + 0] = (timestamp >> 24) & 0xFF;
        message[idLen + 1] = (timestamp >> 16) & 0xFF;
        message[idLen + 2] = (timestamp >> 8)  & 0xFF;
        message[idLen + 3] = (timestamp >> 0)  & 0xFF;
        message[idLen + 4] = command;

        size_t msgLen = idLen + 5;

        mbedtls_md_context_t ctx;

        mbedtls_md_init(&ctx);

        const mbedtls_md_info_t* info =
            mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

        mbedtls_md_setup(&ctx, info, 1);

        mbedtls_md_hmac_starts(&ctx, secret, PHONE_SECRET_LEN);
        mbedtls_md_hmac_update(&ctx, message, msgLen);
        mbedtls_md_hmac_finish(&ctx, hmacOut);

        mbedtls_md_free(&ctx);
    }

    /* ---------------- CONSTANT TIME COMPARE ---------------- */

    bool _constantTimeCompare(const uint8_t* a,
                              const uint8_t* b,
                              size_t len)
    {
        uint8_t diff = 0;

        for (size_t i = 0; i < len; i++) {
            diff |= a[i] ^ b[i];
        }

        return diff == 0;
    }
};
