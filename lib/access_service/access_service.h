#include <stdint.h>
#include <audio_controller.h>
#include <relay_controller.h>
#include <rfid_controller.h>
#include <config.hpp>
#include <relay_config_manager.hpp>

// State and configuration
extern RFIDController rfid;
extern RelayController relays;
extern AudioContoller audio;
extern RelayConfigManager relayConfigManager;
extern RelayAction activeRelayAction;

// Relay state enum
// enum RelayState { RELAY_IDLE, RELAY1_ACTIVE, RELAY2_PENDING, RELAY2_ACTIVE };

// Remove old RelayState, replace with:
enum RelayState {
    RELAY_IDLE,
    RELAY_SINGLE_ACTIVE,
    RELAY_DOUBLE_PULSE1,
    RELAY_DOUBLE_GAP,
    RELAY_DOUBLE_PULSE2,
    RELAY_SEQUENTIAL_1,
    RELAY_SEQUENTIAL_2,
    RELAY_ALL_ACTIVE
};

// Active action tracking — needed by handleRelaySequence
extern RelayAction activeRelayAction;

// --- New: encapsulated state for the loop ---
struct AccessLoopState {
    // Master card state
    uint8_t lastMasterUID[7] = {0};
    uint8_t lastMasterUIDLen = 0;
    bool masterPresent = false;
    unsigned long masterStartTime = 0;
    unsigned long masterLastSeen = 0;

    // Scan + audio
    bool scanned = false;
    bool audioQueued = false;
    uint8_t queuedSound = 0;
    // // Add this somewhere in your class or global state
    // unsigned long audioStartupTime = 0;
    // bool audioStartupPlayed = false;
    // bool audioStartupScheduled = false;
    // bool audioInitialized = false;

// static unsigned long audioStartupTime = 0;
// static bool audioStartupScheduled = false;

    // Timing
    unsigned long startTime = 0;
    unsigned long lastActivityTime = 0;

    // Impatience system
    bool impatient = false;
    bool impatientEnabled = true;

    // Invalid handling
    uint8_t invalidAttempts = 0;
    unsigned long invalidTimeoutEnd = 0;

    // Relay state
    RelayState currentRelayState = RELAY_IDLE;
    unsigned long relayActivatedTime = 0;
    bool relayActive = false;

    // Programming mode
    bool userProgrammingModeActive = false;
    unsigned long userProgLastActivityTime = 0;
    bool userProgWarningGiven = false;
};


constexpr uint8_t MAXIMUM_INVALID_ATTEMPTS = 13;
extern const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS];

// Service logic
void accessServiceSetup();
void accessServiceLoop();
void handleRelaySequence();
void activateRelays();

// --- Helper functions ---
bool handleMasterPresenceTimeout(AccessLoopState &state);
bool validateUIDLength(uint8_t uidLength);
void handleMasterCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state);
void handleRegularCard(uint8_t *uid, uint8_t uidLength, AccessLoopState &state);
void handleImpatienceTimer(AccessLoopState &state);
void markUserActivity(AccessLoopState &state);
void disableImpatience(AccessLoopState &state);
