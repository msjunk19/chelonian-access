#pragma once
#include <NimBLEDevice.h>
#include "phone_token_manager.hpp"
#include "auth_manager.hpp"
#include "macro_config.hpp"
#include "esp_log.h"
#include <config.hpp>

static const char* BLETAG = "BLE";

#define BLE_SERVICE_UUID     "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CMD_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_STATUS_UUID      "beb5483f-36e1-4688-b7f5-ea07361b26a8"
#define BLE_PAIR_UUID        "beb54840-36e1-4688-b7f5-ea07361b26a8"
#define BLE_BEACON_UUID_CHAR "beb54841-36e1-4688-b7f5-ea07361b26a8" // read only
#define BLE_MAC_UUID_CHAR    "beb54842-36e1-4688-b7f5-ea07361b26a8" // read only
#define BLE_VERIFY_UUID      "beb54843-36e1-4688-b7f5-ea07361b26a8" // write/read
#define BLE_MACRO_GET_UUID   "beb54844-36e1-4688-b7f5-ea07361b26a8" // read macro config
#define BLE_MACRO_SET_UUID   "beb54845-36e1-4688-b7f5-ea07361b26a8" // write macro config
#define BLE_CMD_UNLOCK  0x01
#define BLE_CMD_LOCK    0x02
#define BLE_CMD_STATUS  0x03

extern PhoneTokenManager phoneTokenManager;
extern AuthManager authManager;
extern MacroConfigManager macroConfigManager;

class BLEManager {
public:
    void begin(std::function<void(PhoneCommand)> onCommand) {
        _onCommand = onCommand;

        NimBLEDevice::init("Chelonian");
        NimBLEDevice::setMTU(128);

        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityPasskey(0);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

        _server = NimBLEDevice::createServer();
        _server->setCallbacks(new ServerCallbacks(this));

        NimBLEService* service = _server->createService(BLE_SERVICE_UUID);

        // Command characteristic
        _cmdChar = service->createCharacteristic(
            BLE_CMD_UUID,
            NIMBLE_PROPERTY::WRITE
        );
        _cmdChar->setCallbacks(new CommandCallbacks(this));

        // Status characteristic
        _statusChar = service->createCharacteristic(
            BLE_STATUS_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        // Pairing characteristic
        _pairChar = service->createCharacteristic(
            BLE_PAIR_UUID,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
        );
        _pairChar->setCallbacks(new PairingCallbacks(this));

        // Beacon UUID characteristic
        _beaconUUIDChar = service->createCharacteristic(
            BLE_BEACON_UUID_CHAR,
            NIMBLE_PROPERTY::READ
        );

        // MAC address characteristic
        _macChar = service->createCharacteristic(
            BLE_MAC_UUID_CHAR,
            NIMBLE_PROPERTY::READ
        );

        // Verify characteristic
        _verifyChar = service->createCharacteristic(
            BLE_VERIFY_UUID,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
        );
        _verifyChar->setCallbacks(new VerifyCallbacks(this));

        // Macro config - read
        _macroGetChar = service->createCharacteristic(
            BLE_MACRO_GET_UUID,
            NIMBLE_PROPERTY::READ
        );

        // Macro config - write
        _macroSetChar = service->createCharacteristic(
            BLE_MACRO_SET_UUID,
            NIMBLE_PROPERTY::WRITE
        );
        _macroSetChar->setCallbacks(new MacroCallbacks(this));

        _server->start();

        // Generate unique iBeacon UUID from MAC address
        char beaconUUID[37];
        _generateBeaconUUID(beaconUUID);
        ESP_LOGI(BLETAG, "iBeacon UUID: %s", beaconUUID);

        // Set beacon UUID characteristic value
        _beaconUUIDChar->setValue(beaconUUID);

        // Set MAC address characteristic value
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BT);
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        _macChar->setValue(macStr);
        ESP_LOGI(BLETAG, "BLE MAC: %s", macStr);

        _startAdvertising(beaconUUID);
        ESP_LOGI(BLETAG, "BLE started — device name: Chelonian");
    }

    void update() {}

    void notifyStatus(const char* status) {
        if (_statusChar && _server->getConnectedCount() > 0) {
            _statusChar->setValue(status);
            _statusChar->notify();
            ESP_LOGI(BLETAG, "Status notified: %s", status);
        }
    }

    String getMacroConfigJson() {
        String json = "{";
        json += "\"macro_count\":" + String(macroConfigManager.config.macro_count) + ",";
        json += "\"tag_macro\":" + String(macroConfigManager.config.tag_macro) + ",";
        json += "\"macros\":[";
        
        for (uint8_t i = 0; i < macroConfigManager.config.macro_count; i++) {
            if (i > 0) json += ",";
            Macro& m = macroConfigManager.config.macros[i];
            json += "{";
            json += "\"name\":\"" + String(m.name) + "\",";
            json += "\"step_count\":" + String(m.step_count) + ",";
            json += "\"steps\":[";
            for (uint8_t s = 0; s < m.step_count; s++) {
                if (s > 0) json += ",";
                json += "{";
                json += "\"relay_mask\":" + String(m.steps[s].relay_mask) + ",";
                json += "\"duration\":" + String(m.steps[s].duration) + ",";
                json += "\"gap\":" + String(m.steps[s].gap);
                json += "}";
            }
            json += "]";
            json += "}";
        }
        json += "]}";
        return json;
    }

    void refreshMacroChar() {
        if (_macroGetChar) {
            String json = getMacroConfigJson();
            _macroGetChar->setValue(json.c_str());
            ESP_LOGI(BLETAG, "Macro config refreshed (%d bytes)", json.length());
        }
    }

    void openPairingWindow() {
        _pairingWindowOpen  = true;
        _pairingWindowStart = millis();
        ESP_LOGI(BLETAG, "BLE pairing window opened (60s)");
    }

    bool isPairingWindowOpen() {
        return _pairingWindowOpen;
    }

    void updatePairingWindow() {
        if (_pairingWindowOpen &&
            (millis() - _pairingWindowStart > 60000)) {
            _pairingWindowOpen = false;
            ESP_LOGI(BLETAG, "BLE pairing window closed (timeout)");
        }
    }

private:
    NimBLEServer*          _server         = nullptr;
    NimBLECharacteristic*  _cmdChar        = nullptr;
    NimBLECharacteristic*  _statusChar     = nullptr;
    NimBLECharacteristic*  _pairChar       = nullptr;
    NimBLECharacteristic*  _beaconUUIDChar = nullptr;
    NimBLECharacteristic*  _macChar        = nullptr;
    NimBLECharacteristic*  _verifyChar     = nullptr;
    NimBLECharacteristic*  _macroGetChar   = nullptr;
    NimBLECharacteristic*  _macroSetChar   = nullptr;

    bool     _pairingWindowOpen  = false;
    uint32_t _pairingWindowStart = 0;

    std::function<void(PhoneCommand)> _onCommand;

    void _generateBeaconUUID(char* uuidOut) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BT);
        snprintf(uuidOut, 37,
            "43484c4e-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            mac[0], mac[1],
            mac[2], mac[3],
            mac[4], mac[5],
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
        );
    }

    void _parseUUID(const char* uuidStr, uint8_t* out) {
        char hex[33] = {0};
        int j = 0;
        for (int i = 0; uuidStr[i] && j < 32; i++) {
            if (uuidStr[i] != '-') hex[j++] = uuidStr[i];
        }
        for (int i = 0; i < 16; i++) {
            char byte[3] = { hex[i*2], hex[i*2+1], 0 };
            out[i] = (uint8_t)strtol(byte, nullptr, 16);
        }
    }

    void _startAdvertising(const char* beaconUUID) {
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

        uint8_t beaconData[25];
        beaconData[0] = 0x4C;
        beaconData[1] = 0x00;
        beaconData[2] = 0x02;
        beaconData[3] = 0x15;
        _parseUUID(beaconUUID, &beaconData[4]);
        beaconData[20] = (IBEACON_MAJOR >> 8) & 0xFF;
        beaconData[21] =  IBEACON_MAJOR       & 0xFF;
        beaconData[22] = (IBEACON_MINOR >> 8) & 0xFF;
        beaconData[23] =  IBEACON_MINOR       & 0xFF;
        beaconData[24] =  IBEACON_TX_POWER;

        char debugStr[100] = {0};
        for (int i = 0; i < 25; i++) {
            snprintf(debugStr + i*3, 4, "%02X ", beaconData[i]);
        }
        ESP_LOGI(BLETAG, "Beacon data: %s", debugStr);

        NimBLEAdvertisementData advData;
        advData.setFlags(0x04);
        advData.setManufacturerData(beaconData, 25);

        NimBLEAdvertisementData scanData;
        scanData.setName("Chelonian");
        scanData.setCompleteServices(NimBLEUUID(BLE_SERVICE_UUID));

        adv->setAdvertisementData(advData);
        adv->setScanResponseData(scanData);
        adv->start();

        ESP_LOGI(BLETAG, "BLE advertising started with iBeacon");
    }

    // -------------------------
    // Server callbacks

    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
            ESP_LOGI(BLETAG, "Client connected: %s",
                connInfo.getAddress().toString().c_str());
            NimBLEDevice::getAdvertising()->start();
        }

        void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo,
                          int reason) override {
            ESP_LOGI(BLETAG, "Client disconnected, reason: %d", reason);
            NimBLEDevice::getAdvertising()->start();
        }

    private:
        BLEManager* _mgr;
    };

    // -------------------------
    // Command characteristic callbacks

    class CommandCallbacks : public NimBLECharacteristicCallbacks {
    public:
        CommandCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            std::string raw = pChar->getValue();
            while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r' || raw.back() == ' ')) {
                raw.pop_back();
            }
            ESP_LOGI(BLETAG, "CMD received (%d bytes): %s", raw.length(), raw.c_str());

            String payload = String(raw.c_str());
            
            // New format: deviceId|token|command|timestamp
            // Legacy format: deviceId|token|command (no timestamp)
            int sep1 = payload.indexOf('|');
            int sep2 = payload.indexOf('|', sep1 + 1);
            int sep3 = payload.lastIndexOf('|');

            if (sep1 < 0 || sep2 < 0 || sep1 == sep2) {
                ESP_LOGW(BLETAG, "Invalid command format");
                _mgr->notifyStatus("error:bad_format");
                return;
            }

            String deviceId = payload.substring(0, sep1);
            String token    = payload.substring(sep1 + 1, sep2);
            uint8_t command = (uint8_t)payload.substring(sep2 + 1, sep3 > sep2 ? sep3 : payload.length()).toInt();
            uint32_t timestamp = 0;
            
            // Parse timestamp if present (sep3 > sep2)
            if (sep3 > sep2) {
                timestamp = (uint32_t)payload.substring(sep3 + 1).toInt();
            }

            if (timestamp == 0) {
                ESP_LOGW(BLETAG, "Timestamp required for replay protection");
                _mgr->notifyStatus("error:timestamp_required");
                return;
            }

            if (deviceId.length() == 0 || token.length() != 32 || command == 0) {
                ESP_LOGW(BLETAG, "Invalid command fields");
                _mgr->notifyStatus("error:bad_fields");
                return;
            }

            uint8_t storedToken[PHONE_SECRET_LEN] = {0};
            if (!phoneTokenManager.getSecret(deviceId.c_str(), storedToken)) {
                ESP_LOGW(BLETAG, "Unknown device: %s", deviceId.c_str());
                _mgr->notifyStatus("error:unknown_device");
                return;
            }

            uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
            memcpy(incomingToken, token.c_str(),
                   min((size_t)PHONE_SECRET_LEN, token.length()));

            uint8_t diff = 0;
            for (int i = 0; i < PHONE_SECRET_LEN; i++) {
                diff |= storedToken[i] ^ incomingToken[i];
            }

            if (diff != 0) {
                ESP_LOGW(BLETAG, "Invalid token for: %s", deviceId.c_str());
                _mgr->notifyStatus("error:unauthorized");
                return;
            }

            // Validate timestamp if present (replay attack protection)
            if (timestamp > 0 && authManager.isTimeSynced()) {
                uint32_t now = authManager.getCurrentTime();
                int32_t drift = (int32_t)timestamp - (int32_t)now;
                if (drift > 30 || drift < -30) {
                    ESP_LOGW(BLETAG, "Timestamp rejected — drift: %ld seconds", drift);
                    _mgr->notifyStatus("error:timestamp_expired");
                    return;
                }
            }

            PhoneCommand cmd = static_cast<PhoneCommand>(command);
            _mgr->_onCommand(cmd);

            const char* statusStr = "unknown";
            switch (cmd) {
                case PhoneCommand::UNLOCK: statusStr = "unlocked"; break;
                case PhoneCommand::LOCK:   statusStr = "locked";   break;
                case PhoneCommand::STATUS: statusStr = "ok";       break;
                default: break;
            }

            ESP_LOGI(BLETAG, "BLE command OK: %s", statusStr);
            _mgr->notifyStatus(statusStr);
        }

    private:
        BLEManager* _mgr;
    };

    // -------------------------
    // Pairing characteristic callbacks

    class PairingCallbacks : public NimBLECharacteristicCallbacks {
    public:
        PairingCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            if (!_mgr->_pairingWindowOpen) {
                ESP_LOGW(BLETAG, "Pairing attempt outside window");
                pChar->setValue("error:window_closed");
                return;
            }

            std::string raw = pChar->getValue();
            String deviceId = String(raw.c_str());
            deviceId.trim();

            if (deviceId.length() == 0 ||
                deviceId.length() > PHONE_ID_MAX_LEN) {
                pChar->setValue("error:bad_id");
                return;
            }

            uint8_t tokenBytes[PHONE_SECRET_LEN] = {0};
            uint8_t randBytes[16];
            esp_fill_random(randBytes, sizeof(randBytes));

            char tokenHex[33];
            for (int i = 0; i < 16; i++) {
                snprintf(tokenHex + i * 2, 3, "%02x", randBytes[i]);
            }
            tokenHex[32] = 0;
            memcpy(tokenBytes, tokenHex, 32);

            phoneTokenManager.removePhone(deviceId.c_str());
            if (!phoneTokenManager.addPhone(deviceId.c_str(), tokenBytes)) {
                pChar->setValue("error:storage_full");
                return;
            }

            _mgr->_pairingWindowOpen = false;
            ESP_LOGI(BLETAG, "BLE paired: %s", deviceId.c_str());
            pChar->setValue((uint8_t*)tokenHex, 32);
        }

    private:
        BLEManager* _mgr;
    };

    // -------------------------
    // Verify characteristic callbacks

    class VerifyCallbacks : public NimBLECharacteristicCallbacks {
    public:
        VerifyCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            std::string raw = pChar->getValue();
            String deviceId = String(raw.c_str());
            deviceId.trim();

            uint8_t storedToken[PHONE_SECRET_LEN] = {0};
            bool exists = phoneTokenManager.getSecret(deviceId.c_str(), storedToken);

            pChar->setValue(exists ? "valid" : "invalid");
            ESP_LOGI(BLETAG, "Verify %s: %s", deviceId.c_str(),
                exists ? "valid" : "invalid");
        }

    private:
        BLEManager* _mgr;
    };

    // -------------------------
    // Macro configuration callbacks

    class MacroCallbacks : public NimBLECharacteristicCallbacks {
    public:
        MacroCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            std::string raw = pChar->getValue();
            ESP_LOGI(BLETAG, "Macro config write received (%d bytes)", raw.length());

            // Format: macro_count|tag_macro|macro1_name|macro1_steps|step1_relay|duration|gap|...
            // Example: "5|0|Unlock|1|1|1000|0|Lock|1|2|1000|0"
            
            String payload = String(raw.c_str());
            _parseAndSaveMacroConfig(payload);
        }

    private:
        BLEManager* _mgr;

        void _parseAndSaveMacroConfig(const String& payload) {
            // Simple pipe-separated format
            // Count the pipes to determine fields
            int pipeCount = 0;
            for (char c : payload.c_str()) {
                if (c == '|') pipeCount++;
            }

            // Expected: at least 2 pipes for header (macro_count, tag_macro)
            // Then for each macro: name_length(1) + name + steps(1) + for each step: relay(1) + duration(2) + gap(2)
            // Using simpler format: macro_count|tag_macro followed by JSON-like data
            
            // Find first two pipe positions
            int sep1 = payload.indexOf('|');
            int sep2 = payload.indexOf('|', sep1 + 1);
            
            if (sep1 < 0 || sep2 < 0) {
                ESP_LOGW(BLETAG, "Invalid macro format");
                _mgr->notifyStatus("error:macro_format");
                return;
            }

            uint8_t macroCount = payload.substring(0, sep1).toInt();
            uint8_t tagMacro = payload.substring(sep1 + 1, sep2).toInt();

            if (macroCount == 0 || macroCount > MAX_MACROS) {
                ESP_LOGW(BLETAG, "Invalid macro count: %d", macroCount);
                return;
            }

            macroConfigManager.config.macro_count = macroCount;
            macroConfigManager.config.tag_macro = tagMacro;

            // Parse remaining macros
            String remaining = payload.substring(sep2 + 1);
            for (uint8_t i = 0; i < macroCount; i++) {
                if (remaining.length() == 0) break;
                
                int delim = remaining.indexOf('|');
                if (delim < 0) delim = remaining.length();
                
                String name = remaining.substring(0, delim);
                name.trim();
                strncpy(macroConfigManager.config.macros[i].name, name.c_str(), sizeof(macroConfigManager.config.macros[i].name) - 1);
                
                remaining = remaining.substring(delim + 1);
                if (remaining.length() == 0) break;

                delim = remaining.indexOf('|');
                if (delim < 0) delim = remaining.length();
                uint8_t stepCount = remaining.substring(0, delim).toInt();
                if (stepCount > MAX_STEPS) stepCount = MAX_STEPS;
                macroConfigManager.config.macros[i].step_count = stepCount;
                
                remaining = remaining.substring(delim + 1);

                for (uint8_t s = 0; s < stepCount; s++) {
                    if (remaining.length() == 0) break;
                    
                    // Parse relay_mask (1 byte as int)
                    delim = remaining.indexOf('|');
                    if (delim < 0) delim = remaining.length();
                    macroConfigManager.config.macros[i].steps[s].relay_mask = remaining.substring(0, delim).toInt();
                    remaining = remaining.substring(delim + 1);

                    // Parse duration (2 bytes)
                    delim = remaining.indexOf('|');
                    if (delim < 0) delim = remaining.length();
                    macroConfigManager.config.macros[i].steps[s].duration = remaining.substring(0, delim).toInt();
                    remaining = remaining.substring(delim + 1);

                    // Parse gap (2 bytes)
                    delim = remaining.indexOf('|');
                    if (delim < 0) delim = remaining.length();
                    macroConfigManager.config.macros[i].steps[s].gap = remaining.substring(0, delim).toInt();
                    remaining = remaining.substring(delim + 1);
                }
            }

            macroConfigManager.saveAll();
            ESP_LOGI(BLETAG, "Macro config saved: %d macros, tag=%d", macroCount, tagMacro);
            _mgr->notifyStatus("ok:macros_saved");
        }
    };

}; // end BLEManager