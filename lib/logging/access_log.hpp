#pragma once
#include <Preferences.h>
#include <esp_log.h>
#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

static const char* LOGTAG = "LOG";

#define LOG_NAMESPACE "access_log"
#define LOG_SETTINGS_NS "log_settings"
#define LOG_MAX_ENTRIES 50
#define LOG_MAX_HOURS 48

enum class LogLevel : uint8_t { ACCESS = 0, SYSTEM = 1, DEBUG = 2 };
enum class LogSource : uint8_t { RFID = 0, WIFI = 1, BLE = 2 };
enum class LogResult : uint8_t { FAIL = 0, SUCCESS = 1 };

struct LogEntry {
    uint32_t timestamp;
    uint8_t level;
    uint8_t source;
    uint8_t result;
    char identifier[32];
    char message[64];
};

struct LoggingSettings {
    bool enabled;
    bool log_access;
    bool log_system;
    bool log_debug;
};

class AccessLogger {
public:
    AccessLogger() : _head(0), _count(0), _initialized(false), _lastMillis(0), _uptimeOffset(0), _timeOffset(0) {}

    void begin() {
        Preferences prefs;
        prefs.begin(LOG_SETTINGS_NS, true);
        _settings.enabled = prefs.getBool("enabled", true);
        _settings.log_access = prefs.getBool("log_access", true);
        _settings.log_system = prefs.getBool("log_system", true);
        _settings.log_debug = prefs.getBool("log_debug", false);
        prefs.end();

        loadEntries();
        
        prefs.begin(LOG_NAMESPACE, true);
        _lastMillis = prefs.getUInt("last_millis", 0);
        _uptimeOffset = prefs.getUInt("uptime_off", 0);
        _timeOffset = prefs.getUInt("time_off", 0);
        prefs.end();
        
        uint32_t currentMillis = millis();
        if (_lastMillis != 0 && currentMillis < _lastMillis) {
            _uptimeOffset += _lastMillis;
        }
        
        if (LOG_MAX_HOURS > 0) {
            purgeOldEntries();
        }

        _initialized = true;
    }

    void setTimeOffset(uint32_t offset) {
        _timeOffset = offset;
        saveTimeOffset();
    }

    void updateSettings(const LoggingSettings& settings) {
        _settings = settings;
        saveSettings();
    }

    void log(LogLevel level, LogSource source, LogResult result, const char* identifier, const char* message) {
        if (!shouldLog(level)) {
            ESP_LOGI(LOGTAG, "log() skipped - level %d not enabled", (int)level);
            return;
        }

        uint32_t timestamp = (uint32_t)time(nullptr);
        if (timestamp < 1000000000) { // If time not set (before 2001), use uptime
            timestamp = _uptimeOffset + millis() / 1000;
        }
        
        LogEntry& entry = _entries[_head];
        entry.timestamp = timestamp;
        entry.level = static_cast<uint8_t>(level);
        entry.source = static_cast<uint8_t>(source);
        entry.result = static_cast<uint8_t>(result);
        strncpy(entry.identifier, identifier, sizeof(entry.identifier) - 1);
        entry.identifier[sizeof(entry.identifier) - 1] = '\0';
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';

        _head = (_head + 1) % LOG_MAX_ENTRIES;
        if (_count < LOG_MAX_ENTRIES) _count++;

        saveAllEntries();
    }

    void logAccess(LogSource source, LogResult result, const char* identifier, const char* message) {
        log(LogLevel::ACCESS, source, result, identifier, message);
    }

    bool shouldLog(LogLevel level) const {
        if (!_settings.enabled) return false;
        switch (level) {
            case LogLevel::ACCESS: return _settings.log_access;
            case LogLevel::SYSTEM: return _settings.log_system;
            case LogLevel::DEBUG: return _settings.log_debug;
        }
        return false;
    }

    bool getEntry(uint8_t index, LogEntry& entry) const {
        if (index >= _count) return false;
        uint8_t actualIndex = (_head + LOG_MAX_ENTRIES - _count + index) % LOG_MAX_ENTRIES;
        entry = _entries[actualIndex];
        return true;
    }

    uint8_t getCount() const { return _count; }

    String getLogsJson(uint8_t minLevel = 0, size_t maxLen = 0) const {
        String json = "[";
        bool first = true;
        // Reverse: most recent first
        for (int i = _count - 1; i >= 0; i--) {
            LogEntry entry;
            if (!getEntry(i, entry)) continue;
            if (entry.level < minLevel) continue;
            
            if (!first) json += ",";
            first = false;
            json += "{";
            json += "\"ts\":" + String(entry.timestamp);
            json += ",\"level\":" + String(entry.level);
            json += ",\"source\":" + String(entry.source);
            json += ",\"result\":" + String(entry.result);
            json += ",\"id\":\"" + String(entry.identifier) + "\"";
            json += ",\"msg\":\"" + String(entry.message) + "\"";
            json += "}";
            
            if (maxLen > 0 && json.length() > maxLen) {
                json = json.substring(0, maxLen);
                int lastComma = json.lastIndexOf(",{");
                if (lastComma > 0) {
                    json = json.substring(0, lastComma) + "]";
                }
                break;
            }
        }
        if (!json.endsWith("]")) json += "]";
        return json;
    }

    const LoggingSettings& getSettings() const { return _settings; }

    void clear() {
        _head = 0;
        _count = 0;
        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);
        prefs.clear();
        prefs.end();
        _lastMillis = 0;
        _uptimeOffset = 0;
        _timeOffset = 0;
    }

private:
    LogEntry _entries[LOG_MAX_ENTRIES];
    uint8_t _head;
    uint8_t _count;
    bool _initialized;
    uint32_t _lastMillis;
    uint32_t _uptimeOffset;
    uint32_t _timeOffset;  // offset from uptime to real Unix time
    LoggingSettings _settings;

    void loadEntries() {
        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, true);
        _count = prefs.getUChar("count", 0);
        if (_count > LOG_MAX_ENTRIES) _count = 0;
        
        for (uint8_t i = 0; i < _count; i++) {
            char key[8];
            snprintf(key, sizeof(key), "log_%u", i);
            size_t read = prefs.getBytes(key, &_entries[i], sizeof(LogEntry));
            if (read != sizeof(LogEntry)) {
                _count = 0;
                break;
            }
        }
        _head = _count % LOG_MAX_ENTRIES;
        prefs.end();
    }

    void saveAllEntries() {
        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);
        for (uint8_t i = 0; i < _count; i++) {
            char key[8];
            snprintf(key, sizeof(key), "log_%u", i);
            prefs.putBytes(key, &_entries[i], sizeof(LogEntry));
        }
        prefs.putUChar("count", _count);
        prefs.putUInt("last_millis", _lastMillis);
        prefs.putUInt("uptime_off", _uptimeOffset);
        prefs.end();
    }

    void saveSettings() {
        Preferences prefs;
        prefs.begin(LOG_SETTINGS_NS, false);
        prefs.putBool("enabled", _settings.enabled);
        prefs.putBool("log_access", _settings.log_access);
        prefs.putBool("log_system", _settings.log_system);
        prefs.putBool("log_debug", _settings.log_debug);
        prefs.end();
    }

    void saveTimeOffset() {
        Preferences prefs;
        prefs.begin(LOG_NAMESPACE, false);
        prefs.putUInt("time_off", _timeOffset);
        prefs.end();
    }

    void purgeOldEntries() {
        uint32_t maxAgeMs = (uint32_t)LOG_MAX_HOURS * 3600UL * 1000UL;
        uint8_t removed = 0;
        uint32_t currentUptime = _uptimeOffset + millis();
        
        for (uint8_t i = 0; i < _count; i++) {
            uint32_t age = (currentUptime >= _entries[i].timestamp) ? (currentUptime - _entries[i].timestamp) : 0;
            if (age > maxAgeMs) {
                removed++;
            } else if (removed > 0) {
                _entries[i - removed] = _entries[i];
            }
        }
        
        if (removed > 0) {
            _count -= removed;
            _head = _count % LOG_MAX_ENTRIES;
            saveAllEntries();
        }
    }
};

extern AccessLogger accessLogger;
