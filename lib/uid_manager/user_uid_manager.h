#pragma once
#include <EEPROM.h>
#include <config.hpp>

// static constexpr uint8_t UID_MAX_LEN = 7; // maximum UID length allowed

class UserUIDManager {
public:
    bool hasUserUIDs = false;

    //Clear User Region
    void clearUsers() {
        for (uint16_t addr = USER_START; addr < USER_START + USER_SIZE; addr++) {
            EEPROM.write(addr, 0xFF);
        }
        EEPROM.commit();
        Serial.println("User EEPROM cleared.");
    }

    // Print a UID nicely
    void printUID(uint8_t* uid, uint8_t len) {
        char uidStr[50] = "";
        for (uint8_t i = 0; i < len; i++) {
            char buf[4];
            sprintf(buf, "%02X ", uid[i]);
            strcat(uidStr, buf);
        }
        Serial.println(uidStr);
    }

        // Read all master UIDs
        bool readUIDs() {
            uint16_t addr = MASTER_START;
            int index = 0;

            while (addr < MASTER_START + MASTER_SIZE) {
                uint8_t len = EEPROM.read(addr++);
                if (len == 0xFF || len == 0) break; // end marker
                if (len > UID_MAX_LEN) {
                    Serial.println("Invalid UID length detected, aborting scan.");
                    break;
                }

                uint8_t uid[UID_MAX_LEN];
                for (int i = 0; i < len; i++) {
                    uid[i] = EEPROM.read(addr++);
                }

                Serial.print("UID #");
                Serial.print(index++);
                Serial.print(" (length ");
                Serial.print(len);
                Serial.print("): ");
                printUID(uid, len);
            }

            hasUserUIDs = (index > 0);
            if (!hasUserUIDs) Serial.println("No User UIDs stored in EEPROM!");
            return hasUserUIDs;
        }
        
        bool checkUID(uint8_t* uid, uint8_t len) {
            if (len == 0 || len > UID_MAX_LEN) return false;

            uint16_t addr = USER_START;
            while (addr < USER_START + USER_SIZE) {
                uint8_t storedLen = EEPROM.read(addr++);
                if (storedLen == 0xFF || storedLen == 0) break;
                if (storedLen > UID_MAX_LEN) {
                    Serial.println("Invalid stored UID length detected, aborting check.");
                    break;
                }

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

                addr += (storedLen > 0 ? storedLen : 1);
            }

            return false;
        }

        bool addUID(uint8_t* uid, uint8_t len) {
            if (checkUID(uid, len)) return false;

            uint16_t addr = findFirstFreeAddress();

            if (addr + 1 + len > USER_START + USER_SIZE) return false;

            EEPROM.write(addr++, len);
            for (int i = 0; i < len; i++) {
                EEPROM.write(addr++, uid[i]);
            }

            EEPROM.commit();
            return true;
        }

    bool removeUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;

        uint16_t addr = USER_START;
        while (addr < USER_START + USER_SIZE) {
            uint16_t uidStart = addr;
            uint8_t storedLen = EEPROM.read(addr++);
            if (storedLen == 0xFF || storedLen == 0) break;
            if (storedLen > UID_MAX_LEN) {
                Serial.println("Invalid stored UID length detected, aborting removal.");
                break;
            }

            bool match = (storedLen == len);
            for (int i = 0; i < len && match; i++) {
                if (EEPROM.read(addr + i) != uid[i]) {
                    match = false;
                }
            }

            if (match) {
                uint16_t nextAddr = addr + storedLen;

                while (nextAddr < USER_START + USER_SIZE) {
                    EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
                }

                while (uidStart < USER_START + USER_SIZE) {
                    EEPROM.write(uidStart++, 0xFF);
                }

                EEPROM.commit();
                Serial.print("UID removed: ");
                printUID(uid, len);
                return true;
            }

            addr += storedLen;
        }

        return false;
    }

private:
    uint16_t findFirstFreeAddress() {
        uint16_t addr = USER_START;

        while (addr < USER_START + USER_SIZE) {
            uint8_t len = EEPROM.read(addr);
            if (len == 0xFF || len == 0) break;
            if (len > UID_MAX_LEN) {
                Serial.println("Invalid UID length detected in findFirstFreeAddress, stopping.");
                break;
            }
            addr += (1 + len);
        }
        return addr;
    }
};