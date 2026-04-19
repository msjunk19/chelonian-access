#pragma once
#include <Arduino.h>
#include <array>
#include <cstring>
#include "esp_log.h"

static const char* RATELIMIT_TAG = "RATELIMIT";

/**
 * IP-based rate limiter for protecting endpoints
 * Tracks request frequency per IP address
 */
class RateLimiter {
public:
    struct IPEntry {
        char ip[16];
        uint32_t lastRequestTime;
        uint16_t requestCount;
        uint32_t windowStart;
        bool isLocked;
        uint32_t lockoutUntil;
    };

    static constexpr uint8_t MAX_IPS = 10;
    static constexpr uint32_t WINDOW_SIZE_MS = 60000;  // 1 minute window
    static constexpr uint32_t MIN_INTERVAL_MS = 100;   // Min 100ms between requests
    static constexpr uint8_t MAX_REQUESTS_PER_WINDOW = 10;
    static constexpr uint8_t LOCKOUT_MULTIPLIER = 2;   // Exponential backoff

    RateLimiter() {
        // Initialize all entries to zero
        for (auto& entry : entries) {
            memset(&entry, 0, sizeof(IPEntry));
        }
    }

    /**
     * Check if request from IP is allowed
     * Returns: true if allowed, false if rate limited
     */
    bool checkRateLimit(const String& clientIP, uint16_t maxPerWindow = MAX_REQUESTS_PER_WINDOW, 
                        uint32_t minInterval = MIN_INTERVAL_MS) {
        uint32_t now = millis();
        int entryIdx = findOrCreateEntry(clientIP);

        if (entryIdx < 0) {
            ESP_LOGW(RATELIMIT_TAG, "Rate limiter full, rejecting %s", clientIP.c_str());
            return false;
        }

        IPEntry& entry = entries[entryIdx];

        // Check if currently locked out
        if (entry.isLocked && now < entry.lockoutUntil) {
            ESP_LOGW(RATELIMIT_TAG, "IP %s locked out until %lu (now: %lu)", 
                     clientIP.c_str(), entry.lockoutUntil, now);
            return false;
        }

        // Unlock if lockout period has expired
        if (entry.isLocked && now >= entry.lockoutUntil) {
            entry.isLocked = false;
            entry.requestCount = 0;
            entry.windowStart = now;
            ESP_LOGI(RATELIMIT_TAG, "IP %s unlocked", clientIP.c_str());
        }

        // Check minimum interval
        if (now - entry.lastRequestTime < minInterval) {
            ESP_LOGW(RATELIMIT_TAG, "IP %s violated minimum interval (%lu ms < %lu ms)", 
                     clientIP.c_str(), now - entry.lastRequestTime, minInterval);
            triggerLockout(entryIdx);
            return false;
        }

        // Reset window if expired
        if (now - entry.windowStart > WINDOW_SIZE_MS) {
            entry.windowStart = now;
            entry.requestCount = 0;
        }

        // Check request count
        if (entry.requestCount >= maxPerWindow) {
            ESP_LOGW(RATELIMIT_TAG, "IP %s exceeded rate limit (%u >= %u)", 
                     clientIP.c_str(), entry.requestCount, maxPerWindow);
            triggerLockout(entryIdx);
            return false;
        }

        // Allowed - update entry
        entry.lastRequestTime = now;
        entry.requestCount++;
        ESP_LOGD(RATELIMIT_TAG, "IP %s allowed (count: %u/%u)", 
                 clientIP.c_str(), entry.requestCount, maxPerWindow);
        
        return true;
    }

    /**
     * Get remaining requests for IP in current window
     */
    uint16_t getRemainingRequests(const String& clientIP, uint16_t maxPerWindow = MAX_REQUESTS_PER_WINDOW) {
        int entryIdx = findEntry(clientIP);
        if (entryIdx < 0) return maxPerWindow;

        uint32_t now = millis();
        IPEntry& entry = entries[entryIdx];

        if (now - entry.windowStart > WINDOW_SIZE_MS) {
            return maxPerWindow;
        }

        return (entry.requestCount >= maxPerWindow) ? 0 : (maxPerWindow - entry.requestCount);
    }

    /**
     * Reset rate limit for specific IP (admin function)
     */
    void resetIP(const String& clientIP) {
        int entryIdx = findEntry(clientIP);
        if (entryIdx >= 0) {
            memset(&entries[entryIdx], 0, sizeof(IPEntry));
            ESP_LOGI(RATELIMIT_TAG, "Rate limit reset for %s", clientIP.c_str());
        }
    }

    /**
     * Get lockout status for IP
     */
    bool isLockedOut(const String& clientIP) const {
        int entryIdx = findEntry(clientIP);
        if (entryIdx < 0) return false;
        
        uint32_t now = millis();
        return entries[entryIdx].isLocked && now < entries[entryIdx].lockoutUntil;
    }

    /**
     * Get time until lockout expires (0 if not locked)
     */
    uint32_t getLockoutTimeRemaining(const String& clientIP) const {
        int entryIdx = findEntry(clientIP);
        if (entryIdx < 0) return 0;

        if (!entries[entryIdx].isLocked) return 0;

        uint32_t now = millis();
        if (now >= entries[entryIdx].lockoutUntil) return 0;

        return entries[entryIdx].lockoutUntil - now;
    }

private:
    std::array<IPEntry, MAX_IPS> entries;

    /**
     * Find existing entry or create new one
     * Returns: index of entry, or -1 if table full
     */
    int findOrCreateEntry(const String& clientIP) {
        // Try to find existing
        int existingIdx = findEntry(clientIP);
        if (existingIdx >= 0) return existingIdx;

        // Find empty slot
        for (uint8_t i = 0; i < MAX_IPS; i++) {
            if (entries[i].ip[0] == '\0') {
                strncpy(entries[i].ip, clientIP.c_str(), sizeof(entries[i].ip) - 1);
                entries[i].ip[sizeof(entries[i].ip) - 1] = '\0';
                entries[i].windowStart = millis();
                return i;
            }
        }

        return -1;
    }

    /**
     * Find existing entry by IP
     * Returns: index or -1 if not found
     */
    int findEntry(const String& clientIP) const {
        for (uint8_t i = 0; i < MAX_IPS; i++) {
            if (strcmp(entries[i].ip, clientIP.c_str()) == 0) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Trigger exponential backoff lockout for IP
     */
    void triggerLockout(uint8_t entryIdx) {
        uint32_t lockoutTime = 30000;  // Start at 30 seconds

        // Exponential backoff: 30s, 60s, 120s, 240s, etc.
        if (entries[entryIdx].isLocked) {
            lockoutTime = entries[entryIdx].lockoutUntil - millis();
            lockoutTime *= LOCKOUT_MULTIPLIER;
            if (lockoutTime > 3600000) lockoutTime = 3600000;  // Cap at 1 hour
        }

        entries[entryIdx].isLocked = true;
        entries[entryIdx].lockoutUntil = millis() + lockoutTime;
        entries[entryIdx].requestCount = 0;

        ESP_LOGW(RATELIMIT_TAG, "IP %s locked out for %lu ms", 
                 entries[entryIdx].ip, lockoutTime);
    }
};

// Global rate limiter instance
extern RateLimiter rateLimiter;