#pragma once
#include <master_uid_manager.h>
#include <user_uid_manager.h>
#include <led_controller.h>
#include <macro_config.hpp>
#include <relay_controller.h>
#include <audio_controller.h>

// Declare the global object (no memory allocated here)
extern MasterUIDManager masterUidManager;
extern UserUIDManager userUidManager;

extern LEDController led;  // declaration only
extern MacroConfigManager macroConfigManager;

extern RFIDController rfid;
extern RelayController relays;
extern AudioContoller audio;
