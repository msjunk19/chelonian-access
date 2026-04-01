#include <access_service.h>
#include <Arduino.h>
#include <esp_log.h>
#include <led_states.hpp>
#include <globals.hpp>
#include <pin_mapping.hpp>
#include <master_uid_manager.h>
#include <relay_states.hpp>

static const char* TAG = "ACCESS";  // Add TAG definition

// Tracks which action is currently running so handleRelaySequence knows the config
RelayAction activeRelayAction = RelayAction::UNLOCK;

// Instantiate controllers

// LED Selection, only use one. 
// LEDController led(PN_LED); //Single Color LED on pin 8
// LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10

RFIDController rfid;
RelayController relays;
AudioContoller audio;


const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS] = {1,  3,  4,  5,  8,  12, 17,
                                                         23, 30, 38, 47, 57, 68};

static AccessLoopState state;

void accessServiceSetup() {
    macroConfigManager.load();
    led.begin();
    LED_SET_SEQ(SYSTEM_READY);
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
    ESP_LOGI(TAG, "System Boot Complete...");
    audio.playTrack(AudioContoller::SOUND_STARTUP);
}
    ESP_LOGI(TAG, "Waiting for an ISO14443A card");
    markUserActivity(state);
}


// void triggerRelayAction(RelayAction action) {
//     if (state.relayActive) return; // prevent re-triggering during active sequence

//     activeRelayAction = action;
//     RelayActionConfig& cfg = relayConfigManager.get(action);

//     state.relayActivatedTime = millis();
//     state.relayActive        = true;

//     switch (cfg.mode) {
//         case RelayMode::SINGLE:
//             relays.setRelay(cfg.relay, true);
//             state.currentRelayState = RELAY_SINGLE_ACTIVE;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: single pulse relay %d", 
//                 ACTION_NAMES[(uint8_t)action], cfg.relay);
//             break;

//         case RelayMode::DOUBLE:
//             relays.setRelay(cfg.relay, true);
//             state.currentRelayState = RELAY_DOUBLE_PULSE1;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: double pulse relay %d",
//                 ACTION_NAMES[(uint8_t)action], cfg.relay);
//             break;

//         case RelayMode::SEQUENTIAL:
//             relays.setRelay(cfg.relay, true);
//             state.currentRelayState = RELAY_SEQUENTIAL_1;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: sequential relay %d->%d",
//                 ACTION_NAMES[(uint8_t)action], cfg.relay, cfg.relay2);
//             break;

//         case RelayMode::ALL:
//             for (uint8_t i = 0; i < 4; i++) relays.setRelay(i, true);
//             state.currentRelayState = RELAY_ALL_ACTIVE;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: all relays",
//                 ACTION_NAMES[(uint8_t)action]);
//             break;

//         default:
//             ESP_LOGE(TAG, "Unknown relay mode");
//             state.relayActive = false;
//             break;
//     }
// }

// void handleRelaySequence() {
//     RelayActionConfig& cfg = relayConfigManager.get(activeRelayAction);

//     switch (state.currentRelayState) {
//         case RELAY_IDLE:
//             break;

//         case RELAY_SINGLE_ACTIVE:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "Single pulse complete");
//             }
//             break;

//         case RELAY_DOUBLE_PULSE1:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState  = RELAY_DOUBLE_GAP;
//                 ESP_LOGI(TAG, "Double pulse 1 complete — gap started");
//             }
//             break;

//         case RELAY_DOUBLE_GAP:
//             if (millis() - state.relayActivatedTime >= cfg.gap) {
//                 relays.setRelay(cfg.relay, true);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState  = RELAY_DOUBLE_PULSE2;
//                 ESP_LOGI(TAG, "Double pulse 2 started");
//             }
//             break;

//         case RELAY_DOUBLE_PULSE2:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "Double pulse complete");
//             }
//             break;

//         case RELAY_SEQUENTIAL_1:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 relays.setRelay(cfg.relay2, true);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState  = RELAY_SEQUENTIAL_2;
//                 ESP_LOGI(TAG, "Sequential relay 1->2");
//             }
//             break;

//         case RELAY_SEQUENTIAL_2:
//             if (millis() - state.relayActivatedTime >= cfg.duration2) {
//                 relays.setRelay(cfg.relay2, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "Sequential pulse complete");
//             }
//             break;

//         case RELAY_ALL_ACTIVE:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 for (uint8_t i = 0; i < 4; i++) relays.setRelay(i, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "All relay pulse complete");
//             }
//             break;

//         default:
//             ESP_LOGE(TAG, "Unhandled relay state: %d", state.currentRelayState);
//             break;
//     }
// }

// void handleRelaySequence() {
//     switch (state.currentRelayState) {
//         case RELAY_IDLE:
//             break;
//         case RELAY1_ACTIVE:
//             if (millis() - state.relayActivatedTime >= RELAY1_DURATION) {
//                 relays.setRelay(RELAY_1, false);
//                 relays.setRelay(RELAY_2, true);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState = RELAY2_ACTIVE;
//                 ESP_LOGI(TAG, "Relay state transition 1->2");
//             }
//             break;
//         case RELAY2_ACTIVE:
//             if (millis() - state.relayActivatedTime >= RELAY2_DURATION) {
//                 relays.setRelay(RELAY_2, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive = false;
//                 ESP_LOGI(TAG, "Relay sequence complete");
//             }
//             break;
//         default:
//             ESP_LOGE(TAG, "Unhandled relay state: %d", state.currentRelayState);
//             break;
//     }
// }

// void activateRelays() {
//     relays.setRelay(RELAY_1, true);
//     state.relayActivatedTime = millis();
//     state.currentRelayState = RELAY1_ACTIVE;
//     state.relayActive = true;
//     ESP_LOGI(TAG, "Starting relay sequence (state 1)");
// }

// static void updateHardware() {
//     led.update();
//     handleRelaySequence();
// }

void triggerMacro(uint8_t macroIndex) {
    if (state.relayActive) return;

    Macro& m = macroConfigManager.config.macros[macroIndex];
    if (m.step_count == 0) return;

    // copy macro into active state so config changes mid-sequence dont affect it
    state.activeMacro   = m;
    state.currentStep   = 0;
    state.relayActive   = true;

    // fire step 0 immediately
    _fireStep(state.activeMacro.steps[0]);
    state.relayActivatedTime = millis();
    state.currentRelayState  = RELAY_STEP_ACTIVE;

    ESP_LOGI(TAG, "Macro started: %s (%u steps)", m.name, m.step_count);
}

void handleRelaySequence() {
    if (state.currentRelayState == RELAY_IDLE) return;

    RelayStep& step = state.activeMacro.steps[state.currentStep];

    switch (state.currentRelayState) {

        case RELAY_STEP_ACTIVE:
            if (millis() - state.relayActivatedTime >= step.duration) {
                // turn off all relays fired in this step
                _clearStep(step);

                // is there a next step?
                if (state.currentStep + 1 < state.activeMacro.step_count) {
                    // start gap before next step
                    state.relayActivatedTime = millis();
                    state.currentRelayState  = RELAY_STEP_GAP;
                    ESP_LOGI(TAG, "Step %u complete — gap started", state.currentStep);
                } else {
                    // sequence complete
                    state.currentRelayState = RELAY_IDLE;
                    state.relayActive       = false;
                    ESP_LOGI(TAG, "Macro complete: %s", state.activeMacro.name);
                }
            }
            break;

        case RELAY_STEP_GAP:
            if (millis() - state.relayActivatedTime >= step.gap) {
                state.currentStep++;
                RelayStep& next = state.activeMacro.steps[state.currentStep];
                _fireStep(next);
                state.relayActivatedTime = millis();
                state.currentRelayState  = RELAY_STEP_ACTIVE;
                ESP_LOGI(TAG, "Step %u started", state.currentStep);
            }
            break;

        default:
            ESP_LOGE(TAG, "Unhandled relay state: %d", state.currentRelayState);
            break;
    }
}

void fireMacro(uint8_t macroIndex) {
    triggerMacro(state, macroIndex);
}

static uint32_t lastRFIDCheck = 0;
static bool rfidOK = true;

static void updateHardware() {
    led.update();
    // handleRelaySequence();
    handleRelaySequence(state);
    
    if (millis() - lastRFIDCheck > 5000) {
        lastRFIDCheck = millis();
        bool responding = rfid.isResponding();
        
        if (!responding) {
            rfidOK = false;
            rfid.reinitialize();
        } else if (!rfidOK && responding) {
            // Just recovered — reapply SAMConfig
            ESP_LOGI("ACCESS", "PN532 recovered — reapplying SAMConfig");
            rfid.reinitialize(); // force full reinit including SAMConfig
            rfidOK = true;
        } else {
            rfidOK = true;
        }
    }
}


static bool handleBootProgrammingCheck() {
    static bool bootChecked = false;
    static bool bootMasterProgrammingMode = false;

    if (!bootChecked) {
        if (!masterUidManager.hasMasterUIDs) {
            ESP_LOGW(TAG, "No Master UID Stored. Enter Master Programming Mode.");
            bootMasterProgrammingMode = true;
            LED_SET_SEQ(PROGRAMMING_MODE);
        }
        bootChecked = true;
    }
    return false;
}


static bool handleMasterProgrammingMode(uint8_t* uid, uint8_t& uidLength) {
    static bool masterProgrammingMode = !masterUidManager.hasMasterUIDs;
    static bool waitingForRemoval = false;

    static uint8_t storedUID[7];
    static uint8_t storedLength = 0;

    if (!masterProgrammingMode) return false;
    // disableImpatience(state);

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
            yield(); // feed the WDT

            masterUidManager.hasMasterUIDs = true;
            masterProgrammingMode = false;
            waitingForRemoval = false;
            ESP_LOGI(TAG, "Master UID Stored, Exiting Master Programming.");
            // ESP_LOGI(TAG, "Waiting for an ISO14443A card");
            // markUserActivity(state);
        }
    }

    return true;
}

static bool handleUserProgrammingMode(uint8_t* uid, uint8_t uidLength) {
    if (!state.userProgrammingModeActive) return false;

    // Disable impatient timer while in programming mode
    state.impatientEnabled = false;  // stops the normal impatience loop
    state.impatient = false;         // reset any current impatience state

    // ESP_LOGI(TAG, "User Programming Mode. Disabling Impatience Timer");
    // disableImpatience(state);

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
        ESP_LOGI(TAG, "Exit Stage. Enabling Impatience Timer...");

        state.lastActivityTime = millis();
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

    if (masterUidManager.checkUID(uid, uidLength)) {
        handleMasterCard(uid, uidLength, state);
        return;
    }

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
    if (handleUserProgrammingMode(uid, uidLength)) return;

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
        LED_SET_SEQ(MASTER_CARD);
        ESP_LOGI(TAG, "Master card detected - hold started");
    }

    state.masterLastSeen = now;

    // Hold detection
    if (state.masterPresent && (now - state.masterStartTime >= MASTER_HOLD_TIME)) {
        ESP_LOGI(TAG, "Master hold confirmed (%us)", (MASTER_HOLD_TIME / 1000));

        if (!led.isRunning() && !state.audioQueued) {
            LED_SET_SEQ(PROGRAMMING_MODE);
            //TODO fix sound
            audio.playTrack(AudioContoller::SOUND_WAITING);


            // state.queuedSound = AudioContoller::SOUND_ACCEPTED;
            // state.audioQueued = true;

            state.userProgrammingModeActive = true;
            state.userProgLastActivityTime = millis();
            state.userProgWarningGiven = false;

            ESP_LOGI(TAG, "User Programming Mode Enabled");
        }
    }
}


// --- User Card Handler ---

static void handleAccessGranted(AccessLoopState& state) {
    ESP_LOGI(TAG, "Valid Card, Access Granted");
    if (!led.isRunning() && !state.audioQueued) {
        LED_SET_SEQ(ACCESS_GRANTED);
        state.queuedSound      = AudioContoller::SOUND_ACCEPTED;
        state.audioQueued      = true;
        triggerMacro(state, macroConfigManager.config.tag_macro);
        state.invalidAttempts  = 0;
        state.impatientEnabled = false;
        state.impatient        = false;
        ESP_LOGI(TAG, "Access Granted. Disabling Impatience Timer");
    }
}

static void handleAccessDenied(AccessLoopState &state) {
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
        if (state.invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) 
            state.invalidAttempts++;
        markUserActivity(state);
    }
}

void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
    bool validUID = rfid.validateUID(uid, uidLength);
    if (validUID) {
        handleAccessGranted(state);
    } else {
        handleAccessDenied(state);
    }
}


// --- Impatience Timeout ---
// Currently working. Impatient at boot with known MasterUID, Impatient after unknown tag, impatient after short master card hold, Not impatient after access granted, impatience disabled during programming mode
// Not working. markUserActivity blocks programming mode boot announcements
void handleImpatienceTimer(AccessLoopState &state) {
if (state.impatientEnabled &&
    !state.impatient &&
    millis() - state.lastActivityTime > IMPATIENCE_TIMEOUT)
{
    state.impatient = true;

    ESP_LOGW(TAG, " %ds elapsed, notifying", IMPATIENCE_TIMEOUT / 1000);
    LED_SET_SEQ(SYSTEM_IDLE);
    audio.playTrack(AudioContoller::SOUND_WAITING);
}
}

// Not working. markUserActivity blocks programming mode boot announcements
void markUserActivity(AccessLoopState& state)
{
    state.lastActivityTime = millis();
    state.impatientEnabled = true;
    state.impatient = false;
    ESP_LOGI(TAG, "FUNCTION: No User Interaction...Timer Starting...");
}

void disableImpatience(AccessLoopState& state)
{
    // Disable impatient timer while in programming mode
    state.impatientEnabled = false;  // stops the normal impatience loop
    state.impatient = false;         // reset any current impatience state
    ESP_LOGI(TAG, "Disabling Impatience Timer...");
}



// #include <access_service.h>
// #include <Arduino.h>
// #include <esp_log.h>
// #include <led_states.hpp>
// #include <globals.hpp>
// #include <pin_mapping.hpp>
// #include <master_uid_manager.h>
// #include <relay_states.hpp>

// static const char* TAG = "ACCESS";  // Add TAG definition

// // Tracks which action is currently running so handleRelaySequence knows the config
// RelayAction activeRelayAction = RelayAction::UNLOCK;

// // Instantiate controllers

// // LED Selection, only use one. 
// // LEDController led(PN_LED); //Single Color LED on pin 8
// // LEDController led(0, true, PN_NEOPIXEL); //Neopixel on pin 10

// RFIDController rfid;
// RelayController relays;
// AudioContoller audio;


// const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS] = {1,  3,  4,  5,  8,  12, 17,
//                                                          23, 30, 38, 47, 57, 68};

// static AccessLoopState state;

// void accessServiceSetup() {
//     led.begin();
//     LED_SET_SEQ(SYSTEM_READY);
//     rfid.begin();
//     rfid.printFirmwareVersion();
//     relays.begin();
//     if (audio.begin()) {
//     audio.setVolume(AUDIO_DEFAULT_VOLUME);

//     // wait up to 500ms for the controller to be ready, non-blocking-ish
//     unsigned long start = millis();
//     while (millis() - start < 500) {
//         // tiny sleep to yield CPU to other tasks
//         delay(1);  
//     }
//     ESP_LOGI(TAG, "System Boot Complete...");
//     audio.playTrack(AudioContoller::SOUND_STARTUP);
// }
//     ESP_LOGI(TAG, "Waiting for an ISO14443A card");
//     markUserActivity(state);
// }


// void triggerRelayAction(RelayAction action) {
//     if (state.relayActive) return; // prevent re-triggering during active sequence

//     activeRelayAction = action;
//     RelayActionConfig& cfg = relayConfigManager.get(action);

//     state.relayActivatedTime = millis();
//     state.relayActive        = true;

//     switch (cfg.mode) {
//         case RelayMode::SINGLE:
//             relays.setRelay(cfg.relay, true);
//             state.currentRelayState = RELAY_SINGLE_ACTIVE;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: single pulse relay %d", 
//                 ACTION_NAMES[(uint8_t)action], cfg.relay);
//             break;

//         case RelayMode::DOUBLE:
//             relays.setRelay(cfg.relay, true);
//             state.currentRelayState = RELAY_DOUBLE_PULSE1;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: double pulse relay %d",
//                 ACTION_NAMES[(uint8_t)action], cfg.relay);
//             break;

//         case RelayMode::SEQUENTIAL:
//             relays.setRelay(cfg.relay, true);
//             state.currentRelayState = RELAY_SEQUENTIAL_1;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: sequential relay %d->%d",
//                 ACTION_NAMES[(uint8_t)action], cfg.relay, cfg.relay2);
//             break;

//         case RelayMode::ALL:
//             for (uint8_t i = 0; i < 4; i++) relays.setRelay(i, true);
//             state.currentRelayState = RELAY_ALL_ACTIVE;
//             ESP_LOGI(TAG, "RELAY_ACTION %s: all relays",
//                 ACTION_NAMES[(uint8_t)action]);
//             break;

//         default:
//             ESP_LOGE(TAG, "Unknown relay mode");
//             state.relayActive = false;
//             break;
//     }
// }

// void handleRelaySequence() {
//     RelayActionConfig& cfg = relayConfigManager.get(activeRelayAction);

//     switch (state.currentRelayState) {
//         case RELAY_IDLE:
//             break;

//         case RELAY_SINGLE_ACTIVE:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "Single pulse complete");
//             }
//             break;

//         case RELAY_DOUBLE_PULSE1:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState  = RELAY_DOUBLE_GAP;
//                 ESP_LOGI(TAG, "Double pulse 1 complete — gap started");
//             }
//             break;

//         case RELAY_DOUBLE_GAP:
//             if (millis() - state.relayActivatedTime >= cfg.gap) {
//                 relays.setRelay(cfg.relay, true);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState  = RELAY_DOUBLE_PULSE2;
//                 ESP_LOGI(TAG, "Double pulse 2 started");
//             }
//             break;

//         case RELAY_DOUBLE_PULSE2:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "Double pulse complete");
//             }
//             break;

//         case RELAY_SEQUENTIAL_1:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 relays.setRelay(cfg.relay, false);
//                 relays.setRelay(cfg.relay2, true);
//                 state.relayActivatedTime = millis();
//                 state.currentRelayState  = RELAY_SEQUENTIAL_2;
//                 ESP_LOGI(TAG, "Sequential relay 1->2");
//             }
//             break;

//         case RELAY_SEQUENTIAL_2:
//             if (millis() - state.relayActivatedTime >= cfg.duration2) {
//                 relays.setRelay(cfg.relay2, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "Sequential pulse complete");
//             }
//             break;

//         case RELAY_ALL_ACTIVE:
//             if (millis() - state.relayActivatedTime >= cfg.duration) {
//                 for (uint8_t i = 0; i < 4; i++) relays.setRelay(i, false);
//                 state.currentRelayState = RELAY_IDLE;
//                 state.relayActive       = false;
//                 ESP_LOGI(TAG, "All relay pulse complete");
//             }
//             break;

//         default:
//             ESP_LOGE(TAG, "Unhandled relay state: %d", state.currentRelayState);
//             break;
//     }
// }

// // void handleRelaySequence() {
// //     switch (state.currentRelayState) {
// //         case RELAY_IDLE:
// //             break;
// //         case RELAY1_ACTIVE:
// //             if (millis() - state.relayActivatedTime >= RELAY1_DURATION) {
// //                 relays.setRelay(RELAY_1, false);
// //                 relays.setRelay(RELAY_2, true);
// //                 state.relayActivatedTime = millis();
// //                 state.currentRelayState = RELAY2_ACTIVE;
// //                 ESP_LOGI(TAG, "Relay state transition 1->2");
// //             }
// //             break;
// //         case RELAY2_ACTIVE:
// //             if (millis() - state.relayActivatedTime >= RELAY2_DURATION) {
// //                 relays.setRelay(RELAY_2, false);
// //                 state.currentRelayState = RELAY_IDLE;
// //                 state.relayActive = false;
// //                 ESP_LOGI(TAG, "Relay sequence complete");
// //             }
// //             break;
// //         default:
// //             ESP_LOGE(TAG, "Unhandled relay state: %d", state.currentRelayState);
// //             break;
// //     }
// // }

// // void activateRelays() {
// //     relays.setRelay(RELAY_1, true);
// //     state.relayActivatedTime = millis();
// //     state.currentRelayState = RELAY1_ACTIVE;
// //     state.relayActive = true;
// //     ESP_LOGI(TAG, "Starting relay sequence (state 1)");
// // }

// // static void updateHardware() {
// //     led.update();
// //     handleRelaySequence();
// // }

// static uint32_t lastRFIDCheck = 0;

// static bool rfidOK = true;

// static void updateHardware() {
//     led.update();
//     handleRelaySequence();
    
//     if (millis() - lastRFIDCheck > 5000) {
//         lastRFIDCheck = millis();
//         bool responding = rfid.isResponding();
        
//         if (!responding) {
//             rfidOK = false;
//             rfid.reinitialize();
//         } else if (!rfidOK && responding) {
//             // Just recovered — reapply SAMConfig
//             ESP_LOGI("ACCESS", "PN532 recovered — reapplying SAMConfig");
//             rfid.reinitialize(); // force full reinit including SAMConfig
//             rfidOK = true;
//         } else {
//             rfidOK = true;
//         }
//     }
// }

// static bool handleBootProgrammingCheck() {
//     // ESP_LOGI(TAG, "Boot Programming Check Code Executing...");
//     static bool bootChecked = false;
//     static bool bootMasterProgrammingMode = false; //2

//     if (!bootChecked) {
//         if (!masterUidManager.hasMasterUIDs) {
//             ESP_LOGW(TAG, "No Master UID Stored. Enter Master Programming Mode.");
//             bootMasterProgrammingMode = true;
//             LED_SET_SEQ(PROGRAMMING_MODE);
//         // disableImpatience(state);

//         }
//         bootChecked = true;
//     }
//     return false; // does not early-exit loop
// }


// static bool handleMasterProgrammingMode(uint8_t* uid, uint8_t& uidLength) {
//     static bool masterProgrammingMode = !masterUidManager.hasMasterUIDs;
//     static bool waitingForRemoval = false;

//     static uint8_t storedUID[7];
//     static uint8_t storedLength = 0;

//     if (!masterProgrammingMode) return false;
//     // disableImpatience(state);

//     bool cardDetected = rfid.readCard(uid, &uidLength);

//     // 🟢 STEP 1: Detect card ONCE
//     if (!waitingForRemoval) {

//         if (cardDetected) {

//             storedLength = uidLength > sizeof(storedUID) ? sizeof(storedUID) : uidLength;
//             memcpy(storedUID, uid, storedLength);

//             char uidStr[32] = {0};
//             for (uint8_t i = 0, pos = 0; i < storedLength && pos + 3 < sizeof(uidStr); i++) {
//                 if (i > 0) uidStr[pos++] = ':';
//                 pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", storedUID[i]);
//             }

//             ESP_LOGI(TAG, "Detected New Master Card: UID: %s", uidStr);

//             waitingForRemoval = true;

//             LED_SET_SEQ(MASTER_CARD_SET);
//         }
//     }
//     // 🔴 STEP 2: Wait for removal (simple debounce)
//     else {
//         if (!cardDetected) {

//             uint8_t* uids[] = {storedUID};
//             uint8_t lengths[] = {storedLength};

//             masterUidManager.writeUIDs(uids, lengths, 1);
//             yield(); // feed the WDT

//             masterUidManager.hasMasterUIDs = true;
//             masterProgrammingMode = false;
//             waitingForRemoval = false;
//             ESP_LOGI(TAG, "Master UID Stored, Exiting Master Programming.");
//             // ESP_LOGI(TAG, "Waiting for an ISO14443A card");
//             // markUserActivity(state);
//         }
//     }

//     return true;
// }

// static bool handleUserProgrammingMode(uint8_t* uid, uint8_t uidLength) {
//     if (!state.userProgrammingModeActive) return false;

//     // Disable impatient timer while in programming mode
//     state.impatientEnabled = false;  // stops the normal impatience loop
//     state.impatient = false;         // reset any current impatience state

//     // ESP_LOGI(TAG, "User Programming Mode. Disabling Impatience Timer");
//     // disableImpatience(state);

//     unsigned long now = millis();
//     bool cardDetected = rfid.readCard(uid, &uidLength);

//     if (cardDetected) {
//         state.userProgLastActivityTime = now;
//         state.userProgWarningGiven = false;

//         if (masterUidManager.checkUID(uid, uidLength)) {
//             ESP_LOGV(TAG, "Master card detected - ignore during user programming mode");
//             return true;
//         }

//         // Ensure the card is fully removed
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
//         LED_SET_SEQ(PLACEHOLDER);
//     }

//     // Exit stage
//     if (now - state.userProgLastActivityTime > EXIT_TIMEOUT) {
//         ESP_LOGW(TAG, "Exiting User Programming Mode");
//         state.userProgrammingModeActive = false;
//         // Re-enable impatience timer for normal operation
//         ESP_LOGI(TAG, "Exit Stage. Enabling Impatience Timer...");

//         state.lastActivityTime = millis();
//         state.impatientEnabled = true;

//         LED_SET_SEQ(PLACEHOLDER);
//         return false;
//     }

//     return true;
// }


// static bool uidMatches(const uint8_t* a, uint8_t lenA, const uint8_t* b, uint8_t lenB) {
//     if (lenA != lenB) return false;
//     return memcmp(a, b, lenA) == 0;
// }

// static bool handleMasterTimeout() {
//     return handleMasterPresenceTimeout(state);
// }

// static void handleCardDetected(uint8_t* uid, uint8_t uidLength) {
//     state.scanned = true;

//     if (!validateUIDLength(uidLength)) {
//         state.audioQueued = false;
//         return;
//     }

//     // Handle master card first
//     if (masterUidManager.checkUID(uid, uidLength)) {
//         handleMasterCard(uid, uidLength, state);
//         return;  // skip regular card if master
//     }

//     // Regular card processing
//     handleRegularCard(uid, uidLength, state);
// }

// static void handleNoCard() {
//     state.scanned = false;
//     handleImpatienceTimer(state);
// }

// static void processCardScan(uint8_t* uid, uint8_t& uidLength) {
//     bool lockedOut = millis() < state.invalidTimeoutEnd;
//     bool cardDetected = !lockedOut && rfid.readCard(uid, &uidLength);

//     if (cardDetected) {
//         handleCardDetected(uid, uidLength);
//     } else {
//         handleNoCard();
//     }
// }

// static void handleAudioPlayback() {
//     if (state.audioQueued && !led.isRunning()) {
//         audio.playTrack(state.queuedSound);
//         state.audioQueued = false;
//     }
// }


// void accessServiceLoop() {
//     updateHardware();

//     uint8_t uid[7] = {0};
//     uint8_t uidLength = 0;
    
//     if (handleBootProgrammingCheck()) return;
//     // if (handleBootProgrammingCheck()) { ESP_LOGV(TAG, "handleBootProgrammingCheck returned true"); return; }
//     if (handleMasterProgrammingMode(uid, uidLength)) return;
//     // if (handleMasterProgrammingMode(uid, uidLength)) { ESP_LOGV(TAG, "handleMasterProgrammingMode returned true"); return; }
//     if (handleMasterTimeout()) return;
//     // if (handleMasterTimeout()) { ESP_LOGV(TAG, "handleMasterTimeout returned true"); return; }
//     if (handleUserProgrammingMode(uid, uidLength)) return; 
//     // if (handleUserProgrammingMode(uid, uidLength)) { ESP_LOGV(TAG, "handleUserProgrammingMode returned true"); return; }

//     processCardScan(uid, uidLength);
//     handleAudioPlayback();
// }


// // --- Helper functions using AccessLoopState ---
// bool handleMasterPresenceTimeout(AccessLoopState &state) {
//     if (state.masterPresent && (millis() - state.masterLastSeen > 300)) {
//         ESP_LOGI(TAG, "Master card removed - resetting hold");
//         state.masterPresent = false;
//         state.masterStartTime = 0;
//         state.lastMasterUIDLen = 0;
//         markUserActivity(state);
//         return true;
//     }
//     return false;
// }

// bool validateUIDLength(uint8_t uidLength) {
//     switch (uidLength) {
//         case 4:
//             ESP_LOGI(TAG, "4B UID detected");
//             break;
//         case 7:
//             ESP_LOGI(TAG, "7B UID detected");
//             break;
//         default:
//             ESP_LOGE(TAG, "Unknown UID type/length");
//             if (!led.isRunning()) LED_SET_SEQ(UNKNOWN_UID_TYPE);
//             return false;
//     }
//     return true;
// }

// void handleMasterCard(uint8_t* uid, uint8_t uidLength, AccessLoopState &state) {
//     unsigned long now = millis();

//     bool isSameCard = uidMatches(uid, uidLength, state.lastMasterUID, state.lastMasterUIDLen);

//     // New card or re-presented card
//     if (!state.masterPresent || !isSameCard || (now - state.masterLastSeen > 500)) {
//         memcpy(state.lastMasterUID, uid, uidLength);
//         state.lastMasterUIDLen = uidLength;
//         state.masterStartTime = now;
//         state.masterPresent = true;
//         LED_SET_SEQ(MASTER_CARD);
//         ESP_LOGI(TAG, "Master card detected - hold started");
//     }

//     state.masterLastSeen = now;

//     // Hold detection
//     if (state.masterPresent && (now - state.masterStartTime >= MASTER_HOLD_TIME)) {
//         ESP_LOGI(TAG, "Master hold confirmed (%us)", (MASTER_HOLD_TIME / 1000));

//         if (!led.isRunning() && !state.audioQueued) {
//             LED_SET_SEQ(PROGRAMMING_MODE);
//             //TODO fix sound
//             audio.playTrack(AudioContoller::SOUND_WAITING);


//             // state.queuedSound = AudioContoller::SOUND_ACCEPTED;
//             // state.audioQueued = true;

//             state.userProgrammingModeActive = true;
//             state.userProgLastActivityTime = millis();
//             state.userProgWarningGiven = false;

//             ESP_LOGI(TAG, "User Programming Mode Enabled");
//         }
//     }
// }


// // --- User Card Handler ---

// static void handleAccessGranted(AccessLoopState &state) {
//     ESP_LOGI(TAG, "Valid Card, Access Granted");
//     if (!led.isRunning() && !state.audioQueued) {
//         LED_SET_SEQ(ACCESS_GRANTED);
//         //TODO fix sound
//         state.queuedSound      = AudioContoller::SOUND_ACCEPTED;
//         state.audioQueued      = true;
//         // activateRelays();
//         RELAY_ACTION(UNLOCK);
//         state.invalidAttempts  = 0;
//         state.impatientEnabled = false;
//         state.impatient        = false;
//         ESP_LOGI(TAG, "Access Granted. Disabling Impatience Timer");
//     }
// }

// static void handleAccessDenied(AccessLoopState &state) {
//     uint32_t delayMs = (invalidDelays[state.invalidAttempts] * 1000) + 3000;
//     state.invalidTimeoutEnd = millis() + delayMs;

//     ESP_LOGW(TAG, "Invalid card attempt #%u, please wait %u seconds before trying again",
//         state.invalidAttempts + 1, delayMs / 1000);

//     if (!led.isRunning() && !state.audioQueued) {
//         LED_SET_SEQ(ACCESS_DENIED);
//         state.queuedSound = (state.invalidAttempts == 0) ? AudioContoller::SOUND_DENIED_1 :
//                              (state.invalidAttempts == 1) ? AudioContoller::SOUND_DENIED_2 :
//                                                             AudioContoller::SOUND_DENIED_3;
//         state.audioQueued = true;
//         if (state.invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) 
//             state.invalidAttempts++;
//         markUserActivity(state);
//     }
// }

// void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
//     bool validUID = rfid.validateUID(uid, uidLength);
//     if (validUID) {
//         handleAccessGranted(state);
//     } else {
//         handleAccessDenied(state);
//     }
// }

// // void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state) {
// //     bool validUID = rfid.validateUID(uid, uidLength);

// //     if (validUID) {
// //         ESP_LOGI(TAG, "Valid Card, Access Granted");
// //         if (!led.isRunning() && !state.audioQueued) {
// //             LED_SET_SEQ(ACCESS_GRANTED);
// //             state.queuedSound = AudioContoller::SOUND_ACCEPTED;
// //             state.audioQueued = true;
// //             activateRelays();
// //             state.invalidAttempts = 0;
// //             // Disable impatient timer after access granted
// //             ESP_LOGI(TAG, "Access Granted. Disabling Impatience Timer");
// //             // disableImpatience(state);
// //             state.impatientEnabled = false;  // stops the normal impatience loop
// //             state.impatient = false;         // reset any current impatience state
// //         }
// //     } else {
// //         uint32_t delayMs = (invalidDelays[state.invalidAttempts] * 1000) + 3000;
// //         state.invalidTimeoutEnd = millis() + delayMs;

// //         ESP_LOGW(TAG, "Invalid card attempt #%u, please wait %u seconds before trying again",
// //             state.invalidAttempts + 1, delayMs / 1000);

// //         if (!led.isRunning() && !state.audioQueued) {
// //             LED_SET_SEQ(ACCESS_DENIED);
// //             state.queuedSound = (state.invalidAttempts == 0) ? AudioContoller::SOUND_DENIED_1 :
// //                                  (state.invalidAttempts == 1) ? AudioContoller::SOUND_DENIED_2 :
// //                                                            AudioContoller::SOUND_DENIED_3;
// //             state.audioQueued = true;
// //             if (state.invalidAttempts < MAXIMUM_INVALID_ATTEMPTS - 1) state.invalidAttempts++;

// //             markUserActivity(state);
// //             // state.impatientEnabled = true;
// //         }
// //     }
// // }


// // --- Impatience Timeout ---
// // Currently working. Impatient at boot with known MasterUID, Impatient after unknown tag, impatient after short master card hold, Not impatient after access granted, impatience disabled during programming mode
// // Not working. markUserActivity blocks programming mode boot announcements
// void handleImpatienceTimer(AccessLoopState &state) {
// if (state.impatientEnabled &&
//     !state.impatient &&
//     millis() - state.lastActivityTime > IMPATIENCE_TIMEOUT)
// {
//     state.impatient = true;

//     ESP_LOGW(TAG, " %ds elapsed, notifying", IMPATIENCE_TIMEOUT / 1000);
//     LED_SET_SEQ(SYSTEM_IDLE);
//     audio.playTrack(AudioContoller::SOUND_WAITING);
// }
// }

// // Not working. markUserActivity blocks programming mode boot announcements
// void markUserActivity(AccessLoopState& state)
// {
//     state.lastActivityTime = millis();
//     state.impatientEnabled = true;
//     state.impatient = false;
//     ESP_LOGI(TAG, "FUNCTION: No User Interaction...Timer Starting...");
// }

// void disableImpatience(AccessLoopState& state)
// {
//     // Disable impatient timer while in programming mode
//     state.impatientEnabled = false;  // stops the normal impatience loop
//     state.impatient = false;         // reset any current impatience state
//     ESP_LOGI(TAG, "Disabling Impatience Timer...");
// }
