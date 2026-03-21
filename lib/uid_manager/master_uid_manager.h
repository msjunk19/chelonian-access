#include <EEPROM.h>

#define MAX_MASTERS 3
#define UID_LEN 7
#define EEPROM_START 0
#define OBFUSCATION_KEY 0xAA

class MasterUIDManager {
public:
    MasterUIDManager() {
        EEPROM.begin(MAX_MASTERS * UID_LEN + MAX_MASTERS);
        loadFromEEPROM();
    }

    // Check if a UID is master
    bool isMaster(const uint8_t* uid, uint8_t length) {
        for (int i = 0; i < MAX_MASTERS; i++) {
            if (masterLengths[i] == length) {
                bool match = true;
                for (int j = 0; j < length; j++) {
                    if ((masterUIDs[i][j] ^ OBFUSCATION_KEY) != uid[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) return true;
            }
        }
        return false;
    }

    // Add a master UID manually or via first-boot learning
    bool addMaster(const uint8_t* uid, uint8_t length) {
        for (int i = 0; i < MAX_MASTERS; i++) {
            if (masterLengths[i] == 0) { // empty slot
                masterLengths[i] = length;
                for (int j = 0; j < length; j++) {
                    masterUIDs[i][j] = uid[j] ^ OBFUSCATION_KEY;
                }
                saveToEEPROM();
                return true;
            }
        }
        return false; // no free slot
    }

    // Remove a master UID
    bool removeMaster(const uint8_t* uid, uint8_t length) {
        for (int i = 0; i < MAX_MASTERS; i++) {
            if (masterLengths[i] == length) {
                bool match = true;
                for (int j = 0; j < length; j++) {
                    if ((masterUIDs[i][j] ^ OBFUSCATION_KEY) != uid[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    masterLengths[i] = 0;
                    saveToEEPROM();
                    return true;
                }
            }
        }
        return false;
    }

    // First boot / learning mode
    bool firstBootLearning(const uint8_t* uid, uint8_t length) {
        if (noMasters()) {
            addMaster(uid, length);
            return true; // indicate learning happened
        }
        return false;
    }

private:
    uint8_t masterUIDs[MAX_MASTERS][UID_LEN] = {0};
    uint8_t masterLengths[MAX_MASTERS] = {0};

    void saveToEEPROM() {
        for (int i = 0; i < MAX_MASTERS; i++) {
            int addr = EEPROM_START + i * UID_LEN;
            for (int j = 0; j < UID_LEN; j++) {
                EEPROM.write(addr + j, masterUIDs[i][j]);
            }
        }
        for (int i = 0; i < MAX_MASTERS; i++) {
            EEPROM.write(EEPROM_START + MAX_MASTERS * UID_LEN + i, masterLengths[i]);
        }
        EEPROM.commit();
    }

    void loadFromEEPROM() {
        for (int i = 0; i < MAX_MASTERS; i++) {
            int addr = EEPROM_START + i * UID_LEN;
            for (int j = 0; j < UID_LEN; j++) {
                masterUIDs[i][j] = EEPROM.read(addr + j);
            }
        }
        for (int i = 0; i < MAX_MASTERS; i++) {
            masterLengths[i] = EEPROM.read(EEPROM_START + MAX_MASTERS * UID_LEN + i);
        }
    }

    bool noMasters() {
        for (int i = 0; i < MAX_MASTERS; i++) {
            if (masterLengths[i] != 0) return false;
        }
        return true;
    }
};