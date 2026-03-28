#pragma once
#include <NimBLEDevice.h>
#include "phone_token_manager.hpp"
#include "auth_manager.hpp"
#include "esp_log.h"

static const char* BLETAG = "BLE";

// -------------------------
// BLE UUIDs — unique to this device/service
// You can regenerate these at https://www.uuidgenerator.net/

#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CMD_UUID            "beb5483e-36e1-4688-b7f5-ea07361b26a8" // write
#define BLE_STATUS_UUID         "beb5483f-36e1-4688-b7f5-ea07361b26a8" // notify
#define BLE_PAIR_UUID           "beb54840-36e1-4688-b7f5-ea07361b26a8" // write

// -------------------------
// Command byte definitions (matches PhoneCommand enum)
#define BLE_CMD_UNLOCK  0x01
#define BLE_CMD_LOCK    0x02
#define BLE_CMD_STATUS  0x03

extern PhoneTokenManager phoneTokenManager;

class BLEManager {
public:
    void begin(std::function<void(PhoneCommand)> onCommand) {
        _onCommand = onCommand;

        NimBLEDevice::init("Chelonian");
        NimBLEDevice::setMTU(128);

        // Security — require encrypted connection
        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityPasskey(0); // just works pairing for now
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

        _server = NimBLEDevice::createServer();
        _server->setCallbacks(new ServerCallbacks(this));

        // Create service
        NimBLEService* service = _server->createService(BLE_SERVICE_UUID);

        // Command characteristic — phone writes unlock/lock/status
        _cmdChar = service->createCharacteristic(
            BLE_CMD_UUID,
            NIMBLE_PROPERTY::WRITE
        );
        _cmdChar->setCallbacks(new CommandCallbacks(this));

        // Status characteristic — device notifies phone of state changes
        _statusChar = service->createCharacteristic(
            BLE_STATUS_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        // Pairing characteristic — phone writes device_id, gets token back
        _pairChar = service->createCharacteristic(
            BLE_PAIR_UUID,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
        );
        _pairChar->setCallbacks(new PairingCallbacks(this));

        service->start();
        _server->start();
        // Start advertising
        _startAdvertising();

        ESP_LOGI(BLETAG, "BLE started — device name: Chelonian");
    }

    // Call from loop()
    void update() {
        // Nothing needed — NimBLE is event driven
    }

    // Notify all connected clients of a status change
    void notifyStatus(const char* status) {
        if (_statusChar && _server->getConnectedCount() > 0) {
            _statusChar->setValue(status);
            _statusChar->notify();
            ESP_LOGI(BLETAG, "Status notified: %s", status);
        }
    }

    // Open pairing window — called from pairing button
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
    NimBLEServer*          _server     = nullptr;
    NimBLECharacteristic*  _cmdChar    = nullptr;
    NimBLECharacteristic*  _statusChar = nullptr;
    NimBLECharacteristic*  _pairChar   = nullptr;

    bool     _pairingWindowOpen  = false;
    uint32_t _pairingWindowStart = 0;

    std::function<void(PhoneCommand)> _onCommand;

    void _startAdvertising() {
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        adv->addServiceUUID(BLE_SERVICE_UUID);
        adv->setName("Chelonian");
        adv->start();
        ESP_LOGI(BLETAG, "BLE advertising started");
    }

    // -------------------------
    // Server connection callbacks

    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onConnect(NimBLEServer* server) override {
            ESP_LOGI(BLETAG, "Client connected");
            NimBLEDevice::getAdvertising()->start();
        }

        void onDisconnect(NimBLEServer* server) override {
            ESP_LOGI(BLETAG, "Client disconnected");
            NimBLEDevice::getAdvertising()->start();
        }

    private:
        BLEManager* _mgr;
    };

    // -------------------------
    // Command characteristic callbacks
    // Phone writes: "<device_id>:<token>:<command_byte>"

    class CommandCallbacks : public NimBLECharacteristicCallbacks {
    public:
        CommandCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar) override {
            std::string raw = pChar->getValue();
            ESP_LOGI(BLETAG, "CMD received: %s", raw.c_str());

            // Parse "<device_id>:<token>:<command>"
            String payload  = String(raw.c_str());
            // int sep1        = payload.indexOf(':');
            // int sep2        = payload.lastIndexOf(':');
            int sep1 = payload.indexOf('|');
            int sep2 = payload.lastIndexOf('|');


            if (sep1 < 0 || sep2 < 0 || sep1 == sep2) {
                ESP_LOGW(BLETAG, "Invalid command format");
                _mgr->notifyStatus("error:bad_format");
                return;
            }

            String deviceId = payload.substring(0, sep1);
            String token    = payload.substring(sep1 + 1, sep2);
            uint8_t command = (uint8_t)payload.substring(sep2 + 1).toInt();

            if (deviceId.length() == 0 || token.length() != 32 || command == 0) {
                ESP_LOGW(BLETAG, "Invalid command fields");
                _mgr->notifyStatus("error:bad_fields");
                return;
            }

            // Look up stored token
            uint8_t storedToken[PHONE_SECRET_LEN] = {0};
            if (!phoneTokenManager.getSecret(deviceId.c_str(), storedToken)) {
                ESP_LOGW(BLETAG, "Unknown device: %s", deviceId.c_str());
                _mgr->notifyStatus("error:unknown_device");
                return;
            }

            // Constant-time compare
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

            // Dispatch
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
    // Phone writes: "<device_id>"
    // Device responds by setting characteristic value to "<token>" or "error:..."

    class PairingCallbacks : public NimBLECharacteristicCallbacks {
    public:
        PairingCallbacks(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar) override {
            if (!_mgr->_pairingWindowOpen) {
                ESP_LOGW(BLETAG, "Pairing attempt outside window");
                pChar->setValue("error:window_closed");
                return;
            }

            std::string raw  = pChar->getValue();
            String deviceId  = String(raw.c_str());
            deviceId.trim();

            if (deviceId.length() == 0 ||
                deviceId.length() > PHONE_ID_MAX_LEN) {
                pChar->setValue("error:bad_id");
                return;
            }

            // Generate token
            uint8_t tokenBytes[PHONE_SECRET_LEN] = {0};
            uint8_t randBytes[16];
            esp_fill_random(randBytes, sizeof(randBytes));

            char tokenHex[33];
            for (int i = 0; i < 16; i++) {
                snprintf(tokenHex + i * 2, 3, "%02x", randBytes[i]);
            }
            tokenHex[32] = 0;
            memcpy(tokenBytes, tokenHex, 32);

            // Remove old entry if exists, add new
            phoneTokenManager.removePhone(deviceId.c_str());
            if (!phoneTokenManager.addPhone(deviceId.c_str(), tokenBytes)) {
                pChar->setValue("error:storage_full");
                return;
            }

            _mgr->_pairingWindowOpen = false;
            ESP_LOGI(BLETAG, "BLE paired: %s", deviceId.c_str());

            // Return token to phone
            pChar->setValue(tokenHex);
        }

    private:
        BLEManager* _mgr;
    };
};