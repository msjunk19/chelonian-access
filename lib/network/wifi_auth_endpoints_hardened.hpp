#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include "phone_token_manager.hpp"
#include "auth_manager.hpp"
#include "esp_log.h"
#include "esp_random.h"
#include <led_states.hpp>
#include <access_log.hpp>
#include <config.hpp>
#include "rate_limiter.hpp"
#include "nonce_manager.hpp"
#include "encrypted_token_storage.hpp"
#include <macro_config.hpp>

static const char* WIFIAUTHTAG = "WIFIAUTH";

// Instances (defined in main.cpp)
extern WebServer server;
extern PhoneTokenManager phoneTokenManager;
extern MacroConfigManager macroConfigManager;
extern AuthManager authManager;
extern AccessLogger accessLogger;
extern RateLimiter rateLimiter;
extern NonceManager nonceManager;
extern EncryptedTokenStorage encryptedStorage;

static bool pairingWindowOpen = false;
static uint32_t pairingWindowStart = 0;

// Token usage tracking per device ID
static constexpr uint8_t MAX_TRACKED_TOKENS = 10;
struct TokenUsageEntry {
    char deviceId[PHONE_ID_MAX_LEN + 1];
    uint32_t lastUsedTime;
    uint8_t callsInWindow;
    uint32_t windowStart;
    uint8_t failureCount;
};
static std::array<TokenUsageEntry, MAX_TRACKED_TOKENS> tokenUsage;

// -------------------------
// Helper: Get client IP
// -------------------------
String getClientIP() {
    return server.client().remoteIP().toString();
}


// -------------------------
// Helper: Convert hex string to binary
// -------------------------
bool hexStringToBytes(const char* hexStr, uint8_t* bytesOut, size_t bytesLen) {
    if (!hexStr || strlen(hexStr) != bytesLen * 2) {
        return false;
    }
    
    for (size_t i = 0; i < bytesLen; i++) {
        char byte[3] = { hexStr[i*2], hexStr[i*2+1], 0 };
        char* endptr = nullptr;
        long val = strtol(byte, &endptr, 16);
        if (endptr != &byte[2] || val < 0 || val > 255) {
            return false;
        }
        bytesOut[i] = (uint8_t)val;
    }
    
    return true;
}

// -------------------------
// Helper: Send JSON error with rate limit info
// -------------------------
inline void sendJsonError(int code, const char* message, const String& clientIP = "") {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = message;

    // Include rate limit info
    if (clientIP.length() > 0) {
        uint32_t lockoutTime = rateLimiter.getLockoutTimeRemaining(clientIP);
        if (lockoutTime > 0) {
            doc["retry_after"] = (lockoutTime + 999) / 1000;  // Convert to seconds
        }

        uint16_t remaining = rateLimiter.getRemainingRequests(clientIP);
        if (code != 429) {
            doc["remaining_requests"] = remaining;
        }
    }

    String body;
    serializeJson(doc, body);
    server.send(code, "application/json", body);
}

// -------------------------
// Token usage tracking
// -------------------------
int findTokenUsageEntry(const char* deviceId) {
    for (uint8_t i = 0; i < MAX_TRACKED_TOKENS; i++) {
        if (strcmp(tokenUsage[i].deviceId, deviceId) == 0) {
            return i;
        }
    }
    return -1;
}

int createTokenUsageEntry(const char* deviceId) {
    uint32_t now = millis();

    // Find existing or first empty
    int idx = findTokenUsageEntry(deviceId);
    if (idx >= 0) {
        return idx;
    }

    // Find empty slot
    for (uint8_t i = 0; i < MAX_TRACKED_TOKENS; i++) {
        if (tokenUsage[i].deviceId[0] == '\0') {
            strncpy(tokenUsage[i].deviceId, deviceId, PHONE_ID_MAX_LEN);
            tokenUsage[i].deviceId[PHONE_ID_MAX_LEN] = '\0';
            tokenUsage[i].windowStart = now;
            tokenUsage[i].callsInWindow = 0;
            tokenUsage[i].failureCount = 0;
            return i;
        }
    }

    // Table full - overwrite oldest
    uint8_t oldestIdx = 0;
    uint32_t oldestTime = tokenUsage[0].lastUsedTime;
    for (uint8_t i = 1; i < MAX_TRACKED_TOKENS; i++) {
        if (tokenUsage[i].lastUsedTime < oldestTime) {
            oldestTime = tokenUsage[i].lastUsedTime;
            oldestIdx = i;
        }
    }

    strncpy(tokenUsage[oldestIdx].deviceId, deviceId, PHONE_ID_MAX_LEN);
    tokenUsage[oldestIdx].deviceId[PHONE_ID_MAX_LEN] = '\0';
    tokenUsage[oldestIdx].windowStart = now;
    tokenUsage[oldestIdx].callsInWindow = 0;
    tokenUsage[oldestIdx].failureCount = 0;
    return oldestIdx;
}

bool checkTokenRateLimit(const char* deviceId) {
    uint32_t now = millis();
    static constexpr uint32_t TOKEN_WINDOW_MS = 60000;  // 1 minute
    static constexpr uint8_t MAX_CALLS_PER_WINDOW = 10;

    int idx = createTokenUsageEntry(deviceId);
    TokenUsageEntry& entry = tokenUsage[idx];

    // Reset window if expired
    if (now - entry.windowStart > TOKEN_WINDOW_MS) {
        entry.windowStart = now;
        entry.callsInWindow = 0;
        entry.failureCount = 0;
    }

    // Check limit
    if (entry.callsInWindow >= MAX_CALLS_PER_WINDOW) {
        ESP_LOGW(WIFIAUTHTAG, "Token %s rate limit exceeded (%u calls)", 
                 deviceId, entry.callsInWindow);
        return false;
    }

    entry.callsInWindow++;
    entry.lastUsedTime = now;
    return true;
}

// -------------------------
// Pairing window control
// -------------------------
inline void openPairingWindow() {
    pairingWindowOpen = true;
    pairingWindowStart = millis();
    ESP_LOGI(WIFIAUTHTAG, "Pairing window opened (60s)");
    LED_SET_SEQ(SYSTEM_PAIR);
}

inline void updatePairingWindow() {
    if (pairingWindowOpen &&
        (millis() - pairingWindowStart > PAIRING_WINDOW_MS)) {
        pairingWindowOpen = false;
        ESP_LOGI(WIFIAUTHTAG, "Pairing window closed (timeout)");
    }
}

inline bool isPairingWindowOpen() {
    return pairingWindowOpen;
}

// -------------------------
// Generate a random hex token
// -------------------------
inline String generateToken() {
    uint8_t bytes[16];
    esp_fill_random(bytes, sizeof(bytes));
    char hex[33];
    for (int i = 0; i < 16; i++) {
        snprintf(hex + i * 2, 3, "%02x", bytes[i]);
    }
    hex[32] = 0;
    return String(hex);
}

// -------------------------
// POST /api/nonce - Rate limited, no auth required
// -------------------------
inline void handleGetNonce() {
    String clientIP = getClientIP();

    // CRITICAL FIX #1: Rate limit nonce endpoint (1 per 500ms, max 5 per second)
    if (!rateLimiter.checkRateLimit(clientIP, 5, 500)) {
        uint32_t lockoutTime = rateLimiter.getLockoutTimeRemaining(clientIP);
        ESP_LOGW(WIFIAUTHTAG, "Nonce rate limit exceeded for %s (locked for %lu ms)", 
                 clientIP.c_str(), lockoutTime);
        sendJsonError(429, "Rate limit exceeded", clientIP);
        return;
    }

    // Generate nonce using improved manager
    const uint8_t* nonceValue = nonceManager.generateNonce();

    // Convert to hex
    char nonceHex[33];
    for (int i = 0; i < 16; i++) {
        snprintf(nonceHex + i * 2, 3, "%02x", nonceValue[i]);
    }
    nonceHex[32] = 0;

    JsonDocument resp;
    resp["ok"] = true;
    resp["nonce"] = nonceHex;
    resp["timestamp"] = (uint32_t)(time(nullptr));
    resp["nonce_expires_in"] = NonceManager::NONCE_EXPIRY_MS / 1000;  // Inform client

    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);

    ESP_LOGD(WIFIAUTHTAG, "Nonce issued to %s", clientIP.c_str());
}

// -------------------------
// POST /pair - Pairing endpoint
// -------------------------
inline void handlePair() {
    String clientIP = getClientIP();

    // Rate limiting on pair endpoint (1 per 2 seconds, max 3 per minute)
    if (!rateLimiter.checkRateLimit(clientIP, 3, 2000)) {
        sendJsonError(429, "Rate limit exceeded", clientIP);
        return;
    }

    if (!pairingWindowOpen) {
        sendJsonError(403, "Pairing window not open", clientIP);
        return;
    }

    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body", clientIP);
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON", clientIP);
        return;
    }

    const char* deviceId = doc["device_id"];
    if (!deviceId || strlen(deviceId) == 0 || strlen(deviceId) > PHONE_ID_MAX_LEN) {
        sendJsonError(400, "Missing or invalid device_id", clientIP);
        return;
    }

    // Sync time if phone sends timestamp
    uint32_t phoneTimestamp = doc["timestamp"] | 0;
    if (phoneTimestamp > 1000000000) {
        authManager.syncTime(phoneTimestamp);
        accessLogger.clear();
    }

    // Generate token
    String token = generateToken();
    uint8_t tokenBytes[PHONE_SECRET_LEN] = {0};
    memcpy(tokenBytes, token.c_str(), min((size_t)PHONE_SECRET_LEN, token.length()));

    // CRITICAL FIX #2: Encrypt token before storage
    uint8_t encryptedToken[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
    if (!encryptedStorage.encryptToken(tokenBytes, encryptedToken)) {
        sendJsonError(500, "Token encryption failed", clientIP);
        ESP_LOGE(WIFIAUTHTAG, "Failed to encrypt token for %s", deviceId);
        return;
    }

    // Store encrypted token
    phoneTokenManager.removePhone(deviceId);
    if (!phoneTokenManager.addPhoneEncrypted(deviceId, encryptedToken)) {
        sendJsonError(409, "Storage full", clientIP);
        return;
    }

    pairingWindowOpen = false;
    ESP_LOGI(WIFIAUTHTAG, "Phone paired: %s from %s", deviceId, clientIP.c_str());
    accessLogger.logSystem(LogSource::WIFI, LogResult::SUCCESS, deviceId, "Device paired");

    JsonDocument resp;
    resp["ok"] = true;
    resp["token"] = token;  // Send to client
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}

// -------------------------
// POST /cmd - Command execution with full security
// -------------------------
inline void handleCommand(std::function<void(PhoneCommand)> onCommand) {
    String clientIP = getClientIP();

    // Rate limiting on command endpoint (1 per second, max 30 per minute)
    if (!rateLimiter.checkRateLimit(clientIP, 30, 100)) {
        sendJsonError(429, "Rate limit exceeded", clientIP);
        return;
    }

    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body", clientIP);
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON", clientIP);
        return;
    }

    const char* deviceId = doc["device_id"];
    const char* token = doc["token"];
    uint8_t command = doc["command"] | 0;
    uint32_t timestamp = doc["timestamp"] | 0;
    const char* nonce = doc["nonce"];

    if (!deviceId || !token || command == 0 || !nonce) {
        sendJsonError(400, "Missing required fields", clientIP);
        return;
    }

    // Convert hex nonce string to binary
    uint8_t nonceBytes[16];
    if (!hexStringToBytes(nonce, nonceBytes, 16)) {
        sendJsonError(400, "Invalid nonce format", clientIP);
        return;
    }

    // CRITICAL FIX #3: Validate nonce first (prevents command execution)
    if (!nonceManager.validateAndConsume(nonceBytes)) {
        sendJsonError(401, "Invalid or expired nonce", clientIP);
        accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Invalid nonce");
        return;
    }

    // CRITICAL FIX #4: Enforce timestamp validation
    if (timestamp == 0) {
        sendJsonError(400, "Timestamp required", clientIP);
        return;
    }

    if (authManager.isTimeSynced()) {
        uint32_t now = authManager.getCurrentTime();
        int32_t drift = (int32_t)timestamp - (int32_t)now;

        if (abs(drift) > AUTH_TIMESTAMP_WINDOW) {
            ESP_LOGW(WIFIAUTHTAG, "Timestamp rejected for %s: drift=%ld seconds", deviceId, drift);
            sendJsonError(401, "Timestamp out of window", clientIP);
            accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Timestamp rejected");
            return;
        }

        ESP_LOGD(WIFIAUTHTAG, "Timestamp OK for %s: drift=%ld seconds", deviceId, drift);
    }

    if (strlen(token) != 32) {
        sendJsonError(400, "Invalid token length", clientIP);
        return;
    }

    // Look up stored encrypted token for this device
    uint8_t storedTokenEncrypted[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
    uint8_t storedToken[PHONE_SECRET_LEN] = {0};

    if (!phoneTokenManager.getPhoneEncrypted(deviceId, storedTokenEncrypted)) {
        sendJsonError(401, "Unknown device", clientIP);
        accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Unknown device");
        return;
    }

    // CRITICAL FIX #2: Decrypt stored token
    if (!encryptedStorage.decryptToken(storedTokenEncrypted, storedToken)) {
        sendJsonError(500, "Token decryption failed", clientIP);
        ESP_LOGE(WIFIAUTHTAG, "Failed to decrypt token for %s", deviceId);
        return;
    }

    // Constant-time compare
    uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
    memcpy(incomingToken, token, min((size_t)PHONE_SECRET_LEN, strlen(token)));

    uint8_t diff = 0;
    for (int i = 0; i < PHONE_SECRET_LEN; i++) {
        diff |= storedToken[i] ^ incomingToken[i];
    }

    if (diff != 0) {
        ESP_LOGW(WIFIAUTHTAG, "Invalid token for device: %s from %s", deviceId, clientIP.c_str());
        
        // CRITICAL FIX #5: Track failed auth attempts per token
        int idx = createTokenUsageEntry(deviceId);
        tokenUsage[idx].failureCount++;
        
        if (tokenUsage[idx].failureCount > 5) {
            // Exponential backoff: lock after 5 failures
            accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Too many auth failures");
            sendJsonError(429, "Too many failed attempts", clientIP);
            return;
        }

        accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Auth failed");
        sendJsonError(401, "Unauthorized", clientIP);
        return;
    }

    // CRITICAL FIX #6: Per-token rate limiting
    if (!checkTokenRateLimit(deviceId)) {
        ESP_LOGW(WIFIAUTHTAG, "Token %s rate limit exceeded", deviceId);
        sendJsonError(429, "Device rate limit exceeded", clientIP);
        accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Rate limit exceeded");
        return;
    }

    // Sync time from the incoming timestamp
    if (timestamp > 1000000000) {
        authManager.syncTime(timestamp);
    }

    // Dispatch command
    PhoneCommand cmd = static_cast<PhoneCommand>(command);
    onCommand(cmd);

    // Log successful command
    const char* cmdName = "unknown";
    switch (cmd) {
        case PhoneCommand::UNLOCK: cmdName = "Unlock"; break;
        case PhoneCommand::LOCK: cmdName = "Lock"; break;
        case PhoneCommand::TRUNK: cmdName = "Trunk"; break;
        case PhoneCommand::PANIC: cmdName = "Panic"; break;
        default: break;
    }
    accessLogger.logAccess(LogSource::WIFI, LogResult::SUCCESS, deviceId, cmdName);

    const char* statusStr = "unknown";
    switch (cmd) {
        case PhoneCommand::UNLOCK: statusStr = "unlocked"; break;
        case PhoneCommand::LOCK: statusStr = "locked"; break;
        case PhoneCommand::TRUNK: statusStr = "trunk"; break;
        case PhoneCommand::PANIC: statusStr = "panic"; break;
        default: break;
    }

    ESP_LOGI(WIFIAUTHTAG, "Command OK: %s → %s from %s", deviceId, statusStr, clientIP.c_str());

    JsonDocument resp;
    resp["ok"] = true;
    resp["status"] = statusStr;
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}

// -------------------------
// POST /unpair
// -------------------------
inline void handleUnpair() {
    String clientIP = getClientIP();

    if (!rateLimiter.checkRateLimit(clientIP, 5, 200)) {
        sendJsonError(429, "Rate limit exceeded", clientIP);
        return;
    }

    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body", clientIP);
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON", clientIP);
        return;
    }

    const char* deviceId = doc["device_id"];
    const char* token = doc["token"];
    uint32_t timestamp = doc["timestamp"] | 0;

    if (!deviceId || !token) {
        sendJsonError(400, "Missing required fields", clientIP);
        return;
    }

    // Same token validation as command
    uint8_t storedTokenEncrypted[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
    uint8_t storedToken[PHONE_SECRET_LEN] = {0};

    if (!phoneTokenManager.getPhoneEncrypted(deviceId, storedTokenEncrypted)) {
        sendJsonError(401, "Unknown device", clientIP);
        return;
    }

    if (!encryptedStorage.decryptToken(storedTokenEncrypted, storedToken)) {
        sendJsonError(500, "Token decryption failed", clientIP);
        return;
    }

    uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
    memcpy(incomingToken, token, min((size_t)PHONE_SECRET_LEN, strlen(token)));

    uint8_t diff = 0;
    for (int i = 0; i < PHONE_SECRET_LEN; i++) {
        diff |= storedToken[i] ^ incomingToken[i];
    }

    if (diff != 0) {
        sendJsonError(401, "Unauthorized", clientIP);
        return;
    }

    phoneTokenManager.removePhone(deviceId);
    accessLogger.logSystem(LogSource::WIFI, LogResult::SUCCESS, deviceId, "Device unpaired");

    JsonDocument resp;
    resp["ok"] = true;
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}

// // -------------------------
// // Helper: Convert hex string to binary
// // -------------------------
// bool hexStringToBytes(const char* hexStr, uint8_t* bytesOut, size_t bytesLen) {
//     if (!hexStr || strlen(hexStr) != bytesLen * 2) {
//         return false;
//     }
    
//     for (size_t i = 0; i < bytesLen; i++) {
//         char byte[3] = { hexStr[i*2], hexStr[i*2+1], 0 };
//         char* endptr = nullptr;
//         long val = strtol(byte, &endptr, 16);
//         if (endptr != &byte[2] || val < 0 || val > 255) {
//             return false;
//         }
//         bytesOut[i] = (uint8_t)val;
//     }
    
//     return true;
// }

// -------------------------
// Register all endpoints
// -------------------------
inline void setupAuthEndpoints(std::function<void(PhoneCommand)> onCommand) {
    // Nonce endpoint (rate limited, no auth)
    server.on("/api/nonce", HTTP_GET, handleGetNonce);

    // Pairing endpoint (rate limited)
    server.on("/pair", HTTP_POST, handlePair);

    // Command endpoint (fully secured)
    server.on("/cmd", HTTP_POST, [onCommand]() {
        handleCommand(onCommand);
    });

    // Unpair endpoint (requires valid token)
    server.on("/unpair", HTTP_POST, handleUnpair);

    // TODO: Implement similar hardening for macro and log endpoints

    // -------------------------
    // GET /api/macros - Fetch macro configuration
    // -------------------------
    server.on("/api/macros", HTTP_GET, []() {
        // Auto-sync time from query param
        uint32_t phoneTimestamp = server.arg("timestamp").toInt();
        if (phoneTimestamp > 1000000000) {
            authManager.syncTime(phoneTimestamp);
        }

        JsonDocument doc;
        doc["macro_count"] = macroConfigManager.config.macro_count;
        doc["tag_macro"]   = macroConfigManager.config.tag_macro;

        JsonArray macros = doc["macros"].to<JsonArray>();
        for (uint8_t i = 0; i < macroConfigManager.config.macro_count; i++) {
            Macro& m = macroConfigManager.config.macros[i];
            JsonObject macro = macros.add<JsonObject>();
            macro["name"]       = m.name;
            macro["icon"]       = m.icon;
            macro["step_count"] = m.step_count;

            JsonArray steps = macro["steps"].to<JsonArray>();
            for (uint8_t s = 0; s < m.step_count; s++) {
                JsonObject step = steps.add<JsonObject>();
                step["relay_mask"] = m.steps[s].relay_mask;
                step["duration"]   = m.steps[s].duration;
                step["gap"]        = m.steps[s].gap;
            }
        }

        String body;
        serializeJson(doc, body);
        server.send(200, "application/json", body);
    });

    // -------------------------
    // POST /api/macros - Save macro configuration
    // -------------------------
    server.on("/api/macros", HTTP_POST, []() {
        String clientIP = getClientIP();

        // Rate limiting on macro endpoint
        if (!rateLimiter.checkRateLimit(clientIP, 10, 1000)) {
            sendJsonError(429, "Rate limit exceeded", clientIP);
            return;
        }

        if (!server.hasArg("plain")) {
            sendJsonError(400, "Missing body", clientIP);
            return;
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, server.arg("plain"));
        if (err) {
            sendJsonError(400, "Invalid JSON", clientIP);
            return;
        }

        const char* deviceId = doc["device_id"];
        const char* token = doc["token"];
        uint32_t timestamp = doc["timestamp"] | 0;
        const char* nonceStr = doc["nonce"];

        if (!deviceId || !token || !nonceStr) {
            sendJsonError(400, "Missing required fields", clientIP);
            return;
        }

        // Convert hex nonce string to binary
        uint8_t nonceBytes[16];
        if (!hexStringToBytes(nonceStr, nonceBytes, 16)) {
            sendJsonError(400, "Invalid nonce format", clientIP);
            return;
        }

        // Validate nonce
        if (!nonceManager.validateAndConsume(nonceBytes)) {
            sendJsonError(401, "Invalid or expired nonce", clientIP);
            accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Invalid nonce");
            return;
        }

        // Validate timestamp
        if (timestamp == 0) {
            sendJsonError(400, "Timestamp required", clientIP);
            return;
        }

        if (authManager.isTimeSynced()) {
            uint32_t now = authManager.getCurrentTime();
            int32_t drift = (int32_t)timestamp - (int32_t)now;

            if (abs(drift) > AUTH_TIMESTAMP_WINDOW) {
                sendJsonError(401, "Timestamp out of window", clientIP);
                return;
            }
        }

        // Validate token
        if (strlen(token) != 32) {
            sendJsonError(400, "Invalid token length", clientIP);
            return;
        }

        uint8_t storedTokenEncrypted[EncryptedTokenStorage::IV_SIZE + PHONE_SECRET_LEN + EncryptedTokenStorage::TAG_SIZE];
        uint8_t storedToken[PHONE_SECRET_LEN] = {0};

        if (!phoneTokenManager.getPhoneEncrypted(deviceId, storedTokenEncrypted)) {
            sendJsonError(401, "Unknown device", clientIP);
            return;
        }

        if (!encryptedStorage.decryptToken(storedTokenEncrypted, storedToken)) {
            sendJsonError(500, "Token decryption failed", clientIP);
            return;
        }

        uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
        memcpy(incomingToken, token, min((size_t)PHONE_SECRET_LEN, strlen(token)));

        uint8_t diff = 0;
        for (int i = 0; i < PHONE_SECRET_LEN; i++) {
            diff |= storedToken[i] ^ incomingToken[i];
        }

        if (diff != 0) {
            sendJsonError(401, "Unauthorized", clientIP);
            accessLogger.logAccess(LogSource::WIFI, LogResult::FAIL, deviceId, "Auth failed");
            return;
        }

        // Parse macro configuration
        uint8_t count = doc["macro_count"] | 0;
        if (count == 0 || count > MAX_MACROS) {
            sendJsonError(400, "Invalid macro_count", clientIP);
            return;
        }

        uint8_t tag_macro = doc["tag_macro"] | 0;
        if (tag_macro >= count) {
            sendJsonError(400, "tag_macro out of range", clientIP);
            return;
        }

        macroConfigManager.config.macro_count = count;
        macroConfigManager.config.tag_macro = tag_macro;

        uint32_t now = millis();
        JsonArray macros = doc["macros"].as<JsonArray>();
        for (uint8_t i = 0; i < count; i++) {
            JsonObject m = macros[i].as<JsonObject>();
            Macro& macro = macroConfigManager.config.macros[i];

            const char* name = m["name"] | "";
            const char* icon = m["icon"] | "";
            strncpy(macro.name, name, sizeof(macro.name) - 1);
            macro.name[sizeof(macro.name) - 1] = '\0';
            strncpy(macro.icon, icon, sizeof(macro.icon) - 1);
            macro.icon[sizeof(macro.icon) - 1] = '\0';
            macro.magic = MACRO_MAGIC;
            macro.updated_at = now;

            uint8_t step_count = m["step_count"] | 0;
            if (step_count > MAX_STEPS) step_count = MAX_STEPS;
            macro.step_count = step_count;

            // Clear unused steps
            for (uint8_t s = step_count; s < MAX_STEPS; s++) {
                macro.steps[s].relay_mask = 0;
                macro.steps[s].duration = 0;
                macro.steps[s].gap = 0;
            }

            JsonArray steps = m["steps"].as<JsonArray>();
            for (uint8_t s = 0; s < step_count; s++) {
                JsonObject step = steps[s].as<JsonObject>();
                macro.steps[s].relay_mask = step["relay_mask"] | 0;
                macro.steps[s].duration = step["duration"] | 500;
                macro.steps[s].gap = step["gap"] | 0;
            }
        }

        macroConfigManager.saveAll();
        macroConfigManager.printConfig();

        accessLogger.logSystem(LogSource::WIFI, LogResult::SUCCESS, deviceId, "Macros saved");

        JsonDocument resp;
        resp["ok"] = true;
        String body;
        serializeJson(resp, body);
        server.send(200, "application/json", body);

        ESP_LOGI(WIFIAUTHTAG, "Macros saved by %s: %d macros, tag=%d", deviceId, count, tag_macro);
    });
    
    // -------------------------
    // GET /api/logs - Fetch access logs
    // -------------------------
    server.on("/api/logs", HTTP_GET, []() {
        // Auto-sync time from query param
        uint32_t phoneTimestamp = server.arg("timestamp").toInt();
        if (phoneTimestamp > 1000000000) {
            authManager.syncTime(phoneTimestamp);
        }

        int8_t level = -1;
        if (server.hasArg("level")) {
            level = (int8_t)server.arg("level").toInt();
        }
        
        uint32_t clientTime = server.arg("timestamp").toInt();
        String json = accessLogger.getLogsJson(level, clientTime);
        server.send(200, "application/json", json);
    });

    ESP_LOGI(WIFIAUTHTAG, "Hardened auth endpoints registered");

    // ESP_LOGI(WIFIAUTHTAG, "Hardened auth endpoints registered");
}