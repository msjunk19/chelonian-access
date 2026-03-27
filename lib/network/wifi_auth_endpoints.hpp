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
// Body: { "device_id": "uuid", "token": "32-char-hex", "command": 1 }
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

    if (!deviceId || !token || command == 0) {
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

    // Dispatch command
    PhoneCommand cmd = static_cast<PhoneCommand>(command);
    onCommand(cmd);

    const char* statusStr = "unknown";
    switch (cmd) {
        case PhoneCommand::UNLOCK: statusStr = "unlocked"; break;
        case PhoneCommand::LOCK:   statusStr = "locked";   break;
        case PhoneCommand::STATUS: statusStr = "ok";       break;
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

// -------------------------
// Register endpoints

inline void setupAuthEndpoints(std::function<void(PhoneCommand)> onCommand) {
    server.on("/pair", HTTP_POST, handlePair);

    server.on("/cmd", HTTP_POST, [onCommand]() {
        handleCommand(onCommand);
    });

    server.on("/unpair", HTTP_POST, handleUnpair);

    ESP_LOGI(WIFIAUTHTAG, "Auth endpoints registered");
}