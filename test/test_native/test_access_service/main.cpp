#include <test_helpers.h>
#include <unity.h>
#include "test_access_service.h"

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(testInitialState);
    RUN_TEST(testMacroTrigger);
    RUN_TEST(testMacroAlreadyActive);
    RUN_TEST(testImpatientWaiting);
    RUN_TEST(testImpatientDisabled);
    RUN_TEST(testMarkUserActivityResetsImpatient);
    RUN_TEST(testInvalidCardDelays);
    RUN_TEST(testInvalidAttemptsCapped);
    RUN_TEST(testRelayStateTransitions);
    RUN_TEST(testMasterCardHoldDetection);
    RUN_TEST(testUIDValidation);

    return UNITY_END();
}
