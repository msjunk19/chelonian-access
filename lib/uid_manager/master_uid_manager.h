#pragma once
#include <EEPROM.h>
#include <config.hpp>

static constexpr uint8_t UID_MAX_LEN = 7; // maximum UID length allowed

class MasterUIDManager {
public:
    bool hasMasterUIDs = false;

    // Clear only the master region
    void clearAll() {
        for (uint16_t addr = MASTER_START; addr < MASTER_START + MASTER_SIZE; addr++) {
            EEPROM.write(addr, 0xFF);
        }
        EEPROM.commit();
        Serial.println("MASTER EEPROM CLEARED");
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

    // Write multiple UIDs to the master region
    void writeUIDs(uint8_t** uids, uint8_t* lengths, size_t count) {
        uint16_t addr = findFirstFreeAddress();

        for (size_t i = 0; i < count; i++) {
            uint8_t len = lengths[i];

            // Validate length
            if (len == 0 || len > UID_MAX_LEN) {
                Serial.print("Skipping invalid UID length: ");
                Serial.println(len);
                continue;
            }

            // Check for space
            if (addr + 1 + len > MASTER_START + MASTER_SIZE) {
                Serial.println("Not enough space to append UID!");
                break;
            }

            // Skip if UID already exists
            if (checkUID(uids[i], len)) {
                Serial.print("UID already exists, skipping: ");
                printUID(uids[i], len);
                continue;
            }

            // Write length + bytes
            EEPROM.write(addr++, len);
            for (int j = 0; j < len; j++) {
                EEPROM.write(addr++, uids[i][j]);
            }
        }

        EEPROM.commit();
        Serial.println("UIDs written/appended to EEPROM!");
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

        hasMasterUIDs = (index > 0);
        if (!hasMasterUIDs) Serial.println("No master UIDs stored in EEPROM!");
        return hasMasterUIDs;
    }

    // Check if a UID exists
    bool checkUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;

        uint16_t addr = MASTER_START;
        while (addr < MASTER_START + MASTER_SIZE) {
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

            addr += (storedLen > 0 ? storedLen : 1); // always advance at least 1
        }

        return false;
    }

    // Remove a UID
    bool removeUID(uint8_t* uid, uint8_t len) {
        if (len == 0 || len > UID_MAX_LEN) return false;

        uint16_t addr = MASTER_START;
        while (addr < MASTER_START + MASTER_SIZE) {
            uint16_t uidStart = addr;
            uint8_t storedLen = EEPROM.read(addr++);
            if (storedLen == 0xFF || storedLen == 0) break;
            if (storedLen > UID_MAX_LEN) {
                Serial.println("Invalid stored UID length detected, aborting removal.");
                break;
            }

            bool match = (storedLen == len);
            for (int i = 0; i < len && match; i++) {
                if (EEPROM.read(addr + i) != uid[i]) match = false;
            }

            if (match) {
                uint16_t nextAddr = addr + storedLen;
                // Shift remaining UIDs down
                while (nextAddr < MASTER_START + MASTER_SIZE) {
                    EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
                }
                // Fill remaining bytes with 0xFF
                while (uidStart < MASTER_START + MASTER_SIZE) {
                    EEPROM.write(uidStart++, 0xFF);
                }
                EEPROM.commit();
                Serial.print("UID removed: ");
                printUID(uid, len);
                return true;
            }

            addr += (storedLen > 0 ? storedLen : 1);
        }

        Serial.println("UID not found to remove.");
        return false;
    }

private:
    // Find first free address in master region
    uint16_t findFirstFreeAddress() {
        uint16_t addr = MASTER_START;
        while (addr < MASTER_START + MASTER_SIZE) {
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

// #pragma once
// #include <EEPROM.h>
// #include <config.hpp>


// class MasterUIDManager {
// public:
//     bool hasMasterUIDs = false;

//     void clearAll() {
//     // for (uint16_t addr = 0; addr < EEPROM_SIZE; addr++) {
//     for (uint16_t addr = MASTER_START; addr < MASTER_START + MASTER_SIZE; addr++) { // Use reserveed area for Master UIDs
//         EEPROM.write(addr, 0xFF);  // mark all bytes as empty
//     }
//     EEPROM.commit();
//     Serial.println("EEPROM cleared.");
//     }

//     void printUID(uint8_t* uid, uint8_t len) {
//     char uidStr[50] = "";

//     for (uint8_t i = 0; i < len; i++) {
//         char hexBuf[4];
//         sprintf(hexBuf, "%02X ", uid[i]);  // space instead of colon
//         strcat(uidStr, hexBuf);
//     }

//     Serial.println(uidStr);
// }
    
//     void writeUIDs(uint8_t** uids, uint8_t* lengths, size_t count) {
//         uint16_t addr = findFirstFreeAddress();

//         for (size_t i = 0; i < count; i++) {
//             uint8_t len = lengths[i];

//             // if (addr + 1 + len > EEPROM_SIZE) {
//             if (addr + 1 + len > MASTER_START + MASTER_SIZE) { // Adjust for master eeprom location
//                 Serial.println("Not enough EEPROM space to append all UIDs!");
//                 break;
//             }

//             if (checkUID(uids[i], len)) {
//                 Serial.print("UID already exists, skipping: ");
//                 printUID(uids[i], len);
//                 continue;
//             }

//             EEPROM.write(addr++, len);
//             for (int j = 0; j < len; j++) {
//                 EEPROM.write(addr++, uids[i][j]);
//             }
//         }

//         EEPROM.commit();
//         Serial.println("UIDs written/appended to EEPROM!");
//     }


// bool readUIDs() {
//     // uint16_t addr = 0;
//     uint16_t addr = MASTER_START;
//     int index = 0;

//     // while (addr < EEPROM_SIZE) {
//     Serial.printf("Scanning EEPROM: %u → %u\n", MASTER_START, MASTER_START + MASTER_SIZE);
//     while (addr < MASTER_START + MASTER_SIZE) { // master location
//         uint8_t len = EEPROM.read(addr++);
//         // if (len == 0xFF || len == 0) break;
//         if (len == 0xFF || len == 0) break;
//         if (len > 7) {
//             Serial.println("Invalid UID length detected, aborting scan.");
//             break;
// }

//         Serial.print("UID #");
//         Serial.print(index++);
//         Serial.print(" (length ");
//         Serial.print(len);
//         Serial.print("): ");

//         uint8_t uid[7];

//         for (int i = 0; i < len; i++) {
//             uid[i] = EEPROM.read(addr++);
//         }

//         printUID(uid, len);
//     }

//     if (index == 0) {
//         Serial.println("No master UIDs stored in EEPROM!");
//         hasMasterUIDs = false;
//         return false;
//     }

//     hasMasterUIDs = true;
//     return true;
// }


//     bool checkUID(uint8_t* uid, uint8_t len) {
//         // uint16_t addr = 0;
//         uint16_t addr = MASTER_START;


//         // while (addr < EEPROM_SIZE) {
//         while (addr < MASTER_START + MASTER_SIZE) {
//             uint8_t storedLen = EEPROM.read(addr++);
//             // if (storedLen == 0xFF || storedLen == 0) break;
//             if (storedLen == 0xFF || storedLen == 0) break;
//             if (storedLen > 7) {
//                 Serial.println("Invalid Stored UID length detected, aborting scan.");
//                 break;
//             }

//             if (storedLen == len) {
//                 bool match = true;
//                 for (int i = 0; i < len; i++) {
//                     if (EEPROM.read(addr + i) != uid[i]) {
//                         match = false;
//                         break;
//                     }
//                 }
//                 if (match) return true;
//             }
//             addr += storedLen;
//         }

//         return false;
//     }

//     // Remove a UID from EEPROM
//     bool removeUID(uint8_t* uid, uint8_t len) {
//     // uint16_t addr = 0;
//     uint16_t addr = MASTER_START;

//     // while (addr < EEPROM_SIZE) {
//      while (addr < MASTER_START + MASTER_SIZE) { // master eeprom location
//         uint16_t uidStart = addr;          // Start of length byte
//         uint8_t storedLen = EEPROM.read(addr++);
//         // if (storedLen == 0xFF || storedLen == 0) break;
//         if (storedLen == 0xFF || storedLen == 0) break;
//         if (storedLen > 7) {
//             Serial.println("Invalid Stored UID length detected, aborting scan.");
//             break;
//         }

//         // Compare bytes
//         bool match = (storedLen == len);
//         for (int i = 0; i < len && match; i++) {
//             if (EEPROM.read(addr + i) != uid[i]) {
//                 match = false;
//             }
//         }

//         if (match) {
//             uint16_t nextAddr = addr + storedLen; // Start of next UID
//             // Shift all remaining bytes left
//             // while (nextAddr < EEPROM_SIZE) {
//             while (nextAddr < MASTER_START + MASTER_SIZE) { //  master eeprom location
//                 EEPROM.write(uidStart++, EEPROM.read(nextAddr++));
//             }
//             // Fill the remaining space with 0xFF
//             // while (uidStart < EEPROM_SIZE) {
//             while (uidStart < MASTER_START + MASTER_SIZE) { // master location
//                 EEPROM.write(uidStart++, 0xFF);
//             }

//             EEPROM.commit();
//             Serial.print("UID removed: ");
//             printUID(uid, len);
//             return true;
//         }

//         addr += storedLen; // Move to next UID
//     }

//     Serial.println("UID not found to remove.");
//     return false;
// }

// private:
//     uint16_t findFirstFreeAddress() {
//         // uint16_t addr = 0;
//         uint16_t addr = MASTER_START; // master eeprom location

//         // while (addr < EEPROM_SIZE) {
//         while (addr < MASTER_START + MASTER_SIZE) { // master location
//             uint8_t len = EEPROM.read(addr);
//             if (len == 0xFF || len == 0) break;
//             addr += 1 + len;
//         }
//         return addr;
//     }


// };