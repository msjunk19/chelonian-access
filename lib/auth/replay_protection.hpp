#pragma once
#include <Arduino.h>
#include <cstring>
#include <esp_random.h>
#include <esp_log.h>

#define MAX_NONCES 20
#define NONCE_EXPIRY_MS 120000  // 2 minutes

static const char* REPLAY_TAG = "REPLAY";

struct NonceEntry {
    uint8_t nonce[16];
    uint32_t timestamp;
    bool used;
};

class ReplayProtector {
private:
    NonceEntry nonces[MAX_NONCES];
    uint8_t nonce_index = 0;

public:
    void init() {
        memset(nonces, 0, sizeof(nonces));
        ESP_LOGI(REPLAY_TAG, "Replay protector initialized with %d nonce slots", MAX_NONCES);
    }

    // Generate a fresh nonce for the phone to use
    void generateNonce(uint8_t* nonceOut) {
        esp_fill_random(nonceOut, 16);
    }

    // Validate that a nonce hasn't been used before
    bool validateAndConsumeNonce(const uint8_t* nonceIn) {
        uint32_t now = millis();

        // First, check if this nonce has been used
        for (int i = 0; i < MAX_NONCES; i++) {
            if (nonces[i].used && memcmp(nonces[i].nonce, nonceIn, 16) == 0) {
                // Check if it's still valid (within expiry window)
                if (now - nonces[i].timestamp < NONCE_EXPIRY_MS) {
                    ESP_LOGW(REPLAY_TAG, "Nonce replay detected! (slot %d)", i);
                    return false;  // Replay attack!
                } else {
                    // Expired, can be reused
                    nonces[i].used = false;
                    ESP_LOGD(REPLAY_TAG, "Expired nonce slot %d freed", i);
                }
            }
        }

        // Store this nonce as "used"
        nonces[nonce_index].used = true;
        memcpy(nonces[nonce_index].nonce, nonceIn, 16);
        nonces[nonce_index].timestamp = now;
        
        ESP_LOGD(REPLAY_TAG, "Nonce consumed and stored in slot %d", nonce_index);
        
        nonce_index = (nonce_index + 1) % MAX_NONCES;

        return true;  // Valid, not a replay
    }

    // Clean up expired nonces (call periodically)
    void cleanup() {
        uint32_t now = millis();
        int cleaned = 0;
        
        for (int i = 0; i < MAX_NONCES; i++) {
            if (nonces[i].used && (now - nonces[i].timestamp > NONCE_EXPIRY_MS)) {
                nonces[i].used = false;
                cleaned++;
            }
        }
        
        if (cleaned > 0) {
            ESP_LOGD(REPLAY_TAG, "Cleaned up %d expired nonces", cleaned);
        }
    }

    // Get current usage stats (for debugging)
    uint8_t getActiveNonceCount() const {
        uint8_t count = 0;
        uint32_t now = millis();
        
        for (int i = 0; i < MAX_NONCES; i++) {
            if (nonces[i].used && (now - nonces[i].timestamp < NONCE_EXPIRY_MS)) {
                count++;
            }
        }
        
        return count;
    }
};

// Global instance - defined in replay_protection.cpp
extern ReplayProtector replayProtector;