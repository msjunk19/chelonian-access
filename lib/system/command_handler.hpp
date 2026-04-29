#pragma once

/**
 * @file command_handler.hpp
 * @brief Unified command handler for all input sources
 * 
 * This abstraction allows WiFi, BLE, RFID, and other sources to trigger
 * the same actions without duplicating code.
 */

#include <functional>

#include "macro_config.hpp"
#include "auth_manager.hpp"
#include "macro_executor.hpp"

extern MacroConfigManager macroConfigManager;

enum class CommandSource {
    WIFI,
    BLE,
    RFID,
    PAIRING
};

class CommandHandler {
public:
    using CommandCallback = std::function<void(PhoneCommand cmd, CommandSource source)>;

    static CommandHandler& getInstance() {
        static CommandHandler instance;
        return instance;
    }

    void setCallback(CommandCallback callback) {
        m_callback = callback;
    }

    void executeCommand(PhoneCommand cmd, CommandSource source) {
        if (m_callback) {
            m_callback(cmd, source);
        }
    }

private:
    CommandHandler() = default;
    CommandCallback m_callback;
};

/**
 * @brief Helper to execute a command and trigger the associated macro
 * @param cmd The command to execute
 * @param source Where the command originated
 * @param ledSequence Optional LED sequence to display
 */
inline void executeAccessCommand(
    PhoneCommand cmd,
    CommandSource source,
    std::function<void()> ledSequence = nullptr
) {
    // Optional LED feedback
    if (ledSequence) {
        ledSequence();
    }

    // Find and execute macro
    int8_t idx = macroConfigManager.findByName(
        cmd == PhoneCommand::UNLOCK ? "Unlock" :
        cmd == PhoneCommand::LOCK ? "Lock" :
        cmd == PhoneCommand::TRUNK ? "Trunk" :
        cmd == PhoneCommand::PANIC ? "Panic" : ""
    );

    if (idx >= 0) {
        fireMacro(idx);
    }

    // Log access
    const char* sourceStr =
        source == CommandSource::WIFI ? "WiFi" :
        source == CommandSource::BLE ? "BLE" :
        source == CommandSource::RFID ? "RFID" :
        source == CommandSource::PAIRING ? "Pairing" : "Unknown";

    const char* cmdStr =
        cmd == PhoneCommand::UNLOCK ? "Unlock" :
        cmd == PhoneCommand::LOCK ? "Lock" :
        cmd == PhoneCommand::TRUNK ? "Trunk" :
        cmd == PhoneCommand::PANIC ? "Panic" : "Status";

    // accessLogger.logAccess(sourceToLogSource(source), LogResult::SUCCESS, sourceStr, cmdStr);
}