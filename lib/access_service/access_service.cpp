#include "access_service.h"

#include <Arduino.h>
#include "esp_log.h"


#include <led_controller.h>
#include "led_states.hpp"

#include "globals.hpp"
#include "config.hpp"
#include "pin_mapping.hpp"


// #include "user_manager.hpp"
#include <master_uid_manager.h>

// static UserManager userManager;

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

unsigned long relayActivatedTime = 0;

bool relayActive = false; // see if we can remove

static unsigned long userProgLastActivityTime = 0;
static bool userProgWarningGiven = false;

static bool userProgrammingModeActive = false;

const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS] = {1,  3,  4,  5,  8,  12, 17,
                                                         23, 30, 38, 47, 57, 68};


void accessServiceSetup() {

    led.begin(); 
    led.setPattern(PATTERN_BREATHING, 10000);

    rfid.begin();

    rfid.printFirmwareVersion();

    relays.begin();

    if (audio.begin()) {
        audio.setVolume(20);
        delay(500);
        audio.playTrack(AudioContoller::SOUND_STARTUP);
    }

    ESP_LOGI(TAG, "Waiting for an ISO14443A card");
}

void handleRelaySequence() {
    switch (currentRelayState) {
        case RELAY_IDLE:
            break;
        case RELAY1_ACTIVE:
            if (millis() - relayActivatedTime >= RELAY1_DURATION) {
                relays.setRelay(RELAY_1, false);
                relays.setRelay(RELAY_2, true);
                relayActivatedTime = millis();
                currentRelayState = RELAY2_ACTIVE;
                ESP_LOGI(TAG, "Relay state transition 1->2");
            }
            break;
        case RELAY2_ACTIVE:
            if (millis() - relayActivatedTime >= RELAY2_DURATION) {
                relays.setRelay(RELAY_2, false);
                currentRelayState = RELAY_IDLE;
                relayActive = false;
                ESP_LOGI(TAG, "Relay sequence complete");
            }
            break;
        default:
            ESP_LOGE(TAG, "Unhandled relay state: %d", currentRelayState);
            break;
    }
}

void activateRelays() {
    relays.setRelay(RELAY_1, true);
    relayActivatedTime = millis();
    currentRelayState = RELAY1_ACTIVE;
    relayActive = true;
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


    static bool masterProgrammingMode = !masterUidManager.hasMasterUIDs; //3

    if (!masterProgrammingMode) return false;

    if (rfid.readCard(uid, &uidLength)) {

        char uidStr[32] = {0};
        for (uint8_t i = 0, pos = 0; i < uidLength && pos + 3 < sizeof(uidStr); i++) {
            if (i > 0) uidStr[pos++] = ':';
            pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", uid[i]);
        }

        ESP_LOGI(TAG, "Programming master card detected, UID: %s", uidStr);

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
        ESP_LOGI(TAG, "Master UID Stored, Exiting Master Programming.");

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
            ESP_LOGV(TAG, "Master card detected - ignore during user programming mode");
            
            return true;
        }

        // masterUidManager.printUID(uid, uidLength, "Programming user card detected, UID:");
        
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
    if (!userProgWarningGiven && (now - userProgLastActivityTime > WARNING_TIMEOUT)) {
        userProgWarningGiven = true;

        ESP_LOGW(TAG, "Programming Mode Idle...Will Time Out Soon");
        
        // PLAY_SOUND(PROGRAMMING_TIMEOUT_WARNING);
        
        // LED_SET_SEQ(PROGRAMMING_WARNING);
        LED_SET_SEQ(PLACEHOLDER);
    }

    // Exit stage
    if (now - userProgLastActivityTime > EXIT_TIMEOUT) {
        ESP_LOGW(TAG, "Exiting User Programming Mode");

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
    constexpr int uidLenMax = 7;
    static bool masterPresent = false;
    static unsigned long masterStartTime = 0;
    static uint8_t lastMasterUID[uidLenMax] = {0};
    static uint8_t lastMasterUIDLen = 0;
    static unsigned long masterLastSeen = 0;

    unsigned long now = millis();

    bool isSameCard = uidMatches(uid, uidLength, lastMasterUID, lastMasterUIDLen);

    // Check if card is newly presented
    if (!masterPresent || !isSameCard || (now - masterLastSeen > 500)) {
        memcpy(lastMasterUID, uid, uidLength);
        lastMasterUIDLen = uidLength;
        masterStartTime = now;
        masterPresent = true;
        ESP_LOGI(TAG, "Master card detected - hold started");
    }

    masterLastSeen = now;

    // Update the shared state
    state.masterPresent = masterPresent;
    state.masterLastSeen = masterLastSeen;
    state.masterStartTime = masterStartTime;

    // Continuous hold logic
    if (masterPresent && (now - masterStartTime >= MASTER_HOLD_TIME)) {
        ESP_LOGI(TAG, "Master hold confirmed (%us)", (MASTER_HOLD_TIME / 1000));

        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(PROGRAMMING_MODE);
            state.queuedSound = AudioContoller::SOUND_ACCEPTED;
            state.audioQueued = true;
            userProgrammingModeActive = true;
            userProgLastActivityTime = millis();
            userProgWarningGiven = false;
            ESP_LOGI(TAG, "User Programming Mode Enabled");
        }
    }

    // **Do NOT return here**; let removal be handled elsewhere
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

// --- Impatience Timeout ---
void handleImpatienceTimer(AccessLoopState &state) {
    if (state.impatientEnabled && (millis() - state.startTime > IMPATIENCE_TIMEOUT) && !state.impatient) {
        ESP_LOGW(TAG, "%us elapsed, playing waiting sound", (IMPATIENCE_TIMEOUT/1000));
        state.queuedSound = AudioContoller::SOUND_WAITING;
        state.audioQueued = true;
        state.impatient = true;
    }
}
