#pragma once
#include <master_uid_manager.h>
#include <user_uid_manager.h>
#include <led_controller.h>
#include <macro_config.hpp>

// Declare the global object (no memory allocated here)
extern MasterUIDManager masterUidManager;
extern UserUIDManager userUidManager;

extern LEDController led;  // declaration only
extern MacroConfigManager macroConfigManager;
