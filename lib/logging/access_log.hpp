#pragma once

#include <Preferences.h>
#include <esp_log.h>
#include <Arduino.h>
#include <time.h>
#include <string.h>

static const char* LOGTAG   = "LOG";
static const char* TIMETAG  = "TIME_TRACE";

#define LOG_NAMESPACE     "access_log"
#define LOG_SETTINGS_NS   "log_settings"
#define LOG_MAX_ENTRIES   50

enum class LogLevel  : uint8_t { ACCESS = 0, SYSTEM = 1, DEBUG = 2 };
enum class LogSource : uint8_t { RFID = 0, WIFI = 1, BLE = 2 };
enum class LogResult : uint8_t { FAIL = 0, SUCCESS = 1 };

// ======================================================
// ENTRY
// ======================================================

struct LogEntry {
    uint32_t timestamp;   // unix OR uptime
    uint8_t  timeMode;    // 0 = uptime, 1 = unix
    uint8_t  level;
    uint8_t  source;
    uint8_t  result;
    char identifier[32];
    char message[64];
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
        _syncUnix   = unixTime;
        _syncUptime = millis() / 1000;
        _timeSynced = true;

        Serial.println("\n================ TIME SYNC =================");
        Serial.print("RAW UNIX FROM DEVICE: ");
        Serial.println(unixTime);

        Serial.print("ESP UPTIME AT SYNC: ");
        Serial.println(_syncUptime);

        Serial.print("CALCULATED BOOT OFFSET: ");
        Serial.println(_syncUnix - _syncUptime);

        Serial.print("READABLE SYNC TIME: ");
        Serial.println((time_t)unixTime);
        Serial.println("===========================================\n");

        ESP_LOGI(TIMETAG,
            "SYNC unix=%lu uptime=%lu",
            (unsigned long)_syncUnix,
            (unsigned long)_syncUptime
        );

        if (_count > 0) {
            Serial.println("Backfilling mode=0 logs with absolute timestamps...");
            _backfillLogs();
        }
    }

    void _backfillLogs()
    {
        for (uint8_t i = 0; i < _count; i++)
        {
            if (_entries[i].timeMode == 0)
            {
                uint32_t uptimeAtLog = _entries[i].timestamp;
                _entries[i].timestamp = _syncUnix + (uptimeAtLog - _syncUptime);
                _entries[i].timeMode = 1;
                Serial.print("Backfilled log entry ");
                Serial.print(i);
                Serial.print(": ");
                Serial.println(_entries[i].timestamp);
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
        prefs.begin(LOG_SETTINGS_NS, true);

        _settings.enabled    = prefs.getBool("enabled", true);
        _settings.log_access = prefs.getBool("log_access", true);
        _settings.log_system = prefs.getBool("log_system", true);
        _settings.log_debug  = prefs.getBool("log_debug", false);

        prefs.end();

        loadEntries();
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
        if (!shouldLog(level))
            return;

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

        _head = (_head + 1) % LOG_MAX_ENTRIES;
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

    void logBoot()
    {
        if (_bootLogged) return;
        _bootLogged = true;

        logSystem(LogSource::RFID,
                  LogResult::SUCCESS,
                  "System",
                  "Device boot");
    }

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
        String json = "[";
        bool first = true;

        for (int i = _count - 1; i >= 0; i--)
        {
            LogEntry e;
            if (!getEntry(i, e)) continue;
            if (exactLevel >= 0 && e.level != exactLevel) continue;

            uint32_t ts = resolveTime(e);

            if (!first) json += ",";
            first = false;

            json += "{";
            json += "\"ts\":" + String(ts);
            json += ",\"mode\":" + String(e.timeMode);
            json += ",\"level\":" + String(e.level);
            json += ",\"source\":" + String(e.source);
            json += ",\"result\":" + String(e.result);
            json += ",\"id\":\"" + String(e.identifier) + "\"";
            json += ",\"msg\":\"" + String(e.message) + "\"";
            json += "}";
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

    bool getEntry(uint8_t index, LogEntry& out) const
    {
        if (index >= _count) return false;

        uint8_t actual =
            (_head + LOG_MAX_ENTRIES - _count + index) % LOG_MAX_ENTRIES;

        out = _entries[actual];
        return true;
    }

    // ======================================================
    // CLEAR
    // ======================================================

    void clear()
    {
        _head = 0;
        _count = 0;
        _bootLogged = false;

        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);
        prefs.clear();
        prefs.end();
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

        _count = prefs.getUChar("count", 0);
        if (_count > LOG_MAX_ENTRIES) _count = 0;

        for (uint8_t i = 0; i < _count; i++)
        {
            char key[8];
            snprintf(key, sizeof(key), "log_%u", i);
            prefs.getBytes(key, &_entries[i], sizeof(LogEntry));
        }

        _head = _count % LOG_MAX_ENTRIES;
        prefs.end();
    }

    void saveAllEntries()
    {
        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);

        for (uint8_t i = 0; i < _count; i++)
        {
            char key[8];
            snprintf(key, sizeof(key), "log_%u", i);
            prefs.putBytes(key, &_entries[i], sizeof(LogEntry));
        }

        prefs.putUChar("count", _count);
        prefs.end();
    }
};

extern AccessLogger accessLogger;
