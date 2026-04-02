#include "test_access_service.h"
#include <access_service.h>
#include <access_state.hpp>
#include <test_helpers.h>
#include <macro_config.hpp>
#include <unity.h>

extern millis_t mockMillis;

static AccessLoopState testState;

void setUpTestState() {
    memset(&testState, 0, sizeof(testState));
    testState.impatientEnabled = true;
    testState.lastActivityTime = mockMillis;
}

void testInitialState() {
    setUpTestState();
    
    TEST_ASSERT_FALSE(testState.scanned);
    TEST_ASSERT_FALSE(testState.audioQueued);
    TEST_ASSERT_EQUAL(0, testState.invalidAttempts);
    TEST_ASSERT_EQUAL(RELAY_IDLE, testState.currentRelayState);
    TEST_ASSERT_FALSE(testState.relayActive);
    TEST_ASSERT_FALSE(testState.impatient);
    TEST_ASSERT_FALSE(testState.userProgrammingModeActive);
}

void testMacroTrigger() {
    setUpTestState();
    
    TEST_ASSERT_FALSE(testState.relayActive);
    
    Macro& m = macroConfigManager.config.macros[macroConfigManager.config.tag_macro];
    TEST_ASSERT_TRUE(m.step_count > 0);
    
    ::fireMacro(testState, macroConfigManager.config.tag_macro);
    
    TEST_ASSERT_TRUE(testState.relayActive);
    TEST_ASSERT_EQUAL(RELAY_STEP_ACTIVE, testState.currentRelayState);
    TEST_ASSERT_EQUAL(0, testState.currentStep);
}

void testMacroAlreadyActive() {
    setUpTestState();
    testState.relayActive = true;
    
    ::fireMacro(testState, 0);
    
    TEST_ASSERT_TRUE(testState.relayActive);
}

void testImpatientWaiting() {
    setUpTestState();
    testState.impatientEnabled = true;
    testState.lastActivityTime = mockMillis;
    testState.impatient = false;
    
    TEST_ASSERT_FALSE(testState.impatient);
    
    mockMillis = 10001;
    handleImpatienceTimer(testState);
    
    TEST_ASSERT_TRUE(testState.impatient);
}

void testImpatientDisabled() {
    setUpTestState();
    testState.impatientEnabled = false;
    testState.lastActivityTime = mockMillis;
    testState.impatient = false;
    
    mockMillis = 10001;
    handleImpatienceTimer(testState);
    
    TEST_ASSERT_FALSE(testState.impatient);
}

void testMarkUserActivityResetsImpatient() {
    setUpTestState();
    testState.impatient = true;
    testState.lastActivityTime = 0;
    
    markUserActivity(testState);
    
    TEST_ASSERT_FALSE(testState.impatient);
    TEST_ASSERT_TRUE(testState.lastActivityTime > 0);
}

void testInvalidCardDelays() {
    setUpTestState();
    extern const uint8_t invalidDelays[MAXIMUM_INVALID_ATTEMPTS];
    
    testState.invalidAttempts = 0;
    handleAccessDenied(testState);
    
    TEST_ASSERT_EQUAL(1, testState.invalidAttempts);
    TEST_ASSERT_TRUE(testState.invalidTimeoutEnd > mockMillis);
    
    mockMillis = testState.invalidTimeoutEnd + 1;
    handleAccessDenied(testState);
    TEST_ASSERT_EQUAL(2, testState.invalidAttempts);
}

void testInvalidAttemptsCapped() {
    setUpTestState();
    testState.invalidAttempts = MAXIMUM_INVALID_ATTEMPTS - 1;
    
    handleAccessDenied(testState);
    
    TEST_ASSERT_EQUAL(MAXIMUM_INVALID_ATTEMPTS - 1, testState.invalidAttempts);
}

void testRelayStateTransitions() {
    setUpTestState();
    
    ::fireMacro(testState, 0);
    TEST_ASSERT_EQUAL(RELAY_STEP_ACTIVE, testState.currentRelayState);
    
    RelayStep& step = testState.activeMacro.steps[0];
    mockMillis += step.duration + 1;
    handleRelaySequence(testState);
    
    if (testState.activeMacro.step_count > 1) {
        TEST_ASSERT_EQUAL(RELAY_STEP_GAP, testState.currentRelayState);
        
        mockMillis += step.gap + 1;
        handleRelaySequence(testState);
        TEST_ASSERT_EQUAL(RELAY_STEP_ACTIVE, testState.currentRelayState);
    } else {
        TEST_ASSERT_EQUAL(RELAY_IDLE, testState.currentRelayState);
        TEST_ASSERT_FALSE(testState.relayActive);
    }
}

void testMasterCardHoldDetection() {
    setUpTestState();
    uint8_t uid[4] = {0x12, 0x34, 0x56, 0x78};
    
    testState.masterPresent = false;
    testState.lastActivityTime = mockMillis;
    
    handleMasterCard(uid, 4, testState);
    
    TEST_ASSERT_TRUE(testState.masterPresent);
    TEST_ASSERT_TRUE(testState.masterStartTime > 0);
}

void testUIDValidation() {
    TEST_ASSERT_TRUE(validateUIDLength(4));
    TEST_ASSERT_TRUE(validateUIDLength(7));
    TEST_ASSERT_FALSE(validateUIDLength(3));
    TEST_ASSERT_FALSE(validateUIDLength(8));
}
