#include <EEPROM.h>

#define EEPROM_SIZE 128   // Total EEPROM size to use

class MasterUIDManager {
public:
    MasterUIDManager(uint16_t eepromSize = EEPROM_SIZE) {
        EEPROM.begin(eepromSize);
    }

    void writeUIDs(uint8_t** uids, uint8_t* lengths, size_t count) {
        uint16_t addr = findFirstFreeAddress();

        for (size_t i = 0; i < count; i++) {
            uint8_t len = lengths[i];

            if (addr + 1 + len > EEPROM_SIZE) {
                Serial.println("Not enough EEPROM space to append all UIDs!");
                break;
            }

            if (checkUID(uids[i], len)) {
                Serial.print("UID already exists, skipping: ");
                printUID(uids[i], len);
                continue;
            }

            EEPROM.write(addr++, len);
            for (int j = 0; j < len; j++) {
                EEPROM.write(addr++, uids[i][j]);
            }
        }

        EEPROM.commit();
        Serial.println("UIDs written/appended to EEPROM!");
    }

    void readUIDs() {
        uint16_t addr = 0;
        int index = 0;

        while (addr < EEPROM_SIZE) {
            uint8_t len = EEPROM.read(addr++);
            if (len == 0xFF || len == 0) break;

            Serial.print("UID #");
            Serial.print(index++);
            Serial.print(" (length ");
            Serial.print(len);
            Serial.print("): ");

            for (int i = 0; i < len; i++) {
                Serial.print(EEPROM.read(addr++), HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
    }

    bool checkUID(uint8_t* uid, uint8_t len) {
        uint16_t addr = 0;

        while (addr < EEPROM_SIZE) {
            uint8_t storedLen = EEPROM.read(addr++);
            if (storedLen == 0xFF || storedLen == 0) break;

            if (storedLen == len) {
                bool match = true;
                for (int i = 0; i < len; i++) {
                    if (EEPROM.read(addr + i) != uid[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) return true;
            }
            addr += storedLen;
        }

        return false;
    }

    // Remove a UID from EEPROM
    bool removeUID(uint8_t* uid, uint8_t len) {
        uint16_t addr = 0;

        while (addr < EEPROM_SIZE) {
            uint16_t uidStart = addr;
            uint8_t storedLen = EEPROM.read(addr++);
            if (storedLen == 0xFF || storedLen == 0) break;

            bool match = (storedLen == len);
            for (int i = 0; i < len && match; i++) {
                if (EEPROM.read(addr + i) != uid[i]) {
                    match = false;
                }
            }

            if (match) {
                // Shift all subsequent UIDs left to overwrite this one
                uint16_t nextAddr = addr + storedLen;
                while (nextAddr < EEPROM_SIZE) {
                    uint8_t b = EEPROM.read(nextAddr);
                    EEPROM.write(addr, b);
                    addr++;
                    nextAddr++;
                }

                // Clear remaining space at the end
                while (addr < EEPROM_SIZE) {
                    EEPROM.write(addr++, 0xFF);
                }

                EEPROM.commit();
                Serial.print("UID removed: ");
                printUID(uid, len);
                return true;
            }

            addr += storedLen; // move to next UID
        }

        Serial.println("UID not found to remove.");
        return false;
    }

private:
    uint16_t findFirstFreeAddress() {
        uint16_t addr = 0;
        while (addr < EEPROM_SIZE) {
            uint8_t len = EEPROM.read(addr);
            if (len == 0xFF || len == 0) break;
            addr += 1 + len;
        }
        return addr;
    }

    void printUID(uint8_t* uid, uint8_t len) {
        for (int i = 0; i < len; i++) {
            Serial.print(uid[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
};