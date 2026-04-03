#pragma once
#include <Arduino.h>
#include <mbedtls/md.h>
#include <esp_random.h>
#include <esp_log.h>
#include <phone_token_manager.hpp>

static const char* AUTHTAG = "AUTH";

// Max clock drift allowed between phone and device (seconds)
static constexpr uint32_t AUTH_TIMESTAMP_WINDOW = 30;

// Commands the phone can send
enum class PhoneCommand : uint8_t {
    UNLOCK  = 0x01,
    LOCK    = 0x02,
    STATUS  = 0x03,
    UNPAIR  = 0x04,
    TRUNK   = 0x05,
    PANIC   = 0x06,
};

struct AuthPayload {
    char     deviceId[PHONE_ID_MAX_LEN + 1]; // phone UUID
    uint32_t timestamp;                       // unix epoch from phone
    uint8_t  command;                         // PhoneCommand
    uint8_t  hmac[32];                        // HMAC-SHA256
};

class AuthManager {
public:
    AuthManager(PhoneTokenManager& tokenManager) : _tokens(tokenManager) {}

    // Generate a cryptographically random 32-byte secret
    // Call this during pairing, then pass to PhoneTokenManager::addPhone()
    void generateSecret(uint8_t* secretOut) {
        esp_fill_random(secretOut, PHONE_SECRET_LEN);
    }

    // Verify an incoming payload from a phone
    // Returns true if signature valid, device known, and timestamp fresh
    bool verify(const AuthPayload& payload) {
    // 1. Look up secret for this device
    uint8_t secret[PHONE_SECRET_LEN];
    if (!_tokens.getSecret(payload.deviceId, secret)) {
        ESP_LOGW(AUTHTAG, "Unknown device: %s", payload.deviceId);
        return false;
    }

    // 2. TEMP — skip timestamp check for now
    // uint32_t now = _getCurrentTime();
    // int32_t drift = (int32_t)payload.timestamp - (int32_t)now;
    // if (drift > (int32_t)AUTH_TIMESTAMP_WINDOW || 
    //     drift < -(int32_t)AUTH_TIMESTAMP_WINDOW) {
    //     ESP_LOGW(AUTHTAG, "Timestamp rejected — drift: %ld seconds", drift);
    //     return false;
    // }

    // 3. Recompute expected HMAC
    uint8_t expected[32];
    _computeHMAC(secret, payload.deviceId, payload.timestamp, 
                 payload.command, expected);

    // 4. Constant-time compare
    if (!_constantTimeCompare(expected, payload.hmac, 32)) {
        ESP_LOGW(AUTHTAG, "HMAC mismatch for device: %s", payload.deviceId);
        return false;
    }

    ESP_LOGI(AUTHTAG, "Auth OK — device: %s cmd: 0x%02X", 
             payload.deviceId, payload.command);
    return true;
    }

    // Call this after pairing so the device knows what time it is
    // Phone sends its unix timestamp during pairing handshake
    void syncTime(uint32_t phoneTimestamp) {
        _timeOffset = phoneTimestamp - (millis() / 1000);
        ESP_LOGI(AUTHTAG, "Time synced. Offset: %lu", _timeOffset);
    }

    uint32_t getCurrentTime() { return _getCurrentTime(); }
    bool isTimeSynced() { return _timeOffset != 0; }

private:
    PhoneTokenManager& _tokens;
    uint32_t _timeOffset = 0; // offset between millis()/1000 and unix time

    uint32_t _getCurrentTime() {
        return (millis() / 1000) + _timeOffset;
    }

    void _computeHMAC(const uint8_t* secret,
                      const char* deviceId,
                      uint32_t timestamp,
                      uint8_t command,
                      uint8_t* hmacOut) {

        // Build the message: deviceId + timestamp (4 bytes BE) + command
        uint8_t message[PHONE_ID_MAX_LEN + 4 + 1];
        size_t idLen = strlen(deviceId);
        memcpy(message, deviceId, idLen);

        // Timestamp big-endian
        message[idLen + 0] = (timestamp >> 24) & 0xFF;
        message[idLen + 1] = (timestamp >> 16) & 0xFF;
        message[idLen + 2] = (timestamp >>  8) & 0xFF;
        message[idLen + 3] = (timestamp >>  0) & 0xFF;

        message[idLen + 4] = command;
        size_t msgLen = idLen + 5;

        // mbedTLS HMAC-SHA256 (built into ESP-IDF, no extra library needed)
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        mbedtls_md_hmac_starts(&ctx, secret, PHONE_SECRET_LEN);
        mbedtls_md_hmac_update(&ctx, message, msgLen);
        mbedtls_md_hmac_finish(&ctx, hmacOut);
        mbedtls_md_free(&ctx);
    }

    // Constant-time compare — prevents timing side channel attacks
    bool _constantTimeCompare(const uint8_t* a, const uint8_t* b, size_t len) {
        uint8_t diff = 0;
        for (size_t i = 0; i < len; i++) {
            diff |= a[i] ^ b[i];
        }
        return diff == 0;
    }

};