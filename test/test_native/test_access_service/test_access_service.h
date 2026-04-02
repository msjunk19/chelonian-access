#ifndef TEST_ACCESS_SERVICE_H
#define TEST_ACCESS_SERVICE_H

#include <unity.h>
#include <access_state.hpp>

void testInitialState();
void testMacroTrigger();
void testImpatientWaiting();
void testInvalidCardDelays();
void testRelayStateTransitions();

#endif  // TEST_ACCESS_SERVICE_H
