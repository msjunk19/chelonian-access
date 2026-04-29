#pragma once

#include "led_controller.h"
#include "macro_config.hpp"
#include "user_uid_manager.h"
#include "relay_controller.h"
#include "audio_controller.h"
#include "master_uid_manager.h"

// Declare the global object (no memory allocated here)
extern LEDController led;  // declaration only
extern MacroConfigManager macroConfigManager;

extern RFIDController rfid;
extern RelayController relays;
extern AudioContoller audio;

extern UserUIDManager userUidManager;
extern MasterUIDManager masterUidManager;

