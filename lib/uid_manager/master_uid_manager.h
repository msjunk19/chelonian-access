#pragma once
#include <EEPROM.h>
#include <config.hpp>


class MasterUIDManager {
public:
    bool hasMasterUIDs = false;

    void clearAll() {
    for (uint16_t addr = 0; addr < EEPROM_SIZE; addr++) {
        EEPROM.write(addr, 0xFF);  // mark all bytes as empty
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared.");
    }

    void printUID(uint8_t* uid, uint8_t len) {
    char uidStr[50] = "";

    for (uint8_t i = 0; i < len; i++) {
        char hexBuf[4];
        sprintf(hexBuf, "%02X ", uid[i]);  // space instead of colon
        strcat(uidStr, hexBuf);
    }

    Serial.println(uidStr);
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


bool readUIDs() {
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

        uint8_t uid[7];

        for (int i = 0; i < len; i++) {
            uid[i] = EEPROM.read(addr++);
        }

        printUID(uid, len);
    }

    if (index == 0) {
        Serial.println("No master UIDs stored in EEPROM!");
        hasMasterUIDs = false;
        return false;
    }

    hasMasterUIDs = true;
    return true;
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
        uint16_t uidStart = addr;          // Start of length byte
        uint8_t storedLen = EEPROM.read(addr++);
        if (storedLen == 0xFF || storedLen == 0) break;

        // Compare bytes
        bool match = (storedLen == len);
        for (int i = 0; i < len && match; i++) {
            if (EEPROM.read(addr + i) != uid[i]) {
                match = false;
            }
        }

        if (match) {
            uint16_t nextAddr = addr + storedLen; // Start of next UID
            // Shift all remaining bytes left
            while (nextAddr < EEPROM_SIZE) {
                EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
            }
            // Fill the remaining space with 0xFF
            while (uidStart < EEPROM_SIZE) {
                EEPROM.write(uidStart++, 0xFF);
            }

            EEPROM.commit();
            Serial.print("UID removed: ");
            printUID(uid, len);
            return true;
        }

        addr += storedLen; // Move to next UID
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


};