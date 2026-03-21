#include "access_service.h"

#include <Arduino.h>
#include "esp_log.h"


#include <led_controller.h>
#include "led_states.hpp"


// LEDController led(PN_LED); //Single Color LED on pin 8
LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10

// Master UID Variables
static const uint8_t MASTER_UID[] = {0x04, 0x54, 0x6B, 0x32, 0x0A, 0x54, 0x81}; // replace later
static const uint8_t MASTER_UID_LEN = 7;

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

const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS] = {1,  3,  4,  5,  8,  12, 17,
                                                         23, 30, 38, 47, 57, 68};

bool isMasterCard(uint8_t* uid, uint8_t uidLength) {
    if (uidLength != MASTER_UID_LEN) return false;

    for (int i = 0; i < uidLength; i++) {
        if (uid[i] != MASTER_UID[i]) return false;
    }
    return true;
}

bool compareUID(uint8_t* a, uint8_t lenA, uint8_t* b, uint8_t lenB) {
    if (lenA != lenB) return false;
    for (int i = 0; i < lenA; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}




void accessServiceSetup() {

    led.begin(); 
    led.setPattern(PATTERN_BREATHING, 10000);

    if (rfid.begin()) {
        ESP_LOGE(TAG, "RFID controller initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize RFID controller");
    }

    rfid.printFirmwareVersion();
    rfid.initializeDefaultUIDs();

    relays.begin();

    if (audio.begin()) {
        audio.setVolume(20);
        delay(500);
        audio.playTrack(AudioContoller::SOUND_STARTUP);
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

static AccessLoopState state;

void accessServiceLoop() {
    // --- Update hardware ---
    led.update();
    handleRelaySequence();

    // --- Prepare card read ---
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    // --- Master card presence timeout ---
    if (handleMasterPresenceTimeout(state)) {
        return; // Timeout handled, skip rest
    }

    // --- Check if system is locked out ---
    bool lockedOut = millis() < state.invalidTimeoutEnd;
    bool cardDetected = !lockedOut && rfid.readCard(uid, &uidLength);

    if (cardDetected) {
        // --- Card detected ---
        state.scanned = true;
        state.startTime = millis();
        state.impatient = false;

        // --- Validate UID length ---
        if (!validateUIDLength(uidLength)) {
            state.audioQueued = false;
            return;
        }

        // --- Master card ---
        if (isMasterCard(uid, uidLength)) {
            handleMasterCard(uid, uidLength, state);
            return;
        }

        // --- Regular card ---
        handleRegularCard(uid, uidLength, state);
    } else {
        // --- No card detected ---
        state.scanned = false;
        handleImpatienceTimer(state);
    }

    // --- Play queued audio if LED sequence finished ---
    if (state.audioQueued && !led.isRunning()) {
        audio.playTrack(state.queuedSound);
        state.audioQueued = false;
    }
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

void handleMasterCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
    unsigned long now = millis();
    state.masterLastSeen = now;

    if (!state.masterPresent || !compareUID(uid, uidLength, state.lastMasterUID, state.lastMasterUIDLen)) {
        memcpy(state.lastMasterUID, uid, uidLength);
        state.lastMasterUIDLen = uidLength;
        state.masterStartTime = now;
        state.masterPresent = true;
        ESP_LOGE(TAG, "Master card detected - hold started");
    }

    if (state.masterPresent && (now - state.masterStartTime >= 2000)) {
        ESP_LOGE(TAG, "Master hold confirmed (2s)");
        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(PROGRAMMING_MODE);
            state.queuedSound = AudioContoller::SOUND_ACCEPTED;
            state.audioQueued = true;
        }
        // Prevent retrigger spam
        state.masterPresent = false;
        state.masterStartTime = 0;
    }
}

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
    if (state.impatientEnabled && (millis() - state.startTime > 10000) && !state.impatient) {
        ESP_LOGE(TAG, "10s elapsed, playing waiting sound");
        state.queuedSound = AudioContoller::SOUND_WAITING;
        state.audioQueued = true;
        state.impatient = true;
    }
}
