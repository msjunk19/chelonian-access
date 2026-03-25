#include <access_service.h>
#include <Arduino.h>
#include <esp_log.h>
#include <led_states.hpp>
#include <globals.hpp>
#include <pin_mapping.hpp>
#include <master_uid_manager.h>


static const char* TAG = "ACCESS";  // Add TAG definition

// Instantiate controllers

// LEDController led(PN_LED); //Single Color LED on pin 8
LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10

RFIDController rfid;
RelayController relays;
AudioContoller audio;


const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS] = {1,  3,  4,  5,  8,  12, 17,
                                                         23, 30, 38, 47, 57, 68};

static AccessLoopState state;

void accessServiceSetup() {
    led.begin();
    ESP_LOGI(TAG, "LED begin done");
    // Test a basic flash to confirm LED hardware works immediately
    LED_SET_SEQ(SYSTEM_READY);
    ESP_LOGI(TAG, "Test flash sequence triggered");


    rfid.begin();
    rfid.printFirmwareVersion();

    relays.begin();

    if (audio.begin()) {
    audio.setVolume(AUDIO_DEFAULT_VOLUME);

    // wait up to 500ms for the controller to be ready, non-blocking-ish
    unsigned long start = millis();
    while (millis() - start < 500) {
        // tiny sleep to yield CPU to other tasks
        delay(1);  
    }

    audio.playTrack(AudioContoller::SOUND_STARTUP);
}

    ESP_LOGI(TAG, "Waiting for an ISO14443A card");
    markUserActivity(state);

}

void handleRelaySequence() {
    switch (state.currentRelayState) {
        case RELAY_IDLE:
            break;
        case RELAY1_ACTIVE:
            if (millis() - state.relayActivatedTime >= RELAY1_DURATION) {
                relays.setRelay(RELAY_1, false);
                relays.setRelay(RELAY_2, true);
                state.relayActivatedTime = millis();
                state.currentRelayState = RELAY2_ACTIVE;
                ESP_LOGI(TAG, "Relay state transition 1->2");
            }
            break;
        case RELAY2_ACTIVE:
            if (millis() - state.relayActivatedTime >= RELAY2_DURATION) {
                relays.setRelay(RELAY_2, false);
                state.currentRelayState = RELAY_IDLE;
                state.relayActive = false;
                ESP_LOGI(TAG, "Relay sequence complete");
            }
            break;
        default:
            ESP_LOGE(TAG, "Unhandled relay state: %d", state.currentRelayState);
            break;
    }
}

void activateRelays() {
    relays.setRelay(RELAY_1, true);
    state.relayActivatedTime = millis();
    state.currentRelayState = RELAY1_ACTIVE;
    state.relayActive = true;
    ESP_LOGI(TAG, "Starting relay sequence (state 1)");
}

static void updateHardware() {
    led.update();
    handleRelaySequence();
}

static bool handleBootProgrammingCheck() {
    static bool bootChecked = false;
    static bool bootMasterProgrammingMode = false; //2

    if (!bootChecked) {
        if (!masterUidManager.hasMasterUIDs) {
            ESP_LOGW(TAG, "No Master UID Stored. Enter Master Programming Mode.");
            bootMasterProgrammingMode = true;
            LED_SET_SEQ(PROGRAMMING_MODE);
        }
        bootChecked = true;
    }
    return false; // does not early-exit loop
}


static bool handleMasterProgrammingMode(uint8_t* uid, uint8_t& uidLength) {
    static bool masterProgrammingMode = !masterUidManager.hasMasterUIDs;
    static bool waitingForRemoval = false;

    static uint8_t storedUID[7];
    static uint8_t storedLength = 0;

    if (!masterProgrammingMode) return false;

    bool cardDetected = rfid.readCard(uid, &uidLength);

    // 🟢 STEP 1: Detect card ONCE
    if (!waitingForRemoval) {
        if (cardDetected) {

            storedLength = uidLength > sizeof(storedUID) ? sizeof(storedUID) : uidLength;
            memcpy(storedUID, uid, storedLength);

            char uidStr[32] = {0};
            for (uint8_t i = 0, pos = 0; i < storedLength && pos + 3 < sizeof(uidStr); i++) {
                if (i > 0) uidStr[pos++] = ':';
                pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", storedUID[i]);
            }

            ESP_LOGI(TAG, "Detected New Master Card: UID: %s", uidStr);

            waitingForRemoval = true;
            LED_SET_SEQ(MASTER_CARD_SET);
        }
    }
    // 🔴 STEP 2: Wait for removal (simple debounce)
    else {
        if (!cardDetected) {

            uint8_t* uids[] = {storedUID};
            uint8_t lengths[] = {storedLength};

            masterUidManager.writeUIDs(uids, lengths, 1);

            masterUidManager.hasMasterUIDs = true;
            masterProgrammingMode = false;
            waitingForRemoval = false;

            ESP_LOGI(TAG, "Master UID Stored, Exiting Master Programming.");
        }
    }

    return true;
}


// static bool handleUserProgrammingMode(uint8_t* uid, uint8_t uidLength) {
//     if (!state.userProgrammingModeActive) return false;

//     unsigned long now = millis();

//     // state.impatientEnabled = false;

//     bool cardDetected = rfid.readCard(uid, &uidLength);

//     if (cardDetected) {
//         state.userProgLastActivityTime = now;
//         state.userProgWarningGiven = false;
//         // state.impatient = false;

//         if (masterUidManager.checkUID(uid, uidLength)) {
//             ESP_LOGV(TAG, "Master card detected - ignore during user programming mode");
            
//             return true;
//         }

//         while (rfid.readCard(uid, &uidLength)) {
//             delay(50);
//         }

//         if (!userUidManager.checkUID(uid, uidLength)) {
//             userUidManager.addUID(uid, uidLength);
//             LED_SET_SEQ(USER_ADDED);
//         } else {
//             userUidManager.removeUID(uid, uidLength);
//             LED_SET_SEQ(USER_REMOVED);
//         }

//         return true;
//     }

//     // Warning stage
//     if (!state.userProgWarningGiven && (now - state.userProgLastActivityTime > WARNING_TIMEOUT)) {
//         state.userProgWarningGiven = true;

//         ESP_LOGW(TAG, "Programming Mode Idle...Will Time Out Soon");
        
//         // PLAY_SOUND(PROGRAMMING_TIMEOUT_WARNING);
        
//         // LED_SET_SEQ(PROGRAMMING_WARNING);
//         LED_SET_SEQ(PLACEHOLDER);
//     }

//     // Exit stage
//     if (now - state.userProgLastActivityTime > EXIT_TIMEOUT) {
//         ESP_LOGW(TAG, "Exiting User Programming Mode");
//         state.userProgrammingModeActive = false;

//         // PLAY_SOUND(PROGRAMMING_EXIT);
//         // LED_SET_SEQ(IDLE);
//         LED_SET_SEQ(PLACEHOLDER);
//         return false;
//     }
//     return true;
// }
static bool handleUserProgrammingMode(uint8_t* uid, uint8_t uidLength) {
    if (!state.userProgrammingModeActive) return false;

    // Disable impatient timer while in programming mode
    state.impatientEnabled = false;  // stops the normal impatience loop
    state.impatient = false;         // reset any current impatience state

    unsigned long now = millis();
    bool cardDetected = rfid.readCard(uid, &uidLength);

    if (cardDetected) {
        state.userProgLastActivityTime = now;
        state.userProgWarningGiven = false;

        if (masterUidManager.checkUID(uid, uidLength)) {
            ESP_LOGV(TAG, "Master card detected - ignore during user programming mode");
            return true;
        }

        // Ensure the card is fully removed
        while (rfid.readCard(uid, &uidLength)) {
            delay(50);
        }

        if (!userUidManager.checkUID(uid, uidLength)) {
            userUidManager.addUID(uid, uidLength);
            LED_SET_SEQ(USER_ADDED);
        } else {
            userUidManager.removeUID(uid, uidLength);
            LED_SET_SEQ(USER_REMOVED);
        }

        return true;
    }

    // Warning stage
    if (!state.userProgWarningGiven && (now - state.userProgLastActivityTime > WARNING_TIMEOUT)) {
        state.userProgWarningGiven = true;
        ESP_LOGW(TAG, "Programming Mode Idle...Will Time Out Soon");
        LED_SET_SEQ(PLACEHOLDER);
    }

    // Exit stage
    if (now - state.userProgLastActivityTime > EXIT_TIMEOUT) {
        ESP_LOGW(TAG, "Exiting User Programming Mode");
        state.userProgrammingModeActive = false;

        // Re-enable impatience timer for normal operation
        state.impatientEnabled = true;

        LED_SET_SEQ(PLACEHOLDER);
        return false;
    }

    return true;
}


static bool uidMatches(const uint8_t* a, uint8_t lenA, const uint8_t* b, uint8_t lenB) {
    if (lenA != lenB) return false;
    return memcmp(a, b, lenA) == 0;
}

static bool handleMasterTimeout() {
    return handleMasterPresenceTimeout(state);
}

static void handleCardDetected(uint8_t* uid, uint8_t uidLength) {
    state.scanned = true;

    if (!validateUIDLength(uidLength)) {
        state.audioQueued = false;
        return;
    }

    // Handle master card first
    if (masterUidManager.checkUID(uid, uidLength)) {
        handleMasterCard(uid, uidLength, state);
        return;  // skip regular card if master
    }

    // Regular card processing
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
        ESP_LOGI(TAG, "Master card removed - resetting hold");
        state.masterPresent = false;
        state.masterStartTime = 0;
        state.lastMasterUIDLen = 0;
        markUserActivity(state);
        return true;
    }
    return false;
}

bool validateUIDLength(uint8_t uidLength) {
    switch (uidLength) {
        case 4:
            ESP_LOGI(TAG, "4B UID detected");
            break;
        case 7:
            ESP_LOGI(TAG, "7B UID detected");
            break;
        default:
            ESP_LOGE(TAG, "Unknown UID type/length");
            if (!led.isRunning()) LED_SET_SEQ(UNKNOWN_UID_TYPE);
            return false;
    }
    return true;
}

void handleMasterCard(uint8_t* uid, uint8_t uidLength, AccessLoopState &state) {
    unsigned long now = millis();

    bool isSameCard = uidMatches(uid, uidLength, state.lastMasterUID, state.lastMasterUIDLen);

    // New card or re-presented card
    if (!state.masterPresent || !isSameCard || (now - state.masterLastSeen > 500)) {
        memcpy(state.lastMasterUID, uid, uidLength);
        state.lastMasterUIDLen = uidLength;
        state.masterStartTime = now;
        state.masterPresent = true;

        ESP_LOGI(TAG, "Master card detected - hold started");
    }

    state.masterLastSeen = now;

    // Hold detection
    if (state.masterPresent && (now - state.masterStartTime >= MASTER_HOLD_TIME)) {
        ESP_LOGI(TAG, "Master hold confirmed (%us)", (MASTER_HOLD_TIME / 1000));

        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(PROGRAMMING_MODE);
            state.queuedSound = AudioContoller::SOUND_ACCEPTED;
            state.audioQueued = true;

            state.userProgrammingModeActive = true;
            state.userProgLastActivityTime = millis();
            state.userProgWarningGiven = false;

            ESP_LOGI(TAG, "User Programming Mode Enabled");
        }
    }
}


// --- User Card Handler ---
void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
    bool validUID = rfid.validateUID(uid, uidLength);

    if (validUID) {
        ESP_LOGI(TAG, "Valid Card, Access Granted");
        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(ACCESS_GRANTED);
            state.queuedSound = AudioContoller::SOUND_ACCEPTED;
            state.audioQueued = true;
            activateRelays();
            state.invalidAttempts = 0;
        }
    } else {
        uint32_t delayMs = (invalidDelays[state.invalidAttempts] * 1000) + 3000;
        state.invalidTimeoutEnd = millis() + delayMs;

        ESP_LOGW(TAG, "Invalid card attempt #%u, please wait %u seconds before trying again",
            state.invalidAttempts + 1, delayMs / 1000);

        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(ACCESS_DENIED);
            state.queuedSound = (state.invalidAttempts == 0) ? AudioContoller::SOUND_DENIED_1 :
                                 (state.invalidAttempts == 1) ? AudioContoller::SOUND_DENIED_2 :
                                                           AudioContoller::SOUND_DENIED_3;
            state.audioQueued = true;
            if (state.invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) state.invalidAttempts++;

            markUserActivity(state);
        }
    }
}


// --- Impatience Timeout ---
void handleImpatienceTimer(AccessLoopState &state) {
if (state.impatientEnabled &&
    !state.impatient &&
    millis() - state.lastActivityTime > IMPATIENCE_TIMEOUT)
{
    state.impatient = true;

    ESP_LOGW(TAG, " %ds elapsed, playing waiting sound", IMPATIENCE_TIMEOUT / 1000);
    audio.playTrack(AudioContoller::SOUND_WAITING);
}
}

void markUserActivity(AccessLoopState& state)
{
    state.lastActivityTime = millis();
    state.impatient = false;
    ESP_LOGI(TAG, "No User Interaction...Timer Starting...");
}
