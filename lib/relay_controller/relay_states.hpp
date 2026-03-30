#pragma once
#include <relay_config_manager.hpp>

extern RelayConfigManager relayConfigManager;

// Trigger a named relay action
#define RELAY_ACTION(action) triggerRelayAction(RelayAction::action)

// Forward declaration
void triggerRelayAction(RelayAction action);