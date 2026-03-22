#include "access_service.h"

#include <Arduino.h>
#include "esp_log.h"


#include <led_controller.h>
#include "led_states.hpp"

#include "globals.hpp"
#include "config.hpp"
#include "pin_mapping.hpp"


#include "user_manager.hpp"
#include <master_uid_manager.h>

static UserManager userManager;

// LEDController led(PN_LED); //Single Color LED on pin 8
LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10

static const char* TAG = "ACCESS";  // Add TAG definition
// Instantiate controllers
RFIDController rfid;
RelayController relays;
AudioContoller audio;

// State variables
enum RelayState currentRelayState = RELAY_IDLE;
uint8_t invalidAttempts = 0;
bool scanned = false;
// bool impatient = false;
unsigned long relayActivatedTime = 0;
bool relayActive = false;

static unsigned long userProgLastActivityTime = 0;
static bool userProgWarningGiven = false;

bool masterProgrammingMode = false;
static bool userProgrammingModeActive = false;

const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS] = {1,  3,  4,  5,  8,  12, 17,
                                                         23, 30, 38, 47, 57, 68};


void accessServiceSetup() {

    led.begin(); 
    led.setPattern(PATTERN_BREATHING, 10000);

    rfid.begin();

    rfid.printFirmwareVersion();
    rfid.initializeDefaultUIDs();

    relays.begin();

    if (audio.begin()) {
        audio.setVolume(20);
        delay(500);
        audio.playTrack(AudioContoller::SOUND_STARTUP);
    }

    if (masterUidManager.readUIDs()) {
    Serial.println("Master UID(s) detected. Normal operation.");
    } else {
    Serial.println("No master UIDs found. Enter Master programming mode.");
    }

    ESP_LOGE(TAG, "Waiting for an ISO14443A card");
}

void handleRelaySequence() {
    switch (currentRelayState) {
        case RELAY_IDLE:
            break;
        case RELAY1_ACTIVE:
            if (millis() - relayActivatedTime >= RELAY1_DURATION) {
                relays.setRelay(RELAY1_PIN, false);
                relays.setRelay(RELAY2_PIN, true);
                relayActivatedTime = millis();
                currentRelayState = RELAY2_ACTIVE;
                ESP_LOGE(TAG, "Relay state transition 1->2");
            }
            break;
        case RELAY2_ACTIVE:
            if (millis() - relayActivatedTime >= RELAY2_DURATION) {
                relays.setRelay(RELAY2_PIN, false);
                currentRelayState = RELAY_IDLE;
                relayActive = false;
                ESP_LOGE(TAG, "Relay sequence complete");
            }
            break;
        default:
            ESP_LOGE(TAG, "Unhandled relay state: %d", currentRelayState);
            break;
    }
}

void activateRelays() {
    relays.setRelay(RELAY1_PIN, true);
    relayActivatedTime = millis();
    currentRelayState = RELAY1_ACTIVE;
    relayActive = true;
    ESP_LOGE(TAG, "Starting relay sequence (state 1)");
}

static void updateHardware() {
    led.update();
    handleRelaySequence();
}

static bool handleBootProgrammingCheck() {
    static bool bootChecked = false;
    static bool masterProgrammingMode = false;

    if (!bootChecked) {
        if (!masterUidManager.hasMasterUIDs) {
            Serial.println("No master UID stored. Enter programming mode.");
            masterProgrammingMode = true;
            LED_SET_SEQ(PROGRAMMING_MODE);
        }
        bootChecked = true;
    }

    return false; // does not early-exit loop
}

static bool handleMasterProgrammingMode(uint8_t* uid, uint8_t& uidLength) {
    static bool masterProgrammingMode = !masterUidManager.hasMasterUIDs;

    if (!masterProgrammingMode) return false;

    if (rfid.readCard(uid, &uidLength)) {
        Serial.print("Programming master card detected, UID: ");
        masterUidManager.printUID(uid, uidLength);

        // Wait for removal (debounce)
        while (rfid.readCard(uid, &uidLength)) {
            delay(50);
        }

        uint8_t* uids[] = {uid};
        uint8_t lengths[] = {uidLength};
        masterUidManager.writeUIDs(uids, lengths, 1);

        masterUidManager.hasMasterUIDs = true;
        masterProgrammingMode = false;

        LED_SET_SEQ(MASTER_CARD_SET);
        Serial.println("Master UID programmed, exiting programming mode.");
    }

    return true; // skip rest of loop while programming
}


static bool handleUserProgrammingMode(uint8_t* uid, uint8_t uidLength) {
    if (!userProgrammingModeActive) return false;

    unsigned long now = millis();

    bool cardDetected = rfid.readCard(uid, &uidLength);

    if (cardDetected) {
        userProgLastActivityTime = now;
        userProgWarningGiven = false;

        if (masterUidManager.checkUID(uid, uidLength)) {
            Serial.println("Master card detected - ignore during user programming mode");
            return true;
        }

        Serial.print("Programming user card detected, UID: ");
        masterUidManager.printUID(uid, uidLength);

        while (rfid.readCard(uid, &uidLength)) {
            delay(50);
        }

        if (!userUidManager.checkUID(uid, uidLength)) {
            userUidManager.addUID(uid, uidLength);
            Serial.println("User card added");
            LED_SET_SEQ(USER_ADDED);
        } else {
            userUidManager.removeUID(uid, uidLength);
            Serial.println("User card removed");
            LED_SET_SEQ(USER_REMOVED);
        }

        return true;
    }

    // Warning stage
    if (!userProgWarningGiven && (now - userProgLastActivityTime > WARNING_TIMEOUT)) {
        userProgWarningGiven = true;

        Serial.println("Programming mode idle - warning");
        // PLAY_SOUND(PROGRAMMING_TIMEOUT_WARNING);
        
        // LED_SET_SEQ(PROGRAMMING_WARNING);
        LED_SET_SEQ(PLACEHOLDER);
    }

    // Exit stage
    if (now - userProgLastActivityTime > EXIT_TIMEOUT) {
        Serial.println("Exiting user programming mode");

        userProgrammingModeActive = false;

        // PLAY_SOUND(PROGRAMMING_EXIT);
        // LED_SET_SEQ(IDLE);
        LED_SET_SEQ(PLACEHOLDER);


        return false;
    }

    return true;
}

static AccessLoopState state;

static bool uidMatches(const uint8_t* a, uint8_t lenA, const uint8_t* b, uint8_t lenB) {
    if (lenA != lenB) return false;
    return memcmp(a, b, lenA) == 0;
}

static bool handleMasterTimeout() {
    return handleMasterPresenceTimeout(state);
}

static void handleCardDetected(uint8_t* uid, uint8_t uidLength) {
    state.scanned = true;
    state.startTime = millis();
    state.impatient = false;

    if (!validateUIDLength(uidLength)) {
        state.audioQueued = false;
        return;
    }

    constexpr int uidLenMax = 7;  // replace UID_LEN with actual length used
    static bool masterPresent = false;
    static unsigned long masterStartTime = 0;
    static uint8_t lastMasterUID[uidLenMax] = {0};
    static uint8_t lastMasterUIDLen = 0;
    static unsigned long masterLastSeen = 0;

    unsigned long now = millis();

    // Master card detection
    // handleMasterCard(uid, uidLength, state);
    if (masterUidManager.checkUID(uid, uidLength)) {
        // New card or re-presented after removal
        if (!masterPresent || !uidMatches(uid, uidLength, lastMasterUID, lastMasterUIDLen) ||
            (now - masterLastSeen > 500)) {
            memcpy(lastMasterUID, uid, uidLength);
            lastMasterUIDLen = uidLength;
            masterStartTime = now;       // reset hold timer
            masterPresent = true;
            ESP_LOGE(TAG, "Master card detected - hold started");
        }

        masterLastSeen = now;  // update last seen time

        // Continuous hold for 2s
        if (masterPresent && (now - masterStartTime >= MASTER_HOLD_TIME)) {
            ESP_LOGE(TAG, "Master hold confirmed (%us)", (MASTER_HOLD_TIME/1000));

            if (!led.isRunning() && !state.audioQueued) {
                LED_SET_SEQ(PROGRAMMING_MODE);
                state.queuedSound = AudioContoller::SOUND_ACCEPTED;
                state.audioQueued = true;

                // Trigger user programming mode
                userProgrammingModeActive = true;

                userProgLastActivityTime = millis();
                userProgWarningGiven = false;

                Serial.println("User Programming Mode Enabled");
            }

            if (masterPresent && (now - masterLastSeen > 300)) {
                ESP_LOGE(TAG, "Master card not detected - hold reset");
                masterPresent = false;
                masterStartTime = 0;
            }

            return;
        }

        return;  // still holding, skip regular card processing
    }

    // --- REGULAR CARD ---
    handleRegularCard(uid, uidLength, state);
}


static void handleNoCard() {
    state.scanned = false;
    handleImpatienceTimer(state);
}

static void processCardScan(uint8_t* uid, uint8_t& uidLength) {
    bool lockedOut = millis() < state.invalidTimeoutEnd;
    bool cardDetected = !lockedOut && rfid.readCard(uid, &uidLength);

    if (cardDetected) {
        handleCardDetected(uid, uidLength);
    } else {
        handleNoCard();
    }
}


static void handleAudioPlayback() {
    if (state.audioQueued && !led.isRunning()) {
        audio.playTrack(state.queuedSound);
        state.audioQueued = false;
    }
}




void accessServiceLoop() {
    updateHardware();

    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    if (handleBootProgrammingCheck()) return;
    if (handleMasterProgrammingMode(uid, uidLength)) return;
    if (handleMasterTimeout()) return;

    if (handleUserProgrammingMode(uid, uidLength)) return; // <-- user programming mode

    processCardScan(uid, uidLength);
    handleAudioPlayback();
}


// --- Helper functions using AccessLoopState ---

bool handleMasterPresenceTimeout(AccessLoopState &state) {
    if (state.masterPresent && (millis() - state.masterLastSeen > 300)) {
        ESP_LOGE(TAG, "Master card removed - resetting hold");
        state.masterPresent = false;
        state.masterStartTime = 0;
        state.lastMasterUIDLen = 0;
        return true;
    }
    return false;
}

bool validateUIDLength(uint8_t uidLength) {
    switch (uidLength) {
        case 4:
            ESP_LOGE(TAG, "4B UID detected");
            break;
        case 7:
            ESP_LOGE(TAG, "7B UID detected");
            break;
        default:
            ESP_LOGE(TAG, "Unknown UID type/length");
            if (!led.isRunning()) LED_SET_SEQ(UNKNOWN_UID_TYPE);
            return false;
    }
    return true;
}

// void handleMasterCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
//     unsigned long now = millis();
//     state.masterLastSeen = now;

//     if (!state.masterPresent || !uidMatches(uid, uidLength, state.lastMasterUID, state.lastMasterUIDLen)) {
//         memcpy(state.lastMasterUID, uid, uidLength);
//         state.lastMasterUIDLen = uidLength;
//         state.masterStartTime = now;
//         state.masterPresent = true;
//         ESP_LOGE(TAG, "Master card detected - hold started");
//     }

//     if (state.masterPresent && (now - state.masterStartTime >= MASTER_HOLD_TIME)) {
//         ESP_LOGE(TAG, "Master hold confirmed (2s)");
//         if (!led.isRunning() && !state.audioQueued) {
//             LED_SET_SEQ(PROGRAMMING_MODE);
//             state.queuedSound = AudioContoller::SOUND_ACCEPTED;
//             state.audioQueued = true;
//         }
//         // Prevent retrigger spam
//         state.masterPresent = false;
//         state.masterStartTime = 0;
//     }
// }

void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
    bool validUID = rfid.validateUID(uid, uidLength);

    if (validUID) {
        ESP_LOGE(TAG, "Valid card");
        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(ACCESS_GRANTED);
            state.queuedSound = AudioContoller::SOUND_ACCEPTED;
            state.audioQueued = true;
            activateRelays();
            invalidAttempts = 0;
            state.impatientEnabled = false;
            state.impatient = false;
            state.startTime = millis();
        }
    } else {
        ESP_LOGW(TAG, "Invalid card attempt #%u", invalidAttempts + 1);
        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(ACCESS_DENIED);
            state.queuedSound = (invalidAttempts == 0) ? AudioContoller::SOUND_DENIED_1 :
                                 (invalidAttempts == 1) ? AudioContoller::SOUND_DENIED_2 :
                                                           AudioContoller::SOUND_DENIED_3;
            state.audioQueued = true;
            state.invalidTimeoutEnd = millis() + (invalidDelays[invalidAttempts] * 1000) + 3000;
            if (invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) invalidAttempts++;
            state.impatientEnabled = true;
            state.impatient = false;
            state.startTime = millis();
        }
    }
}

void handleImpatienceTimer(AccessLoopState &state) {
    if (state.impatientEnabled && (millis() - state.startTime > IMPATIENCE_TIMEOUT) && !state.impatient) {
        // ESP_LOGE(TAG, "10s elapsed, playing waiting sound");
        ESP_LOGE(TAG, "%us elapsed, playing waiting sound", (IMPATIENCE_TIMEOUT/1000));
        state.queuedSound = AudioContoller::SOUND_WAITING;
        state.audioQueued = true;
        state.impatient = true;
    }
}
