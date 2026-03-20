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


void accessServiceLoop() {
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;

    handleRelaySequence();

    static bool scanned = false;

    static bool audioQueued = false;
    static uint8_t queuedSound = 0;

    static unsigned long startTime = millis(); // impatience timer
    static bool impatient = false;
    static bool impatientEnabled = true;

    static unsigned long invalidTimeoutEnd = 0; // lockout timer

    led.update();

    static bool masterPresent = false;
    static unsigned long masterStartTime = 0;
    static uint8_t lastMasterUID[7];
    static uint8_t lastMasterUIDLen = 0;
    static unsigned long masterLastSeen = 0;

    // --- MASTER PRESENCE TIMEOUT (GLOBAL RESET) ---
    if (masterPresent && (millis() - masterLastSeen > 300)) {
        ESP_LOGE(TAG, "Master card removed - resetting hold");

        masterPresent = false;
        masterStartTime = 0;
        lastMasterUIDLen = 0;
    }

    bool success = rfid.readCard(uid, &uidLength);

    bool lockedOut = millis() < invalidTimeoutEnd;
    if (!lockedOut) {
        // Only read card if not in lockout
        success = rfid.readCard(uid, &uidLength);
    } else {
        success = false; // pretend no card
    }

    if (success) {
        scanned = true;

        // Reset impatience timer
        impatient = false;
        startTime = millis();

        bool validUID = rfid.validateUID(uid, uidLength);

        // --- Unknown UID ---
        if (uidLength == 4) {
            ESP_LOGE(TAG, "4B UID detected");
        } else if (uidLength == 7) {
            ESP_LOGE(TAG, "7B UID detected");
        } else {
            ESP_LOGE(TAG, "Unknown UID type/length");

            if (!led.isRunning()) {
                LED_SET_SEQ(UNKNOWN_UID_TYPE); // Unknown UID Type/Length
                audioQueued = false;
            }
            return;
        }
        
        
        bool isMaster = isMasterCard(uid, uidLength);

        // --- MASTER CARD ---
        if (isMaster) {

            unsigned long now = millis();
            masterLastSeen = now;  // Track Master UID

            // First time seeing master OR different card
            if (!masterPresent || !compareUID(uid, uidLength, lastMasterUID, lastMasterUIDLen)) {
                memcpy(lastMasterUID, uid, uidLength);
                lastMasterUIDLen = uidLength;

                masterStartTime = now;
                masterPresent = true;

                ESP_LOGE(TAG, "Master card detected - hold started");
            }

            // Still holding same master card
            if (masterPresent && (now - masterStartTime >= 2000)) {
                ESP_LOGE(TAG, "Master hold confirmed (2s)");

                if (!led.isRunning() && !audioQueued) {
                    LED_SET_SEQ(PROGRAMMING_MODE);

                    queuedSound = AudioContoller::SOUND_ACCEPTED;
                    audioQueued = true;

                    // 👉 NEXT STEP: set a flag like:
                    // programmingModeActive = true;
                }

                // // Prevent retrigger spam
                // if (masterPresent && (millis() - masterLastSeen > 300)) {
                //     masterPresent = false;
                // }
                // Prevent retrigger spam
                masterPresent = false;
                masterStartTime = 0;
                return;
            }

            return; // Still holding, do nothing else
        }
        // // Master Card Handling
        // bool isMaster = isMasterCard(uid, uidLength);

        // // --- MASTER CARD ---
        // if (isMaster) {
        //     unsigned long now = millis();
        //     masterLastSeen = now; // Track removed master card for debounce/programming mode

        //     if (!masterPresent || !compareUID(uid, uidLength, lastMasterUID, lastMasterUIDLen)) {
        //         memcpy(lastMasterUID, uid, uidLength);
        //         lastMasterUIDLen = uidLength;
        //         masterStartTime = now;
        //         masterPresent = true;
        //         ESP_LOGE(TAG, "Master card detected - hold started");
        //     }

        //     // Still holding same master card
        //     if (masterPresent && (now - masterStartTime >= 2000)) {
        //         ESP_LOGE(TAG, "Master hold confirmed (2s)");

        //         if (!led.isRunning() && !audioQueued) {
        //             LED_SET_SEQ(ACCESS_GRANTED);
        //             queuedSound = AudioContoller::SOUND_ACCEPTED;
        //             audioQueued = true;
        //             // 👉 NEXT STEP: set a flag like:
        //             // programmingModeActive = true;
        //         }

        //         // Prevent retrigger spam
        //         if (masterPresent && (millis() - masterLastSeen > 300)) {
        //             masterPresent = false;
        //             masterStartTime = 0; // 👈 THIS is what you're missing
        //         }
        //         return;
        //     }

        //     return; // Still holding, do nothing else
        // }

        // --- VALID CARD ---
        if (validUID) {
            ESP_LOGE(TAG, "Valid card");

            if (!led.isRunning() && !audioQueued) {
                // led.setPattern(PATTERN_SOLID, 2000, LEDColor::GREEN);
                LED_SET_SEQ(ACCESS_GRANTED);
                // led.enqueuePattern(PATTERN_TRIPLE_BLINK, 2000, LEDColor::GREEN);
                // led.enqueuePattern(PATTERN_SOLID, 2000, LEDColor::GREEN);

                queuedSound = AudioContoller::SOUND_ACCEPTED;
                audioQueued = true;

                activateRelays();

                invalidAttempts = 0;

                // Disable impatience after success
                impatientEnabled = false;
                impatient = false;
            }
        }
        // --- INVALID CARD ---
        else {
            ESP_LOGW(TAG, "Invalid card attempt #%u", invalidAttempts + 1);

            if (!led.isRunning() && !audioQueued) {
                LED_SET_SEQ(ACCESS_DENIED);

                // Queue correct sound
                if (invalidAttempts == 0)
                    queuedSound = AudioContoller::SOUND_DENIED_1;
                else if (invalidAttempts == 1)
                    queuedSound = AudioContoller::SOUND_DENIED_2;
                else
                    queuedSound = AudioContoller::SOUND_DENIED_3;

                audioQueued = true;

                // 🔒 Start lockout timer (non-blocking)
                invalidTimeoutEnd = millis() + (invalidDelays[invalidAttempts] * 1000) + (3000); // Replicate the 3 Second + 1s*invalid card read Timeout for Brute Force Protection

                // Increment attempts AFTER scheduling
                if (invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1)
                    invalidAttempts++;

                // Re-enable impatience after invalid
                impatientEnabled = true;
                impatient = false;
                startTime = millis();
            }
        }

    } else {
        scanned = false;

        // logic for waiting sound
        if (impatientEnabled && millis() - startTime > 10000 && !impatient) {
            ESP_LOGE(TAG, "10s elapsed, playing waiting sound");

            queuedSound = AudioContoller::SOUND_WAITING;
            audioQueued = true;

            impatient = true;
        }
    }

    // --- Play audio AFTER LED finishes ---
    if (audioQueued && !led.isRunning()) {
        audio.playTrack(queuedSound);
        audioQueued = false;
    }
}

//Working but broken formatting
// void accessServiceLoop() { 
//     uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; 
//     uint8_t uidLength; 
    
//     handleRelaySequence(); 

//     static bool scanned = false; 

//     static bool audioQueued = false; 
//     static uint8_t queuedSound = 0; 

//     static unsigned long startTime = millis(); // impatience timer 
//     static bool impatient = false; 
//     static bool impatientEnabled = true; 

//     static unsigned long invalidTimeoutEnd = 0; // lockout timer 

//     led.update(); 

//     static bool masterPresent = false; 
//     static unsigned long masterStartTime = 0; 
//     static uint8_t lastMasterUID[7]; 
//     static uint8_t lastMasterUIDLen = 0; 
//     static unsigned long masterLastSeen = 0; 

//     // bool success; 
//     bool success = rfid.readCard(uid, &uidLength);

//     bool lockedOut = millis() < invalidTimeoutEnd; 
//     if (!lockedOut) { 
//         // Only read card if not in lockout 
//         success = rfid.readCard(uid, &uidLength); 
//     } else { 
//         success = false; // pretend no card 
//     }
    
//     if (success) { 
//         scanned = true; 
        
//         // Reset impatience timer 
//         impatient = false; 
//         startTime = millis();

//         bool validUID = rfid.validateUID(uid, uidLength); 
        
//         // --- Unknown UID --- 
        
//         if (uidLength == 4) { 
//             ESP_LOGE(TAG, "4B UID detected"); 
//         } else if (uidLength == 7) { 
//             ESP_LOGE(TAG, "7B UID detected"); 
//         } else { 
//             ESP_LOGE(TAG, "Unknown UID type/length"); 

//             if (!led.isRunning()) { 
//                 LED_SET_SEQ(UNKNOWN_UID_TYPE);//Unknown UID Type/Length 
//                 audioQueued = false; 
//             } 
//             return; 
//         }

//         // Master Card Handling 
//         bool isMaster = isMasterCard(uid, uidLength); 
        
//         // --- MASTER CARD --- 
//         if (isMaster) { 
//             unsigned long now = millis(); 
//             masterLastSeen = now; // Track removed master card for debounce/programming mode 
//             if (!masterPresent || !compareUID(uid, uidLength, lastMasterUID, lastMasterUIDLen)) { 
//                 memcpy(lastMasterUID, uid, uidLength); 
//                 lastMasterUIDLen = uidLength; 
//                 masterStartTime = now; 
//                 masterPresent = true; 
//                 ESP_LOGE(TAG, "Master card detected - hold started"); 
            
//             } 
//             // Still holding same master card 
//             if (masterPresent && (now - masterStartTime >= 2000)) { 
//                 ESP_LOGE(TAG, "Master hold confirmed (2s)"); 
//                 if (!led.isRunning() && !audioQueued) { 
//                     LED_SET_SEQ(ACCESS_GRANTED); 
//                     queuedSound = AudioContoller::SOUND_ACCEPTED; 
//                     audioQueued = true; 
//                     // 👉 NEXT STEP: set a flag like: 
//                     // programmingModeActive = true; 
                   
//                 } 
//                     // Prevent retrigger spam 
//                     if (masterPresent && (millis() - masterLastSeen > 300)) { 
//                         masterPresent = false; 
//                         masterStartTime = 0; // 👈 THIS is what you're missing 
//                         } 
//                         return; 
//                 } 
//                     return; // Still holding, do nothing else 
//             }
                    
                    
//                     // --- VALID CARD --- 
//                     if (validUID) { 
//                         ESP_LOGE(TAG, "Valid card"); 

//                         if (!led.isRunning() && !audioQueued) { 
//                             // led.setPattern(PATTERN_SOLID, 2000, LEDColor::GREEN); 
//                             LED_SET_SEQ(ACCESS_GRANTED); 
//                             // led.enqueuePattern(PATTERN_TRIPLE_BLINK, 2000, LEDColor::GREEN); 
//                             // led.enqueuePattern(PATTERN_SOLID, 2000, LEDColor::GREEN); 
                            
//                             queuedSound = AudioContoller::SOUND_ACCEPTED; 
//                             audioQueued = true; 
                            
//                             activateRelays(); 
                            
//                             invalidAttempts = 0; 
                            
//                             // Disable impatience after success 
//                             impatientEnabled = false; 
//                             impatient = false; 
//                         } 
//                     } 
//                     // --- INVALID CARD --- 
//                     else { 
//                         ESP_LOGW(TAG, "Invalid card attempt #%u", invalidAttempts + 1); 
                        
//                         if (!led.isRunning() && !audioQueued) { 
//                             LED_SET_SEQ(ACCESS_DENIED); 

//                             // Queue correct sound 
//                             if (invalidAttempts == 0) queuedSound = AudioContoller::SOUND_DENIED_1; 
//                             else if (invalidAttempts == 1) queuedSound = AudioContoller::SOUND_DENIED_2; 
//                             else queuedSound = AudioContoller::SOUND_DENIED_3; 
                            
//                             audioQueued = true;
                            
//                             // 🔒 Start lockout timer (non-blocking) 
//                             invalidTimeoutEnd = millis() + (invalidDelays[invalidAttempts] * 1000) + (3000); //Replicate the 3 Second + 1s*invalid card read Timeout for Brute Force Protection 
                            
//                             // Increment attempts AFTER scheduling 
//                             if (invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) 
//                             invalidAttempts++; 
                            
//                             // Re-enable impatience after invalid 
//                             impatientEnabled = true; 
//                             impatient = false; 
//                             startTime = millis(); 
//                         } 
//                     } 
//                 } else { 
//                     scanned = false; 
                    
//                     // logic for waiting sound 
//                     if (impatientEnabled && millis() - startTime > 10000 && !impatient) { 
//                         ESP_LOGE(TAG, "10s elapsed, playing waiting sound");

//                         queuedSound = AudioContoller::SOUND_WAITING; 
//                         audioQueued = true; 

//                         impatient = true; 
//                     } 
//                 }
//                     // --- Play audio AFTER LED finishes --- 
//                     if (audioQueued && !led.isRunning()) {
//                     audio.playTrack(queuedSound); 
//                     audioQueued = false; 
//                     } 
//             }
 