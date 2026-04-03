#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include "phone_token_manager.hpp"
#include <auth_manager.hpp>
#include "esp_log.h"
#include "esp_random.h"
#include <led_states.hpp>

static const char* WIFIAUTHTAG = "WIFIAUTH";

static bool pairingWindowOpen = false;
static uint32_t pairingWindowStart = 0;
static constexpr uint32_t PAIRING_WINDOW_MS = 60000;

extern WebServer server;
extern PhoneTokenManager phoneTokenManager;
extern MacroConfigManager macroConfigManager;
extern AuthManager authManager;

// Max allowed clock drift between phone and device (seconds)
static constexpr uint32_t CMD_TIMESTAMP_WINDOW = 30;


// -------------------------
// Pairing window control

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
// Helper — send JSON error

inline void sendJsonError(int code, const char* message) {
    JsonDocument doc;
    doc["ok"]    = false;
    doc["error"] = message;
    String body;
    serializeJson(doc, body);
    server.send(code, "application/json", body);
}

// -------------------------
// Generate a random hex token

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
// POST /pair
// Body: { "device_id": "uuid-string" }
// Response: { "ok": true, "token": "32-char-hex" }

inline void handlePair() {
    if (!pairingWindowOpen) {
        sendJsonError(403, "Pairing window not open");
        return;
    }

    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON");
        return;
    }

    const char* deviceId = doc["device_id"];
    if (!deviceId || strlen(deviceId) == 0 ||
        strlen(deviceId) > PHONE_ID_MAX_LEN) {
        sendJsonError(400, "Missing or invalid device_id");
        return;
    }

    // Sync time if phone sends timestamp
    uint32_t phoneTimestamp = doc["timestamp"] | 0;
    if (phoneTimestamp > 1000000000) { // sanity check: must be reasonable Unix time
        authManager.syncTime(phoneTimestamp);
    }

    // Generate a random session token
    String token = generateToken();

    // Store token as the "secret" for this device
    uint8_t tokenBytes[PHONE_SECRET_LEN] = {0};
    memcpy(tokenBytes, token.c_str(), min((size_t)PHONE_SECRET_LEN, token.length()));

    // Try to add — if already paired, update the token
    phoneTokenManager.removePhone(deviceId); // remove old entry if exists
    if (!phoneTokenManager.addPhone(deviceId, tokenBytes)) {
        sendJsonError(409, "Storage full");
        return;
    }

    pairingWindowOpen = false;
    ESP_LOGI(WIFIAUTHTAG, "Phone paired via WiFi: %s", deviceId);

    JsonDocument resp;
    resp["ok"]    = true;
    resp["token"] = token;
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}

// -------------------------
// POST /cmd
// Body: { "device_id": "uuid", "token": "32-char-hex", "command": 1, "timestamp": 1234567890 }
// Response: { "ok": true, "status": "unlocked" }

inline void handleCommand(std::function<void(PhoneCommand)> onCommand) {
    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON");
        return;
    }

    const char* deviceId = doc["device_id"];
    const char* token    = doc["token"];
    uint8_t     command  = doc["command"] | 0;
    uint32_t    timestamp = doc["timestamp"] | 0;

    if (!deviceId || !token || command == 0 || timestamp == 0) {
        sendJsonError(400, "Missing required fields");
        return;
    }

    if (strlen(token) != 32) {
        sendJsonError(400, "Invalid token length");
        return;
    }

    // Look up stored token for this device
    uint8_t storedToken[PHONE_SECRET_LEN] = {0};
    if (!phoneTokenManager.getSecret(deviceId, storedToken)) {
        sendJsonError(401, "Unknown device");
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
        ESP_LOGW(WIFIAUTHTAG, "Invalid token for device: %s", deviceId);
        sendJsonError(401, "Unauthorized");
        return;
    }

    // Validate timestamp to prevent replay attacks (skip if time not yet synced)
    if (authManager.isTimeSynced()) {
        uint32_t now = authManager.getCurrentTime();
        int32_t drift = (int32_t)timestamp - (int32_t)now;
        if (drift > (int32_t)CMD_TIMESTAMP_WINDOW || 
            drift < -(int32_t)CMD_TIMESTAMP_WINDOW) {
            ESP_LOGW(WIFIAUTHTAG, "Timestamp rejected — drift: %ld seconds (cmd from %s)", drift, deviceId);
            sendJsonError(401, "Timestamp expired");
            return;
        }
    }

    // Dispatch command
    PhoneCommand cmd = static_cast<PhoneCommand>(command);
    onCommand(cmd);

    const char* statusStr = "unknown";
    switch (cmd) {
        case PhoneCommand::UNLOCK: statusStr = "unlocked"; break;
        case PhoneCommand::LOCK:   statusStr = "locked";   break;
        case PhoneCommand::STATUS: statusStr = "ok";       break;
        case PhoneCommand::TRUNK:  statusStr = "trunk";    break;
        case PhoneCommand::PANIC:  statusStr = "panic";    break;
        default: break;
    }

    ESP_LOGI(WIFIAUTHTAG, "Command OK: %s → %s", deviceId, statusStr);

    JsonDocument resp;
    resp["ok"]     = true;
    resp["status"] = statusStr;
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}

// -------------------------
// POST /unpair
// Body: { "device_id": "uuid", "token": "32-char-hex" }

inline void handleUnpair() {
    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON");
        return;
    }

    const char* deviceId = doc["device_id"];
    const char* token    = doc["token"];

    if (!deviceId || !token) {
        sendJsonError(400, "Missing required fields");
        return;
    }

    // Verify token before allowing unpair
    uint8_t storedToken[PHONE_SECRET_LEN] = {0};
    if (!phoneTokenManager.getSecret(deviceId, storedToken)) {
        sendJsonError(401, "Unknown device");
        return;
    }

    uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
    memcpy(incomingToken, token, min((size_t)PHONE_SECRET_LEN, strlen(token)));

    uint8_t diff = 0;
    for (int i = 0; i < PHONE_SECRET_LEN; i++) {
        diff |= storedToken[i] ^ incomingToken[i];
    }

    if (diff != 0) {
        sendJsonError(401, "Unauthorized");
        return;
    }

    phoneTokenManager.removePhone(deviceId);

    JsonDocument resp;
    resp["ok"] = true;
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}

// GET /api/macros
// Returns full macro config as JSON
inline void handleGetMacros() {
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
}

// POST /api/macros
// Body: same structure as GET response
inline void handleSetMacros() {
    if (!server.hasArg("plain")) {
        sendJsonError(400, "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON");
        return;
    }

    uint8_t count = doc["macro_count"] | 0;
    if (count == 0 || count > MAX_MACROS) {
        sendJsonError(400, "Invalid macro_count");
        return;
    }

    uint8_t tag_macro = doc["tag_macro"] | 0;
    if (tag_macro >= count) {
        sendJsonError(400, "tag_macro out of range");
        return;
    }

    macroConfigManager.config.macro_count = count;
    macroConfigManager.config.tag_macro   = tag_macro;

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

        uint8_t step_count = m["step_count"] | 0;
        if (step_count > MAX_STEPS) step_count = MAX_STEPS;
        macro.step_count = step_count;

        JsonArray steps = m["steps"].as<JsonArray>();
        for (uint8_t s = 0; s < step_count; s++) {
            JsonObject step = steps[s].as<JsonObject>();
            macro.steps[s].relay_mask = step["relay_mask"] | 0;
            macro.steps[s].duration   = step["duration"]   | 500;
            macro.steps[s].gap        = step["gap"]        | 0;
        }
    }

    macroConfigManager.saveAll();
    macroConfigManager.printConfig();

    JsonDocument resp;
    resp["ok"] = true;
    String body;
    serializeJson(resp, body);
    server.send(200, "application/json", body);
}


// -------------------------
// Register endpoints

inline void setupAuthEndpoints(std::function<void(PhoneCommand)> onCommand) {
    server.on("/pair", HTTP_POST, handlePair);

    server.on("/cmd", HTTP_POST, [onCommand]() {
        handleCommand(onCommand);
    });

    server.on("/unpair", HTTP_POST, handleUnpair);

    server.on("/api/macros", HTTP_GET,  handleGetMacros);
    server.on("/api/macros", HTTP_POST, handleSetMacros);

    ESP_LOGI(WIFIAUTHTAG, "Auth endpoints registered");
}