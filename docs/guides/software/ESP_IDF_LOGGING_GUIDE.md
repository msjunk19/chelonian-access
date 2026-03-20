# ESP-IDF Logging Implementation Guide

## Overview
This document explains how to use the ESP-IDF logging system implemented in the Chelonian Access project. The logging system provides structured, leveled logging with runtime control capabilities.

## Key Features
- Five log severity levels (Error, Warn, Info, Debug, Verbose)
- Per-module log level control
- Millisecond timestamps in all logs
- Color-coded output in PlatformIO terminal
- Compile-time log level configuration

## Basic Usage

### 1. Include the Header
```cpp
#include "esp_log.h"
```

### 2. Define a Tag
Each source file should define its own tag:
```cpp
static const char* TAG = "ModuleName";
```

### 3. Logging Macros
Use these macros based on message severity:

| Macro        | Level  | Description                          |
|--------------|--------|--------------------------------------|
| `ESP_LOGE()` | Error  | Critical errors that need attention  |
| `ESP_LOGW()` | Warn   | Potential issues                     |
| `ESP_LOGE()` | Info   | Normal operational messages          |
| `ESP_LOGD()` | Debug  | Debug information                    |
| `ESP_LOGV()` | Verbose| Very detailed tracing                |

Example:
```cpp
ESP_LOGE(TAG, "System initialized. Free heap: %d bytes", esp_get_free_heap_size());
```

## Configuration

### PlatformIO Settings
The logging behavior is configured in `platformio.ini`:

```ini
build_flags =
    -DCONFIG_LOG_DEFAULT_LEVEL=3   ; Default level (INFO)
    -DCONFIG_LOG_MAXIMUM_LEVEL=5   ; Maximum level (VERBOSE)
    -DCONFIG_LOG_DYNAMIC_LEVEL_CONTROL=1 ; Enable runtime level changes
```

### Runtime Level Control
Change log levels at runtime:
```cpp
// Set module-specific level
esp_log_level_set("ModuleName", ESP_LOG_WARN);

// Set default level for all modules
esp_log_level_set("*", ESP_LOG_INFO);
```

## Best Practices

1. **Tag Naming**: Use short but descriptive module names (e.g., "Main", "Audio", "RFID")

2. **Log Levels**:
   - ERROR: Critical failures that affect operation
   - WARN: Unexpected but recoverable conditions
   - INFO: Important operational messages
   - DEBUG: Development debugging info
   - VERBOSE: Very detailed tracing

## Troubleshooting

**Logs not appearing?**
1. Check `CONFIG_LOG_DEFAULT_LEVEL` in platformio.ini
2. Verify no runtime level override exists
3. Ensure monitor speed matches Serial.begin() rate (115200)
4. Check CDC Setting (some esp32c3)

**Changing Log Levels**
View current levels:
```cpp
esp_log_level_t level = esp_log_level_get("ModuleName");
```

Set new level:
```cpp
esp_log_level_set("ModuleName", ESP_LOG_DEBUG);
```

## Production Considerations
For production builds, consider:
- Setting `CONFIG_LOG_DEFAULT_LEVEL=1` (ERROR only)
- Disabling `CONFIG_LOG_DYNAMIC_LEVEL_CONTROL` to save space
- Removing verbose/debug logs completely
