#pragma once
#include <Arduino.h>

#define MAX_USERS 10
#define UID_LEN 7  // max UID length

class UserManager {
public:
    UserManager() : count(0) {}

    // Check if UID is in the list
    bool hasUID(uint8_t* uid, uint8_t uidLength) {
        for (int i = 0; i < count; i++) {
            if (compareUID(uid, uidLength, users[i], lengths[i])) {
                return true;
            }
        }
        return false;
    }

    // Add a new UID
    bool addUID(uint8_t* uid, uint8_t uidLength) {
        if (count >= MAX_USERS) return false;
        memcpy(users[count], uid, uidLength);
        lengths[count] = uidLength;
        count++;
        return true;
    }

    // Remove a UID
    bool removeUID(uint8_t* uid, uint8_t uidLength) {
        for (int i = 0; i < count; i++) {
            if (compareUID(uid, uidLength, users[i], lengths[i])) {
                // shift remaining UIDs down
                for (int j = i; j < count - 1; j++) {
                    memcpy(users[j], users[j + 1], lengths[j + 1]);
                    lengths[j] = lengths[j + 1];
                }
                count--;
                return true;
            }
        }
        return false;
    }

    int getCount() const { return count; }

private:
    uint8_t users[MAX_USERS][UID_LEN] = {0};
    uint8_t lengths[MAX_USERS] = {0};
    int count;

    // Simple UID comparison helper
    bool compareUID(uint8_t* a, uint8_t lenA, uint8_t* b, uint8_t lenB) {
        if (lenA != lenB) return false;
        for (int i = 0; i < lenA; i++) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }
};