// //Secure BLE
// #pragma once

// #include <esp_log.h>
// #include <NimBLEDevice.h>

// #include "config.hpp"
// #include "access_log.hpp"
// #include "auth_manager.hpp"
// #include "macro_config.hpp"
// #include "phone_token_manager.hpp"

// static const char* BLETAG = "BLE";

// #define BLE_SERVICE_UUID     "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define BLE_CMD_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define BLE_STATUS_UUID      "beb5483f-36e1-4688-b7f5-ea07361b26a8"
// #define BLE_PAIR_UUID        "beb54840-36e1-4688-b7f5-ea07361b26a8"
// #define BLE_BEACON_UUID_CHAR "beb54841-36e1-4688-b7f5-ea07361b26a8" // read only
// #define BLE_MAC_UUID_CHAR    "beb54842-36e1-4688-b7f5-ea07361b26a8" // read only
// #define BLE_VERIFY_UUID      "beb54843-36e1-4688-b7f5-ea07361b26a8" // write/read
// #define BLE_MACRO_GET_UUID   "beb54844-36e1-4688-b7f5-ea07361b26a8" // read macro config
// #define BLE_MACRO_SET_UUID   "beb54845-36e1-4688-b7f5-ea07361b26a8" // write macro config
// #define BLE_LOG_GET_UUID     "beb54846-36e1-4688-b7f5-ea07361b26a8" // read logs
// #define BLE_LOG_CLEAR_UUID   "beb54847-36e1-4688-b7f5-ea07361b26a8" // clear logs
// #define BLE_REBOOT_UUID      "beb54848-36e1-4688-b7f5-ea07361b26a8" // reboot
// #define BLE_TIME_SYNC_UUID   "beb54849-36e1-4688-b7f5-ea07361b26a8" //sync time
// #define BLE_CMD_UNLOCK  0x01
// #define BLE_CMD_LOCK    0x02
// #define BLE_CMD_STATUS  0x03

// extern PhoneTokenManager phoneTokenManager;
// extern AuthManager authManager;
// extern MacroConfigManager macroConfigManager;
// extern AccessLogger accessLogger;
// extern EncryptedTokenStorage encryptedStorage;

// static constexpr uint8_t CHUNK_SIZE = 3;

// class BLEManager {
// public:
//     void begin(std::function<void(PhoneCommand)> onCommand) {
//         _onCommand = onCommand;
//         ESP_LOGI(BLETAG, "BLEManager::begin() called - starting initialization");


//         NimBLEDevice::init("Chelonian");
//         NimBLEDevice::setMTU(128);

//         NimBLEDevice::setSecurityAuth(true, true, true);
//         NimBLEDevice::setSecurityPasskey(0);
//         NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

//         _server = NimBLEDevice::createServer();
//         _server->setCallbacks(new ServerCallbacks(this));

//         NimBLEService* service = _server->createService(BLE_SERVICE_UUID);

//         // Command characteristic
//         _cmdChar = service->createCharacteristic(
//             BLE_CMD_UUID,
//             NIMBLE_PROPERTY::WRITE
//         );
//         _cmdChar->setCallbacks(new CommandCallbacks(this));

//         // Status characteristic
//         // _statusChar = service->createCharacteristic(
//         //     BLE_STATUS_UUID,
//         //     NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
//         // );
//         _statusChar = service->createCharacteristic(
//             BLE_STATUS_UUID,
//             NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
//         );
//         // _statusChar->addDescriptor(new NimBLEDescriptor(BLEUUID((uint16_t)0x2902)));
//         // _statusChar->addDescriptor(new NimBLEDescriptor("2902"));
//         // _statusChar->addDescriptor(new NimBLE2904());
 

//         // Pairing characteristic
//         _pairChar = service->createCharacteristic(
//             BLE_PAIR_UUID,
//             NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
//         );
//         _pairChar->setCallbacks(new PairingCallbacks(this));

//         // Beacon UUID characteristic
//         _beaconUUIDChar = service->createCharacteristic(
//             BLE_BEACON_UUID_CHAR,
//             NIMBLE_PROPERTY::READ
//         );

//         // MAC address characteristic
//         _macChar = service->createCharacteristic(
//             BLE_MAC_UUID_CHAR,
//             NIMBLE_PROPERTY::READ
//         );

//         // Verify characteristic
//         _verifyChar = service->createCharacteristic(
//             BLE_VERIFY_UUID,
//             NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
//         );
//         _verifyChar->setCallbacks(new VerifyCallbacks(this));

//         // Macro config - read
//         _macroGetChar = service->createCharacteristic(
//             BLE_MACRO_GET_UUID,
//             NIMBLE_PROPERTY::READ
//         );
//         _macroGetChar->setCallbacks(new MacroGetCallbacks(this));

//         // Macro config - write
//         _macroSetChar = service->createCharacteristic(
//             BLE_MACRO_SET_UUID,
//             NIMBLE_PROPERTY::WRITE
//         );

//         _macroSetChar->setCallbacks(new MacroCallbacks(this));
//         ESP_LOGI(BLETAG, "Created _macroSetChar at UUID %s with WRITE permission", BLE_MACRO_SET_UUID);
        
//         // _macroSetChar->setCallbacks(new TestWriteCallback());

//         // Initialize macro config characteristic with current values
//         refreshMacroChar();

//         _logGetChar = service->createCharacteristic(
//             BLE_LOG_GET_UUID,
//             NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
//         );

//         // _logGetChar->addDescriptor(new NimBLE2904());
//         _logGetChar->setCallbacks(new LogGetCallbacks(this));
 

//         // Log - clear
//         _logClearChar = service->createCharacteristic(
//             BLE_LOG_CLEAR_UUID,
//             NIMBLE_PROPERTY::WRITE
//         );
//         _logClearChar->setCallbacks(new LogClearCallbacks(this));

//         // Reboot
//         NimBLECharacteristic* rebootChar = service->createCharacteristic(
//             BLE_REBOOT_UUID,
//             NIMBLE_PROPERTY::WRITE
//         );
//         rebootChar->setCallbacks(new RebootCallbacks());

//         // ✅ NEW: Time Sync characteristic
//         _timeSyncChar = service->createCharacteristic(
//             BLE_TIME_SYNC_UUID,
//             NIMBLE_PROPERTY::WRITE
//         );
//         _timeSyncChar->setCallbacks(new TimeSyncCallbacks(this));
//         ESP_LOGI(BLETAG, "Created _timeSyncChar at UUID %s", BLE_TIME_SYNC_UUID);
    


//         // Generate unique iBeacon UUID from MAC address
//         char beaconUUID[37];
//         _generateBeaconUUID(beaconUUID);
//         ESP_LOGI(BLETAG, "iBeacon UUID: %s", beaconUUID);

//         // Set beacon UUID characteristic value
//         _beaconUUIDChar->setValue(beaconUUID);

//         // Set MAC address characteristic value
//         uint8_t mac[6];
//         esp_read_mac(mac, ESP_MAC_BT);
//         char macStr[18];
//         snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
//             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//         _macChar->setValue(macStr);
//         ESP_LOGI(BLETAG, "BLE MAC: %s", macStr);


        
//         _server->start();

//         _startAdvertising(beaconUUID);
//         ESP_LOGI(BLETAG, "BLE started — device name: Chelonian");
//     }

//     void update() {}

//     void notifyStatus(const char* status) {
//         if (_statusChar && _server->getConnectedCount() > 0) {
//             _statusChar->setValue(status);
//             _statusChar->notify();
//             ESP_LOGI(BLETAG, "Status notified: %s", status);
//         }
//     }

//     String buildMacroConfigJson() {
//         String json = "{";
//         json += "\"macro_count\":" + String(macroConfigManager.config.macro_count) + ",";
//         json += "\"tag_macro\":" + String(macroConfigManager.config.tag_macro) + ",";
//         json += "\"macros\":[";
        
//         for (uint8_t i = 0; i < macroConfigManager.config.macro_count; i++) {
//             if (i > 0) json += ",";
//             Macro& m = macroConfigManager.config.macros[i];
//             json += "{";
//             json += "\"name\":\"" + String(m.name) + "\",";
//             json += "\"step_count\":" + String(m.step_count) + ",";
//             json += "\"steps\":[";
//             for (uint8_t s = 0; s < m.step_count; s++) {
//                 if (s > 0) json += ",";
//                 json += "{";
//                 json += "\"relay_mask\":" + String(m.steps[s].relay_mask) + ",";
//                 json += "\"duration\":" + String(m.steps[s].duration) + ",";
//                 json += "\"gap\":" + String(m.steps[s].gap);
//                 json += "}";
//             }
//             json += "]";
//             json += "}";
//         }
//         json += "]}";
//         return json;
//     }

//     void refreshMacroChar() {
//         if (_macroGetChar) {
//             String json = buildMacroConfigJson();
//             _macroGetChar->setValue(json.c_str());
//             ESP_LOGI(BLETAG, "Macro config refreshed (%d bytes)", json.length());
//         } else {
//             ESP_LOGW(BLETAG, "refreshMacroChar: _macroGetChar is null!");
//         }
//     }

//     void refreshLogsChar() {
//         if (_logGetChar) {
//             String json = accessLogger.getLogsJson(-1, 0);
//             _logGetChar->setValue(json.c_str());
//             ESP_LOGI(BLETAG, "Logs refreshed (%d bytes): %s", json.length(), json.c_str());
//             ESP_LOGI(BLETAG, "Log count: %u", accessLogger.getCount());
//         } else {
//             ESP_LOGW(BLETAG, "refreshLogsChar: _logGetChar is null!");
//         }
//     }

//     void openPairingWindow() {
//         _pairingWindowOpen  = true;
//         _pairingWindowStart = millis();
//         // ESP_LOGI(BLETAG, "BLE pairing window opened (60s)");
//         ESP_LOGI(BLETAG, "BLE pairing window opened (%lus)", PAIRING_WINDOW_MS / 1000);
//     }

//     bool isPairingWindowOpen() {
//         return _pairingWindowOpen;
//     }

//     void updatePairingWindow() {
//         if (_pairingWindowOpen &&
//             (millis() - _pairingWindowStart > PAIRING_WINDOW_MS)) {
//             _pairingWindowOpen = false;
//             ESP_LOGI(BLETAG, "BLE pairing window closed (timeout)");
//         }
//     }

// private:
//     NimBLEServer*          _server         = nullptr;
//     NimBLECharacteristic*  _cmdChar        = nullptr;
//     NimBLECharacteristic*  _statusChar     = nullptr;
//     NimBLECharacteristic*  _pairChar       = nullptr;
//     NimBLECharacteristic*  _beaconUUIDChar = nullptr;
//     NimBLECharacteristic*  _macChar        = nullptr;
//     NimBLECharacteristic*  _verifyChar     = nullptr;
//     NimBLECharacteristic*  _macroGetChar   = nullptr;
//     NimBLECharacteristic*  _macroSetChar   = nullptr;
//     NimBLECharacteristic*  _logGetChar     = nullptr;
//     NimBLECharacteristic*  _logClearChar   = nullptr;
//     NimBLECharacteristic*  _timeSyncChar   = nullptr;

//     bool     _pairingWindowOpen  = false;
//     uint32_t _pairingWindowStart = 0;

//     std::function<void(PhoneCommand)> _onCommand;

//     void _generateBeaconUUID(char* uuidOut) {
//         uint8_t mac[6];
//         esp_read_mac(mac, ESP_MAC_BT);
//         snprintf(uuidOut, 37,
//             "43484c4e-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
//             mac[0], mac[1],
//             mac[2], mac[3],
//             mac[4], mac[5],
//             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
//         );
//     }

//     void _parseUUID(const char* uuidStr, uint8_t* out) {
//         char hex[33] = {0};
//         int j = 0;
//         for (int i = 0; uuidStr[i] && j < 32; i++) {
//             if (uuidStr[i] != '-') hex[j++] = uuidStr[i];
//         }
//         for (int i = 0; i < 16; i++) {
//             char byte[3] = { hex[i*2], hex[i*2+1], 0 };
//             out[i] = (uint8_t)strtol(byte, nullptr, 16);
//         }
//     }

//     void _startAdvertising(const char* beaconUUID) {
//         NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

//         uint8_t beaconData[25];
//         beaconData[0] = 0x4C;
//         beaconData[1] = 0x00;
//         beaconData[2] = 0x02;
//         beaconData[3] = 0x15;
//         _parseUUID(beaconUUID, &beaconData[4]);
//         beaconData[20] = (IBEACON_MAJOR >> 8) & 0xFF;
//         beaconData[21] =  IBEACON_MAJOR       & 0xFF;
//         beaconData[22] = (IBEACON_MINOR >> 8) & 0xFF;
//         beaconData[23] =  IBEACON_MINOR       & 0xFF;
//         beaconData[24] =  IBEACON_TX_POWER;

//         char debugStr[100] = {0};
//         for (int i = 0; i < 25; i++) {
//             snprintf(debugStr + i*3, 4, "%02X ", beaconData[i]);
//         }
//         ESP_LOGI(BLETAG, "Beacon data: %s", debugStr);

//         NimBLEAdvertisementData advData;
//         advData.setFlags(0x04);
//         advData.setManufacturerData(beaconData, 25);

//         NimBLEAdvertisementData scanData;
//         scanData.setName("Chelonian");
//         scanData.setCompleteServices(NimBLEUUID(BLE_SERVICE_UUID));

//         adv->setAdvertisementData(advData);
//         adv->setScanResponseData(scanData);
//         adv->start();

//         ESP_LOGI(BLETAG, "BLE advertising started with iBeacon");
//     }

//     // -------------------------
//     // Server callbacks

//     class ServerCallbacks : public NimBLEServerCallbacks {
//     public:
//         ServerCallbacks(BLEManager* mgr) : _mgr(mgr) {}

//         void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
//             ESP_LOGI(BLETAG, "Client connected: %s",
//                 connInfo.getAddress().toString().c_str());
//             NimBLEDevice::getAdvertising()->start();
//         }

//         void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo,
//                           int reason) override {
//             ESP_LOGI(BLETAG, "Client disconnected, reason: %d", reason);
//             NimBLEDevice::getAdvertising()->start();
//         }

//     private:
//         BLEManager* _mgr;
//     };

//     // -------------------------
//     // Command characteristic callbacks

//     class CommandCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         CommandCallbacks(BLEManager* mgr) : _mgr(mgr) {}

//         void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             std::string raw = pChar->getValue();
//             while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r' || raw.back() == ' ')) {
//                 raw.pop_back();
//             }
//             ESP_LOGI(BLETAG, "CMD received (%d bytes): %s", raw.length(), raw.c_str());

//             String payload = String(raw.c_str());

//             int sep1 = payload.indexOf('|');
//             int sep2 = payload.indexOf('|', sep1 + 1);
//             int sep3 = payload.indexOf('|', sep2 + 1);
//             int sep4 = payload.lastIndexOf('|');

//             if (sep1 < 0 || sep2 < 0 || sep1 == sep2) {
//                 ESP_LOGW(BLETAG, "Invalid command format");
//                 _mgr->notifyStatus("error:bad_format");
//                 return;
//             }

//             String deviceId = payload.substring(0, sep1);
//             String token    = payload.substring(sep1 + 1, sep2);
//             uint8_t command = (uint8_t)payload.substring(sep2 + 1, sep3 > sep2 ? sep3 : payload.length()).toInt();
//             uint32_t timestamp = 0;
//             String source = "M";

//             if (sep3 > sep2) {
//                 timestamp = (uint32_t)payload.substring(sep3 + 1, sep4 > sep3 ? sep4 : payload.length()).toInt();
//             }
//             if (sep4 > sep3) {
//                 source = payload.substring(sep4 + 1);
//             } else {
//                 source = "M";
//             }

//             if (timestamp == 0) {
//                 ESP_LOGW(BLETAG, "Timestamp required for replay protection");
//                 _mgr->notifyStatus("error:timestamp_required");
//                 return;
//             }

//             if (deviceId.length() == 0 || token.length() != 64 || command == 0) {
//                 ESP_LOGW(BLETAG, "Invalid command fields");
//                 _mgr->notifyStatus("error:bad_fields");
//                 return;
//             }

//             uint8_t storedToken[PHONE_SECRET_LEN] = {0};
//             if (!phoneTokenManager.getSecret(deviceId.c_str(), storedToken)) {
//                 ESP_LOGW(BLETAG, "Unknown device: %s", deviceId.c_str());
//                 _mgr->notifyStatus("error:unknown_device");
//                 return;
//             }

//             uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
//             for (int i = 0; i < PHONE_SECRET_LEN && i * 2 < (int)token.length(); i++) {
//                 char byte[3] = { token.c_str()[i*2], token.c_str()[i*2+1], 0 };
//                 incomingToken[i] = (uint8_t)strtol(byte, nullptr, 16);
//             }

//             uint8_t diff = 0;
//             for (int i = 0; i < PHONE_SECRET_LEN; i++) {
//                 diff |= storedToken[i] ^ incomingToken[i];
//             }

//             if (diff != 0) {
//                 ESP_LOGW(BLETAG, "Invalid token for: %s", deviceId.c_str());
//                 _mgr->notifyStatus("error:unauthorized");
//                 return;
//             }

//             if (timestamp > 1000000000) {
//                 authManager.syncTime(timestamp);
//             }

//             PhoneCommand cmd = static_cast<PhoneCommand>(command);
//             _mgr->_onCommand(cmd);

//             const char* statusStr = "unknown";
//             switch (cmd) {
//                 case PhoneCommand::UNLOCK: statusStr = "unlocked"; break;
//                 case PhoneCommand::LOCK:   statusStr = "locked";   break;
//                 case PhoneCommand::STATUS: statusStr = "ok";       break;
//                 default: break;
//             }

//             // Log command with source (P = Proximity, M = Manual)
//             LogSource logSrc = (source == "P") ? LogSource::PROXIMITY : LogSource::BLE;
//             const char* cmdMsg = (command == 1) ? "Unlock" : (command == 2) ? "Lock" : "Command";
//             accessLogger.logAccess(logSrc, LogResult::SUCCESS, deviceId.c_str(), cmdMsg);

//             ESP_LOGI(BLETAG, "BLE command OK: %s (source: %s)", statusStr, source.c_str());
//             _mgr->notifyStatus(statusStr);
//         }

//     private:
//         BLEManager* _mgr;
//     };

//         // -------------------------
//         // Pairing characteristic callbacks

//         class PairingCallbacks : public NimBLECharacteristicCallbacks {
//         public:
//             PairingCallbacks(BLEManager* mgr) : _mgr(mgr) {}

//             void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             if (!_mgr->_pairingWindowOpen) {
//                 ESP_LOGW(BLETAG, "Pairing attempt outside window");
//                 pChar->setValue("error:window_closed");
//                 return;
//             }

//             std::string raw = pChar->getValue();
//             String deviceId = String(raw.c_str());
//             deviceId.trim();

//             if (deviceId.length() == 0 ||
//                 deviceId.length() > PHONE_ID_MAX_LEN) {
//                 pChar->setValue("error:bad_id");
//                 return;
//             }

//             // Generate random token (32 bytes)
//             uint8_t tokenBytes[PHONE_SECRET_LEN] = {0};
//             esp_fill_random(tokenBytes, sizeof(tokenBytes));

//             // Encrypt token to 60 bytes (12 IV + 32 ciphertext + 16 tag)
//             uint8_t encryptedToken[60] = {0};
//             if (!encryptedStorage.encryptToken(tokenBytes, encryptedToken)) {
//                 ESP_LOGE(BLETAG, "Token encryption failed");
//                 pChar->setValue("error:encryption_failed");
//                 return;
//             }

//             // Store encrypted token
//             phoneTokenManager.removePhone(deviceId.c_str());
//             if (!phoneTokenManager.addPhoneEncrypted(deviceId.c_str(), encryptedToken)) {
//                 pChar->setValue("error:storage_full");
//                 return;
//             }

//             _mgr->_pairingWindowOpen = false;
//             ESP_LOGI(BLETAG, "BLE paired: %s", deviceId.c_str());
            
//             // Return plain token hex to client (for their records)
//             char tokenHex[65];
//             for (int i = 0; i < 32; i++) {
//                 snprintf(tokenHex + i * 2, 3, "%02x", tokenBytes[i]);
//             }
//             tokenHex[64] = 0;
//             pChar->setValue((uint8_t*)tokenHex, 64);
//         }

//     private:
//         BLEManager* _mgr;
//     };

//     // -------------------------
//     // Verify characteristic callbacks

//     class VerifyCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         VerifyCallbacks(BLEManager* mgr) : _mgr(mgr) {}

//         void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             std::string raw = pChar->getValue();
//             String deviceId = String(raw.c_str());
//             deviceId.trim();

//             uint8_t storedToken[PHONE_SECRET_LEN] = {0};
//             bool exists = phoneTokenManager.getSecret(deviceId.c_str(), storedToken);

//             pChar->setValue(exists ? "valid" : "invalid");
//             ESP_LOGI(BLETAG, "Verify %s: %s", deviceId.c_str(),
//                 exists ? "valid" : "invalid");
//         }

//     private:
//         BLEManager* _mgr;
//     };

//     // -------------------------
//     // Macro GET callbacks - reloads from NVS on each read

//     class MacroGetCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         MacroGetCallbacks(BLEManager* mgr) : _mgr(mgr) {}

//         void onRead(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             macroConfigManager.load();
//             String json = _mgr->buildMacroConfigJson();
//             pChar->setValue(json.c_str());
//         }
//     private:
//         BLEManager* _mgr;
//     };

//     // -------------------------
//     // Log GET callbacks - reads from NVS with chunked support

//     // Change LogGetCallbacks to support both onRead and onWrite
// class LogGetCallbacks : public NimBLECharacteristicCallbacks {
// public:
//     LogGetCallbacks(BLEManager* mgr) : _mgr(mgr), _byteOffset(0) {}

//     void onRead(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//         String allLogsJson = accessLogger.getLogsJson(-1, 0);  // Fresh every time
//         const size_t CHUNK_BYTES = 500;
        
//         if (_byteOffset >= allLogsJson.length()) {
//             pChar->setValue("__END__");
//             ESP_LOGI(BLETAG, "Sent END at byte %d", _byteOffset);
//         } else {
//             String chunk = allLogsJson.substring(_byteOffset, _byteOffset + CHUNK_BYTES);
//             pChar->setValue(chunk.c_str());
//             _byteOffset += CHUNK_BYTES;
//             ESP_LOGI(BLETAG, "Sent bytes %d-%d", _byteOffset - CHUNK_BYTES, _byteOffset);
//         }
//     }

//     void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//         std::string cmd = pChar->getValue();
//         if (cmd == "reset") {
//             _byteOffset = 0;
//             ESP_LOGI(BLETAG, "Reset offset to 0");
//         }
//     }

//     private:
//         size_t _byteOffset;
//         String allLogsJson;  // Cache the full JSON
//         BLEManager* _mgr;
// };

//     // -------------------------
//     // Macro SET callbacks

//     class MacroCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         MacroCallbacks(BLEManager* mgr) : _mgr(mgr) {}

// void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             std::string raw = pChar->getValue();
//             ESP_LOGI(BLETAG, "Macro config write received (%d bytes)", raw.length());

//             String payload = String(raw.c_str());
            
//             // Format: deviceId|token|timestamp|macro_count|tag_macro|macro1_name|steps|relay|duration|gap|...
//             // Example: "uuid|token|1234567890|5|0|Unlock|1|1|1000|0|Lock|1|2|1000|0"
            
//             // Skip auth - just pass entire payload to parser
//             _parseAndSaveMacroConfig(payload);
//             return;
            
//             // Validate token
//             // COMMENTED OUT
//             // uint8_t storedToken[PHONE_SECRET_LEN] = {0};
//             // if (!phoneTokenManager.getSecret(deviceId.c_str(), storedToken)) {
//             //     ESP_LOGW(BLETAG, "Unknown device: %s", deviceId.c_str());
//             //     _mgr->notifyStatus("error:unknown_device");
//             //     return;
//             // }

//             // uint8_t incomingToken[PHONE_SECRET_LEN] = {0};
//             // memcpy(incomingToken, token.c_str(),
//             //        min((size_t)PHONE_SECRET_LEN, token.length()));

//             // uint8_t diff = 0;
//             // for (int i = 0; i < PHONE_SECRET_LEN; i++) {
//             //     diff |= storedToken[i] ^ incomingToken[i];
//             // }

//             // if (diff != 0) {
//             //     ESP_LOGW(BLETAG, "Invalid token for: %s", deviceId.c_str());
//             //     _mgr->notifyStatus("error:unauthorized");
//             //     return;
//             // }
            
//             // Sync time
//             // COMMENTED OUT
//             // if (timestamp > 1000000000) {
//             //     authManager.syncTime(timestamp);
//             // }
            
//             // Validate timestamp
//             // COMMENTED OUT
//             // if (authManager.isTimeSynced()) {
//             //     uint32_t now = authManager.getCurrentTime();
//             //     int32_t drift = (int32_t)timestamp - (int32_t)now;
//             //     if (drift > 30 || drift < -30) {
//             //         ESP_LOGW(BLETAG, "Timestamp rejected — drift: %ld seconds", drift);
//             //         _mgr->notifyStatus("error:timestamp_expired");
//             //         return;
//             //     }
//             // }
            
//             // Parse macro data - pass full payload
//             _parseAndSaveMacroConfig(payload);
//         }

//     private:
//         BLEManager* _mgr;

//     void _parseAndSaveMacroConfig(const String& payload) {
//         ESP_LOGI(BLETAG, "Parsing macro config payload: %s", payload.c_str());
        
        
//         int sep1 = payload.indexOf('|');
//         int sep2 = payload.indexOf('|', sep1 + 1);
        
//         if (sep1 < 0 || sep2 < 0) {
//             ESP_LOGW(BLETAG, "Invalid macro format - sep1=%d, sep2=%d", sep1, sep2);
//             _mgr->notifyStatus("error:macro_format");
//             return;
//         }

//         uint8_t macroCount = payload.substring(0, sep1).toInt();
//         uint8_t tagMacro = payload.substring(sep1 + 1, sep2).toInt();
//         ESP_LOGI(BLETAG, "Parsed: macroCount=%d, tagMacro=%d", macroCount, tagMacro);

//         if (macroCount == 0 || macroCount > MAX_MACROS) {
//             ESP_LOGW(BLETAG, "Invalid macro count: %d", macroCount);
//             return;
//         }

//         macroConfigManager.config.macro_count = macroCount;
//         macroConfigManager.config.tag_macro = tagMacro;

//         // Parse remaining macros
//         String remaining = payload.substring(sep2 + 1);
//         for (uint8_t i = 0; i < macroCount; i++) {
//             if (remaining.length() == 0) break;
            
//             int delim = remaining.indexOf('|');
//             if (delim < 0) delim = remaining.length();
            
//             String name = remaining.substring(0, delim);
//             name.trim();
//             strncpy(macroConfigManager.config.macros[i].name, name.c_str(), sizeof(macroConfigManager.config.macros[i].name) - 1);
//             macroConfigManager.config.macros[i].name[sizeof(macroConfigManager.config.macros[i].name) - 1] = '\0';
            
//             // Clear icon and set magic with timestamp
//             memset(macroConfigManager.config.macros[i].icon, 0, sizeof(macroConfigManager.config.macros[i].icon));
//             macroConfigManager.config.macros[i].magic = MACRO_MAGIC;
//             macroConfigManager.config.macros[i].updated_at = millis();
            
//             ESP_LOGI(BLETAG, "Macro %d: name='%s', magic=0x%08X", i, name.c_str(), MACRO_MAGIC);
            
//             remaining = remaining.substring(delim + 1);
//             if (remaining.length() == 0) break;

//             delim = remaining.indexOf('|');
//             if (delim < 0) delim = remaining.length();
//             uint8_t stepCount = remaining.substring(0, delim).toInt();
//             if (stepCount > MAX_STEPS) stepCount = MAX_STEPS;
//             macroConfigManager.config.macros[i].step_count = stepCount;
            
//             // Clear any unused steps
//             for (uint8_t s = stepCount; s < MAX_STEPS; s++) {
//                 macroConfigManager.config.macros[i].steps[s].relay_mask = 0;
//                 macroConfigManager.config.macros[i].steps[s].duration = 0;
//                 macroConfigManager.config.macros[i].steps[s].gap = 0;
//             }
            
//             ESP_LOGI(BLETAG, "Macro %d: stepCount=%d", i, stepCount);
            
//             remaining = remaining.substring(delim + 1);

//             for (uint8_t s = 0; s < stepCount; s++) {
//                 if (remaining.length() == 0) break;
                
//                 delim = remaining.indexOf('|');
//                 if (delim < 0) delim = remaining.length();
//                 uint16_t relayMask = remaining.substring(0, delim).toInt();
//                 macroConfigManager.config.macros[i].steps[s].relay_mask = relayMask;
//                 remaining = remaining.substring(delim + 1);

//                 delim = remaining.indexOf('|');
//                 if (delim < 0) delim = remaining.length();
//                 uint16_t duration = remaining.substring(0, delim).toInt();
//                 macroConfigManager.config.macros[i].steps[s].duration = duration;
//                 remaining = remaining.substring(delim + 1);

//                 delim = remaining.indexOf('|');
//                 if (delim < 0) delim = remaining.length();
//                 uint16_t gap = remaining.substring(0, delim).toInt();
//                 macroConfigManager.config.macros[i].steps[s].gap = gap;
//                 remaining = remaining.substring(delim + 1);
                
//                 ESP_LOGI(BLETAG, "  Step %d: relay=%d, duration=%d, gap=%d", s, relayMask, duration, gap);
//             }
//         }
        
//         ESP_LOGI(BLETAG, "Parsed %d macros, calling saveAll()", macroConfigManager.config.macro_count);
//         macroConfigManager.printConfig();

//         macroConfigManager.saveAll();
//         ESP_LOGI(BLETAG, "Macro config saved: %d macros, tag=%d", macroCount, tagMacro);

//         // accessLogger.logSystem()
//         accessLogger.logSystem(LogSource::BLE, LogResult::SUCCESS, "System", "Macro Configuration Updated");

        
//         ESP_LOGI(BLETAG, "Calling refreshMacroChar() after save");
//         _mgr->refreshMacroChar();
//         _mgr->notifyStatus("ok:macros_saved");
//     }
//     };

//     // TEST: Simple write callback to see if ANY writes are detected
//     class TestWriteCallback : public NimBLECharacteristicCallbacks {
//     public:
//         void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             ESP_LOGI(BLETAG, "TEST CALLBACK: Got a write!");
//         }
//     };


//     // Log clear callbacks
//     class LogClearCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         LogClearCallbacks(BLEManager* mgr) : _mgr(mgr) {}

//         void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             std::string raw = pChar->getValue();
//             ESP_LOGI(BLETAG, "Log clear command received");
//             accessLogger.clear();
//             accessLogger.logSystem(LogSource::BLE, LogResult::SUCCESS, "System", "Logs cleared");
//             _mgr->notifyStatus("ok:logs_cleared");
//         }

//     private:
//         BLEManager* _mgr;
//     };

//     // Reboot callbacks
//     class RebootCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             ESP_LOGI(BLETAG, "Reboot command received");
//             accessLogger.logSystem(LogSource::BLE, LogResult::SUCCESS, "System", "Manual Reboot");

//             delay(1000);
//             ESP.restart();
//         }
//     };

//     class TimeSyncCallbacks : public NimBLECharacteristicCallbacks {
//     public:
//         TimeSyncCallbacks(BLEManager* mgr) : _mgr(mgr) {}
    
//         void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
//             std::string raw = pChar->getValue();
            
//             // Parse timestamp from request
//             uint32_t timestamp = (uint32_t)atoi(raw.c_str());
            
//             if (timestamp == 0) {
//                 ESP_LOGW(BLETAG, "TimeSync: Invalid timestamp");
//                 pChar->setValue("error:invalid_timestamp");
//                 return;
//             }
            
//             if (timestamp <= 1000000000) {
//                 ESP_LOGW(BLETAG, "TimeSync: Timestamp too old: %lu", (unsigned long)timestamp);
//                 pChar->setValue("error:timestamp_too_old");
//                 return;
//             }
            
//             // ✅ SYNC THE TIME
//             ESP_LOGI(BLETAG, "TimeSync: Syncing device time to %lu", (unsigned long)timestamp);
//             authManager.syncTime(timestamp);
            
//             // Acknowledge success
//             pChar->setValue("ok");
//             ESP_LOGI(BLETAG, "TimeSync: Complete");
//         }
    
//     private:
//         BLEManager* _mgr;
//     };

// }; // end BLEManager