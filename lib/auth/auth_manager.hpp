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
        if (_timeSynced)
            return;

        uint32_t elapsed = (millis() - _bootMillis) / 1000;
        uint32_t bootEpoch = phoneEpoch - elapsed;

        struct timeval tv;
        tv.tv_sec = phoneEpoch;
        tv.tv_usec = 0;

        settimeofday(&tv, nullptr);

        // int8_t bootIdx = accessLogger.getBootLogIndex();
        int8_t bootIdx = accessLogger.findBootLogIndex();

        if (bootIdx >= 0)
        {
            accessLogger.updateEntryTimestamp(bootIdx, bootEpoch);
        }

        _timeSynced = true;

        ESP_LOGI(AUTHTAG, "System time synced: %lu", phoneEpoch);
    }

    // void syncTime(uint32_t phoneEpoch)
    // {
    //     if (_timeSynced) {
    //         ESP_LOGI(AUTHTAG, "Time already synced");
    //         return;
    //     }

    //     uint32_t elapsed = (millis() - _bootMillis) / 1000;
    //     uint32_t bootEpoch = phoneEpoch - elapsed;

    //     ESP_LOGI(AUTHTAG,
    //              "Time sync: phone=%lu elapsed=%lus bootEpoch=%lu",
    //              phoneEpoch,
    //              elapsed,
    //              bootEpoch);

    //     /* find newest boot placeholder */

    //     // int8_t bootIdx = accessLogger.findLatestBootPlaceholder();
    //     int8_t bootIdx = accessLogger.getBootLogIndex();

    //     if (bootIdx >= 0) {

    //         accessLogger.updateEntryTimestamp(bootIdx, bootEpoch);

    //         ESP_LOGI(AUTHTAG,
    //                  "Boot log corrected index=%d -> %lu",
    //                  bootIdx,
    //                  bootEpoch);
    //     }
    //     else {
    //         // ESP_LOGW(AUTHTAG, "No boot placeholder found");
    //         ESP_LOGW(AUTHTAG, "Boot log index not found");
    //     }

    //     /* set system time */

    //     struct timeval tv;
    //     tv.tv_sec  = phoneEpoch;
    //     tv.tv_usec = 0;

    //     settimeofday(&tv, nullptr);

    //     _timeSynced = true;

    //     ESP_LOGI(AUTHTAG, "Boot index = %d", accessLogger.getBootLogIndex());
    //     ESP_LOGI(AUTHTAG, "Log count = %d", accessLogger.getCount());

    // }

    /* ---------------- RESTORE ---------------- */

    void restoreTime()
    {
        // Every boot starts unsynced
        _timeSynced = false;
        ESP_LOGI(AUTHTAG, "Waiting for time sync");
    }

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


// #pragma once
// #include <Arduino.h>
// #include <Preferences.h>
// #include <sys/time.h>
// #include <mbedtls/md.h>
// #include <esp_random.h>
// #include <esp_log.h>
// #include <phone_token_manager.hpp>
// #include <access_log.hpp>

// static const char* AUTHTAG = "AUTH";

// static constexpr uint32_t AUTH_TIMESTAMP_WINDOW = 30;

// enum class PhoneCommand : uint8_t {
//     UNLOCK  = 0x01,
//     LOCK    = 0x02,
//     STATUS  = 0x03,
//     UNPAIR  = 0x04,
//     TRUNK   = 0x05,
//     PANIC   = 0x06,
// };

// struct AuthPayload {
//     char     deviceId[PHONE_ID_MAX_LEN + 1];
//     uint32_t timestamp;
//     uint8_t  command;
//     uint8_t  hmac[32];
// };

// class AuthManager {
// public:
//     AuthManager(PhoneTokenManager& tokenManager) : _tokens(tokenManager) {}

//     void generateSecret(uint8_t* secretOut) {
//         esp_fill_random(secretOut, PHONE_SECRET_LEN);
//     }

//     bool verify(const AuthPayload& payload) {
//         uint8_t secret[PHONE_SECRET_LEN];
//         if (!_tokens.getSecret(payload.deviceId, secret)) {
//             ESP_LOGW(AUTHTAG, "Unknown device: %s", payload.deviceId);
//             return false;
//         }

//         uint8_t expected[32];
//         _computeHMAC(secret, payload.deviceId, payload.timestamp, 
//                      payload.command, expected);

//         if (!_constantTimeCompare(expected, payload.hmac, 32)) {
//             ESP_LOGW(AUTHTAG, "HMAC mismatch for device: %s", payload.deviceId);
//             return false;
//         }

//         ESP_LOGI(AUTHTAG, "Auth OK — device: %s cmd: 0x%02X", 
//                  payload.deviceId, payload.command);
//         return true;
//     }

//     void syncTime(uint32_t phoneTimestamp) {
//         // Calculate what the epoch was at boot time
//         uint32_t elapsed_s = millis() / 1000;
//         uint32_t boot_timestamp = phoneTimestamp - elapsed_s;
        
//         Serial.println("[TIME] syncTime: phone=" + String(phoneTimestamp) + ", elapsed=" + String(elapsed_s) + ", boot_ts=" + String(boot_timestamp));
//         ESP_LOGI(AUTHTAG, "Time synced: boot_ts=%lu (phone=%lu - elapsed=%lus)", boot_timestamp, phoneTimestamp, elapsed_s);
        
//         // Find boot log entry and update its timestamp to the correct epoch
//         int8_t bootIdx = accessLogger.findBootLogIndex();
//         if (bootIdx >= 0) {
//             accessLogger.updateEntryTimestamp(bootIdx, boot_timestamp);
//             ESP_LOGI(AUTHTAG, "Updated boot log timestamp to %lu", boot_timestamp);
//         }
        
//         // Set system time to phone time
//         struct timeval tv = { (time_t)phoneTimestamp, 0 };
//         settimeofday(&tv, nullptr);
        
//         // Mark as synced
//         Preferences prefs;
//         prefs.begin("auth", false);
//         prefs.putBool("time_synced", true);
//         prefs.end();
//     }

//     void restoreTime() {
//         Preferences prefs;
//         prefs.begin("auth", true);
//         bool synced = prefs.getBool("time_synced", false);
//         prefs.end();

//         if (!synced) {
//             ESP_LOGI(AUTHTAG, "Waiting for first time sync");
//         }
//     }
         
//     uint32_t getCurrentTime() {
//         struct timeval tv;
//         gettimeofday(&tv, nullptr);
//         return (uint32_t)tv.tv_sec;
//     }

//     bool isTimeSynced() {
//         Preferences prefs;
//         prefs.begin("auth", true);
//         bool synced = prefs.getBool("time_synced", false);
//         prefs.end();
//         return synced;
//     }

// private:
//     PhoneTokenManager& _tokens;

//     void _computeHMAC(const uint8_t* secret,
//                       const char* deviceId,
//                       uint32_t timestamp,
//                       uint8_t command,
//                       uint8_t* hmacOut) {
//         uint8_t message[PHONE_ID_MAX_LEN + 4 + 1];
//         size_t idLen = strlen(deviceId);
//         memcpy(message, deviceId, idLen);

//         message[idLen + 0] = (timestamp >> 24) & 0xFF;
//         message[idLen + 1] = (timestamp >> 16) & 0xFF;
//         message[idLen + 2] = (timestamp >>  8) & 0xFF;
//         message[idLen + 3] = (timestamp >>  0) & 0xFF;

//         message[idLen + 4] = command;
//         size_t msgLen = idLen + 5;

//         mbedtls_md_context_t ctx;
//         mbedtls_md_init(&ctx);
//         mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
//         mbedtls_md_hmac_starts(&ctx, secret, PHONE_SECRET_LEN);
//         mbedtls_md_hmac_update(&ctx, message, msgLen);
//         mbedtls_md_hmac_finish(&ctx, hmacOut);
//         mbedtls_md_free(&ctx);
//     }

//     bool _constantTimeCompare(const uint8_t* a, const uint8_t* b, size_t len) {
//         uint8_t diff = 0;
//         for (size_t i = 0; i < len; i++) {
//             diff |= a[i] ^ b[i];
//         }
//         return diff == 0;
//     }
// };