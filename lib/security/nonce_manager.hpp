#pragma once
#include <Arduino.h>
#include <array>
#include <cstring>
#include "esp_log.h"
#include "esp_random.h"

static const char* REPLAY_TAG = "REPLAY";

/**
 * Nonce-based replay protection
 * Generates and validates single-use nonces
 */
class NonceManager {
public:
    static constexpr uint8_t NONCE_SIZE = 16;
    static constexpr uint8_t MAX_NONCES = 32;
    static constexpr uint32_t NONCE_EXPIRY_MS = 30000;  // 30 seconds (reduced from 2 minutes)
    static constexpr uint32_t CLEANUP_INTERVAL_MS = 5000;

    struct Nonce {
        uint8_t value[NONCE_SIZE];
        uint32_t timestamp;
        bool used;
        uint32_t usedAt;
    };

    NonceManager() : nextIndex(0), lastCleanup(0) {
        // Initialize all nonces to zero
        for (auto& nonce : nonces) {
            memset(&nonce, 0, sizeof(Nonce));
        }
    }

    /**
     * Generate a new nonce
     * Returns: pointer to 16-byte nonce
     */
    const uint8_t* generateNonce() {
        cleanupExpiredNonces();

        // Find next unused slot
        int slot = findUnusedSlot();
        if (slot < 0) {
            ESP_LOGW(REPLAY_TAG, "All nonce slots full, overwriting oldest");
            slot = nextIndex % MAX_NONCES;
        }

        Nonce& nonce = nonces[slot];
        esp_fill_random(nonce.value, NONCE_SIZE);
        nonce.timestamp = millis();
        nonce.used = false;
        nonce.usedAt = 0;

        nextIndex = (slot + 1) % MAX_NONCES;

        ESP_LOGD(REPLAY_TAG, "Generated nonce in slot %d", slot);
        return nonce.value;
    }

    /**
     * Validate and consume a nonce
     * Returns: true if valid and freshly used, false if invalid/expired/already used
     */
    bool validateAndConsume(const uint8_t* nonceValue) {
        if (!nonceValue) {
            ESP_LOGW(REPLAY_TAG, "Null nonce");
            return false;
        }

        uint32_t now = millis();
        cleanupExpiredNonces();

        for (uint8_t i = 0; i < MAX_NONCES; i++) {
            if (memcmp(nonces[i].value, nonceValue, NONCE_SIZE) == 0) {
                // Found matching nonce

                // Check if already used
                if (nonces[i].used) {
                    ESP_LOGW(REPLAY_TAG, "Nonce already used (at %lu, now %lu)", 
                             nonces[i].usedAt, now);
                    return false;
                }

                // Check if expired
                if (now - nonces[i].timestamp > NONCE_EXPIRY_MS) {
                    ESP_LOGW(REPLAY_TAG, "Nonce expired (age: %lu ms)", 
                             now - nonces[i].timestamp);
                    nonces[i].used = true;  // Mark to prevent reuse
                    nonces[i].usedAt = now;
                    return false;
                }

                // Valid - consume nonce
                nonces[i].used = true;
                nonces[i].usedAt = now;
                ESP_LOGI(REPLAY_TAG, "Nonce validated and consumed (age: %lu ms)", 
                         now - nonces[i].timestamp);
                return true;
            }
        }

        ESP_LOGW(REPLAY_TAG, "Nonce not found (invalid)");
        return false;
    }

    /**
     * Get statistics for monitoring
     */
    struct Stats {
        uint8_t activeNonces;
        uint8_t usedNonces;
        uint8_t expiredNonces;
    };

    Stats getStats() const {
        Stats stats = {0, 0, 0};
        uint32_t now = millis();

        for (uint8_t i = 0; i < MAX_NONCES; i++) {
            if (nonces[i].timestamp > 0) {
                if (nonces[i].used) {
                    stats.usedNonces++;
                } else if (now - nonces[i].timestamp > NONCE_EXPIRY_MS) {
                    stats.expiredNonces++;
                } else {
                    stats.activeNonces++;
                }
            }
        }

        return stats;
    }

    /**
     * Clear all nonces (for testing/reset)
     */
    void clear() {
        for (auto& nonce : nonces) {
            memset(&nonce, 0, sizeof(Nonce));
        }
        nextIndex = 0;
        lastCleanup = millis();
        ESP_LOGI(REPLAY_TAG, "All nonces cleared");
    }

private:
    std::array<Nonce, MAX_NONCES> nonces;
    uint8_t nextIndex;
    uint32_t lastCleanup;

    /**
     * Find first completely unused slot
     * Returns: index or -1 if none available
     */
    int findUnusedSlot() const {
        for (uint8_t i = 0; i < MAX_NONCES; i++) {
            if (nonces[i].timestamp == 0) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Clean up expired nonces periodically
     */
    void cleanupExpiredNonces() {
        uint32_t now = millis();

        // Only cleanup periodically to save CPU
        if (now - lastCleanup < CLEANUP_INTERVAL_MS) {
            return;
        }

        lastCleanup = now;
        uint8_t cleaned = 0;

        for (uint8_t i = 0; i < MAX_NONCES; i++) {
            if (nonces[i].timestamp > 0 && 
                (nonces[i].used || (now - nonces[i].timestamp > NONCE_EXPIRY_MS * 2))) {
                
                // Clear slot for reuse if:
                // - Already used and has been sitting for a while, OR
                // - Expired well beyond the window
                if ((nonces[i].used && (now - nonces[i].usedAt > NONCE_EXPIRY_MS)) ||
                    (now - nonces[i].timestamp > NONCE_EXPIRY_MS * 2)) {
                    
                    memset(&nonces[i], 0, sizeof(Nonce));
                    cleaned++;
                }
            }
        }

        if (cleaned > 0) {
            ESP_LOGD(REPLAY_TAG, "Cleaned up %d expired nonce slots", cleaned);
        }
    }
};

// Global nonce manager instance
extern NonceManager nonceManager;