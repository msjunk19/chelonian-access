#include "access_service.h"

#include <Arduino.h>
#include "esp_log.h"

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

void accessServiceSetup() {
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
    boolean success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;

    handleRelaySequence();

    static bool impatient = false;
    static bool impatientEnabled = true;
    static unsigned long startTime = millis();

    success = rfid.readCard(uid, &uidLength);

    if (success) {
        // Reset impatient timer immediately
        impatient = false;
        startTime = millis();

        scanned = true;
        ESP_LOGE(TAG, "Card detected");
        ESP_LOGE(TAG, "UID Length: %u bytes", uidLength);
        ESP_LOGE(TAG, "UID Value:");
        for (uint8_t i = 0; i < uidLength; i++) {
            char buffer[20];
            sprintf(buffer, " 0x%02X", uid[i]);
            // Could accumulate into a string if desired
        }

        bool validUID = rfid.validateUID(uid, uidLength);

        if (uidLength == 4) {
            ESP_LOGE(TAG, "4B UID detected");
        } else if (uidLength == 7) {
            ESP_LOGE(TAG, "7B UID detected");
        } else {
            ESP_LOGE(TAG, "Unknown UID type/length");
        }

        if (validUID) {
            ESP_LOGE(TAG, "Valid card - activating relays");
            invalidAttempts = 0;
            audio.playTrack(AudioContoller::SOUND_ACCEPTED);
            activateRelays();

            // Disable impatient logic after success
            impatientEnabled = false;
            impatient = false;
        } else {
            ESP_LOGW(TAG, "Invalid card - attempt #%u", invalidAttempts + 1);

            // Re-enable impatient logic and restart timer
            impatientEnabled = true;
            impatient = false;
            startTime = millis();


            if (invalidAttempts == 0) audio.playTrack(AudioContoller::SOUND_DENIED_1);
            else if (invalidAttempts == 1) audio.playTrack(AudioContoller::SOUND_DENIED_2);
            else audio.playTrack(AudioContoller::SOUND_DENIED_3);

            delay(3000);
            delay(invalidDelays[invalidAttempts] * 1000);

            if (invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) invalidAttempts++;
        }

    } else {
        // Play waiting sound if no card present
        if (impatientEnabled && millis() - startTime > 10000 && !impatient) {
            ESP_LOGE(TAG, "10s elapsed, playing waiting sound");
            audio.playTrack(AudioContoller::SOUND_WAITING);
            impatient = true;
        }
    }
}