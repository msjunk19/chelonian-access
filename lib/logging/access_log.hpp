#pragma once

#include <time.h>
#include <string.h>
#include <esp_log.h>
#include <Arduino.h>
#include <Preferences.h>


static const char* LOGTAG   = "LOG";
static const char* TIMETAG  = "TIME_TRACE";

static int totalLogCalls = 0;

#define LOG_NAMESPACE     "access_log"
#define LOG_SETTINGS_NS   "log_settings"
#define LOG_MAX_ENTRIES   50

enum class LogLevel  : uint8_t { ACCESS = 0, SYSTEM = 1, DEBUG = 2 };
enum class LogSource : uint8_t { RFID = 0, WIFI = 1, BLE = 2, SYSTEM = 3, PROXIMITY = 4};
enum class LogResult : uint8_t { FAIL = 0, SUCCESS = 1 };

// ======================================================
// ENTRY
// ======================================================

struct LogEntry {
    uint32_t timestamp;
    uint8_t  timeMode;
    uint8_t  level;
    uint8_t  source;
    uint8_t  result;
    char identifier[32];
    char message[64];

    LogEntry() {
        timestamp = 0;
        timeMode = 0;
        level = 0;
        source = 0;
        result = 0;
        identifier[0] = '\0';
        message[0] = '\0';
    }
};

// ======================================================
// SETTINGS
// ======================================================

struct LoggingSettings {
    bool enabled;
    bool log_access;
    bool log_system;
    bool log_debug;
};

// ======================================================
// LOGGER
// ======================================================

class AccessLogger {
public:
    AccessLogger()
        : _head(0),
          _count(0),
          _bootLogged(false),
          _timeSynced(false),
          _syncUnix(0),
          _syncUptime(0) {}

    // ======================================================
    // TIME SYNC (FROM PHONE COMMANDS ONLY)
    // ======================================================

    void setSystemTime(uint32_t unixTime)
    {
        if (_timeSynced) return;  // Don't backfill if already synced
    
        _syncUnix   = unixTime;
        _syncUptime = millis() / 1000;
        _timeSynced = true;

        // Serial.println("\n================ TIME SYNC =================");
        // Serial.print("RAW UNIX FROM DEVICE: ");
        // Serial.println(unixTime);

        // Serial.print("ESP UPTIME AT SYNC: ");
        // Serial.println(_syncUptime);

        // Serial.print("CALCULATED BOOT OFFSET: ");
        // Serial.println(_syncUnix - _syncUptime);

        // Serial.print("READABLE SYNC TIME: ");
        // Serial.println((time_t)unixTime);
        // Serial.println("===========================================\n");

        ESP_LOGI(TIMETAG,
            "SYNC unix=%lu uptime=%lu",
            (unsigned long)_syncUnix,
            (unsigned long)_syncUptime
        );

        if (_count > 0) {
            // Serial.println("Backfilling mode=0 logs with absolute timestamps...");
            _backfillLogs();
        }
    }

    void _backfillLogs()
    {
        for (uint8_t i = 0; i < _count; i++)
        {
            LogEntry& e = getEntryAtIndex(i);
            if (e.timeMode == 0)
            {
                uint32_t uptimeAtLog = e.timestamp;
                e.timestamp = _syncUnix + (uptimeAtLog - _syncUptime);
                e.timeMode = 1;
                // Serial.print("Backfilled log entry ");
                // Serial.print(i);
                // Serial.print(": ");
                // Serial.println(e.timestamp);
            }
        }
        saveAllEntries();
    }

    // ======================================================
    // INIT
    // ======================================================

    void begin()
    {
        Preferences prefs;
    
        // First ensure namespace exists with defaults
        prefs.begin(LOG_SETTINGS_NS, false);
        prefs.putBool("enabled", true);
        prefs.putBool("log_access", true);
        prefs.putBool("log_system", true);
        prefs.putBool("log_debug", false);
        prefs.end();
        
        // Then read
        prefs.begin(LOG_SETTINGS_NS, true);

        _settings.enabled    = prefs.getBool("enabled", true);
        _settings.log_access = prefs.getBool("log_access", true);
        _settings.log_system = prefs.getBool("log_system", true);
        _settings.log_debug  = prefs.getBool("log_debug", false);

        prefs.end();

        loadEntries();

        for (int i = 0; i < LOG_MAX_ENTRIES; i++) {
            if (_entries[i].identifier[0] != '\0') {
                // Serial.print("Array slot ");
                // Serial.print(i);
                // Serial.print(": ");
                // Serial.println(_entries[i].identifier);
            }
        }

        _bootLogged = false;
    }

    // ======================================================
    // CORE LOG
    // ======================================================

    void log(LogLevel level,
             LogSource source,
             LogResult result,
             const char* identifier,
             const char* message,
             int32_t externalTimestamp = -1)
    {
        totalLogCalls++;
        // Serial.print("log() call #");
        // Serial.println(totalLogCalls);

        // Serial.print("[");
        // Serial.print(millis());
        // Serial.print("] LOG: id=");
        // Serial.print(identifier);
        // Serial.print(" msg=");
        // Serial.println(message);

        if (!shouldLog(level))
            return;

        // Get reference to the entry at _head (overwrites oldest when full)
        LogEntry& e = _entries[_head];

        uint32_t uptime = millis() / 1000;

        // ---------------- TIME RULE ----------------
        if (externalTimestamp > 1000000000)
        {
            // ALWAYS trust external timestamp if provided
            e.timestamp = (uint32_t)externalTimestamp;
            e.timeMode  = 1;

            // ALSO update sync anchor (important fix)
            setSystemTime(externalTimestamp);
        }
        else
        {
            if (_timeSynced)
            {
                // convert uptime into unix space
                e.timestamp = _syncUnix + (uptime - _syncUptime);
                e.timeMode  = 1;
            }
            else
            {
                e.timestamp = uptime;
                e.timeMode  = 0;
            }
        }

        e.level  = (uint8_t)level;
        e.source = (uint8_t)source;
        e.result = (uint8_t)result;

        strncpy(e.identifier, identifier, sizeof(e.identifier) - 1);
        e.identifier[31] = '\0';

        strncpy(e.message, message, sizeof(e.message) - 1);
        e.message[63] = '\0';

        // Advance head pointer
        _head = (_head + 1) % LOG_MAX_ENTRIES;
        
        // Increment count only until we reach max capacity
        if (_count < LOG_MAX_ENTRIES)
            _count++;

        saveAllEntries();
    }

    // ======================================================
    // RESTORED API
    // ======================================================

    void logAccess(LogSource s, LogResult r, const char* id, const char* msg)
        { log(LogLevel::ACCESS, s, r, id, msg); }

    void logSystem(LogSource s, LogResult r, const char* id, const char* msg)
        { log(LogLevel::SYSTEM, s, r, id, msg); }

    void logDebug(LogSource s, LogResult r, const char* id, const char* msg)
        { log(LogLevel::DEBUG, s, r, id, msg); }

    // ======================================================
    // TIME RESOLVE (FIXED DISPLAY LOGIC)
    // ======================================================

    uint32_t resolveTime(const LogEntry& e) const
    {
        if (e.timeMode == 1)
            return e.timestamp;

        if (!_timeSynced)
            return e.timestamp;

        return _syncUnix + (e.timestamp - _syncUptime);
    }

    // ======================================================
    // JSON OUTPUT
    // ======================================================

    String getLogsJson(int8_t exactLevel,
                       uint32_t /*clientUnixTime*/,
                       size_t maxLen = 0) const
    {
        return getLogsJsonChunk(exactLevel, 0, maxLen);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIXED: getLogsJsonChunk - Now properly handles offset and maxCount
    // ═══════════════════════════════════════════════════════════════════════
    String getLogsJsonChunk(int8_t exactLevel, size_t offset, size_t maxCount) const
    {
        String json = "[";
        bool first = true;
        size_t skipped = 0;      // Count entries we've skipped
        size_t added = 0;         // Count entries we've actually added

        // Iterate from newest to oldest
        for (int i = 0; i < _count; i++)
        {
            LogEntry e;
            if (!getEntry(i, e)) continue;
            if (exactLevel >= 0 && e.level != exactLevel) continue;

            // Skip first 'offset' entries
            if (skipped < offset) {
                skipped++;
                continue;
            }

            // Stop after 'maxCount' entries (if maxCount > 0)
            if (maxCount > 0 && added >= maxCount) {
                break;
            }

            uint32_t ts = resolveTime(e);

            if (!first) json += ",";
            first = false;

            json += "{\"ts\":" + String(ts);
            json += ",\"mode\":" + String(e.timeMode);
            json += ",\"level\":" + String(e.level);
            json += ",\"source\":" + String(e.source);
            json += ",\"result\":" + String(e.result);
            json += ",\"id\":\"" + String(e.identifier) + "\"";
            json += ",\"msg\":\"" + String(e.message) + "\"";
            json += "}";

            added++;
        }

        json += "]";
        return json;
    }

    // ======================================================
    // SETTINGS
    // ======================================================

    bool shouldLog(LogLevel level) const
    {
        if (!_settings.enabled) return false;

        switch (level)
        {
            case LogLevel::ACCESS: return _settings.log_access;
            case LogLevel::SYSTEM: return _settings.log_system;
            case LogLevel::DEBUG:  return _settings.log_debug;
        }
        return false;
    }

    const LoggingSettings& getSettings() const { return _settings; }
    uint16_t getCount() const { return _count; }
    uint8_t getHead() const { return _head; }

    bool getEntry(int8_t index, LogEntry& out) const
    {
        if (index < 0 || index >= _count) return false;

        // Calculate actual position in circular buffer
        // index 0 = newest (most recent)
        // index _count-1 = oldest
        uint8_t actual = (_head + LOG_MAX_ENTRIES - _count + index) % LOG_MAX_ENTRIES;

        out = _entries[actual];
        return true;
    }

    // Direct access to entry by circular position (for internal use)
    LogEntry& getEntryAtIndex(uint8_t logicalIndex)
    {
        uint8_t actual = (_head + LOG_MAX_ENTRIES - _count + logicalIndex) % LOG_MAX_ENTRIES;
        return _entries[actual];
    }

    // ======================================================
    // CLEAR
    // ======================================================

    void clear()
    {
        _head = 0;
        _count = 0;
        _bootLogged = false;

        // Clear entries from memory
        for (uint8_t i = 0; i < LOG_MAX_ENTRIES; i++) {
            _entries[i] = LogEntry();
        }

        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);
        prefs.clear();
        prefs.end();

        Serial.println("Logs cleared");
    }

private:

    LogEntry _entries[LOG_MAX_ENTRIES];
    uint8_t _head;
    uint8_t _count;

    LoggingSettings _settings;

    bool _bootLogged;
    bool _timeSynced;

    uint32_t _syncUnix;
    uint32_t _syncUptime;

    void loadEntries()
    {
        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, true);

        // Load circular buffer state
        _head = prefs.getUChar("head", 0);
        _count = prefs.getUChar("count", 0);
        
        // Serial.print("LOAD: _head=");
        // Serial.print(_head);
        // Serial.print(" _count=");
        // Serial.println(_count);
        
        // Load all entries from storage (full array)
        for (uint8_t i = 0; i < LOG_MAX_ENTRIES; i++)
        {
            char key[8];
            snprintf(key, sizeof(key), "log_%u", i);
            prefs.getBytes(key, &_entries[i], sizeof(LogEntry));
        }

        prefs.end();
    }

    void saveAllEntries()
    {
        // Serial.print("SAVE: _head=");
        // Serial.print(_head);
        // Serial.print(" _count=");
        // Serial.println(_count);

        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);

        // Save circular buffer state
        prefs.putUChar("head", _head);
        prefs.putUChar("count", _count);

        // Save all entries (full array)
        for (uint8_t i = 0; i < LOG_MAX_ENTRIES; i++)
        {
            char key[8];
            snprintf(key, sizeof(key), "log_%u", i);
            prefs.putBytes(key, &_entries[i], sizeof(LogEntry));
        }

        prefs.end();
    }
};

extern AccessLogger accessLogger;