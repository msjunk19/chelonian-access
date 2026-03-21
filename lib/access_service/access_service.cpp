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

// --- Helper functions ---

bool handleMasterPresenceTimeout(bool &masterPresent, unsigned long &masterLastSeen,
                                 unsigned long &masterStartTime, uint8_t &lastMasterUIDLen) {
    if (masterPresent && (millis() - masterLastSeen > 300)) {
        ESP_LOGE(TAG, "Master card removed - resetting hold");
        masterPresent = false;
        masterStartTime = 0;
        lastMasterUIDLen = 0;
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

void handleMasterCard(uint8_t *uid, uint8_t uidLength, bool &masterPresent, uint8_t *lastMasterUID,
                      uint8_t &lastMasterUIDLen, unsigned long &masterStartTime,
                      unsigned long &masterLastSeen, bool &audioQueued, uint8_t &queuedSound) {
    unsigned long now = millis();
    masterLastSeen = now;

    if (!masterPresent || !compareUID(uid, uidLength, lastMasterUID, lastMasterUIDLen)) {
        memcpy(lastMasterUID, uid, uidLength);
        lastMasterUIDLen = uidLength;
        masterStartTime = now;
        masterPresent = true;
        ESP_LOGE(TAG, "Master card detected - hold started");
    }

    if (masterPresent && (now - masterStartTime >= 2000)) {
        ESP_LOGE(TAG, "Master hold confirmed (2s)");
        if (!led.isRunning() && !audioQueued) {
            LED_SET_SEQ(PROGRAMMING_MODE);
            queuedSound = AudioContoller::SOUND_ACCEPTED;
            audioQueued = true;
        }
        masterPresent = false;
        masterStartTime = 0;
    }
}

void handleRegularCard(uint8_t *uid, uint8_t uidLength, bool &audioQueued, uint8_t &queuedSound,
                       bool &impatientEnabled, unsigned long &startTime, unsigned long &invalidTimeoutEnd) {
    bool validUID = rfid.validateUID(uid, uidLength);

    if (validUID) {
        ESP_LOGE(TAG, "Valid card");
        if (!led.isRunning() && !audioQueued) {
            LED_SET_SEQ(ACCESS_GRANTED);
            queuedSound = AudioContoller::SOUND_ACCEPTED;
            audioQueued = true;
            activateRelays();
            invalidAttempts = 0;
            impatientEnabled = false;
        }
    } else {
        ESP_LOGW(TAG, "Invalid card attempt #%u", invalidAttempts + 1);
        if (!led.isRunning() && !audioQueued) {
            LED_SET_SEQ(ACCESS_DENIED);
            queuedSound = (invalidAttempts == 0) ? AudioContoller::SOUND_DENIED_1 :
                          (invalidAttempts == 1) ? AudioContoller::SOUND_DENIED_2 :
                                                    AudioContoller::SOUND_DENIED_3;
            audioQueued = true;
            invalidTimeoutEnd = millis() + (invalidDelays[invalidAttempts] * 1000) + 3000;
            if (invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) invalidAttempts++;
            impatientEnabled = true;
            startTime = millis();
        }
    }
}

void handleImpatienceTimer(bool &audioQueued, uint8_t &queuedSound, unsigned long startTime,
                           bool &impatient, bool impatientEnabled) {
    if (impatientEnabled && (millis() - startTime > 10000) && !impatient) {
        ESP_LOGE(TAG, "10s elapsed, playing waiting sound");
        queuedSound = AudioContoller::SOUND_WAITING;
        audioQueued = true;
        impatient = true;
    }
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


void accessServiceLoop() {
    static uint8_t lastMasterUID[7];
    static uint8_t lastMasterUIDLen = 0;
    static bool masterPresent = false;
    static unsigned long masterStartTime = 0;
    static unsigned long masterLastSeen = 0;

    static bool scanned = false;
    static bool audioQueued = false;
    static uint8_t queuedSound = 0;

    static unsigned long startTime = millis(); // impatience timer
    static bool impatient = false;
    static bool impatientEnabled = true;

    static unsigned long invalidTimeoutEnd = 0; // lockout timer

    led.update();
    handleRelaySequence();

    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    if (handleMasterPresenceTimeout(masterPresent, masterLastSeen, masterStartTime, lastMasterUIDLen)) {
        return;
    }

    bool lockedOut = millis() < invalidTimeoutEnd;
    bool success = !lockedOut && rfid.readCard(uid, &uidLength);

    if (success) {
        scanned = true;
        startTime = millis();
        impatient = false;

        if (!validateUIDLength(uidLength)) {
            audioQueued = false;
            return;
        }

        bool isMaster = isMasterCard(uid, uidLength);
        if (isMaster) {
            handleMasterCard(uid, uidLength, masterPresent, lastMasterUID, lastMasterUIDLen,
                             masterStartTime, masterLastSeen, audioQueued, queuedSound);
            return;
        }

        handleRegularCard(uid, uidLength, audioQueued, queuedSound, impatientEnabled,
                          startTime, invalidTimeoutEnd);
    } else {
        scanned = false;
        handleImpatienceTimer(audioQueued, queuedSound, startTime, impatient, impatientEnabled);
    }

    // Play queued audio after LED finishes
    if (audioQueued && !led.isRunning()) {
        audio.playTrack(queuedSound);
        audioQueued = false;
    }
}