#pragma once
#include <Preferences.h>
#include "esp_log.h"
#include <pin_mapping.hpp>

static const char* MACROTAG    = "MACRO";
static const char* MACRO_NVS_NS = "macro_cfg";

// Max 4 steps per macro, 6 macro slots
#define MAX_STEPS  4
#define MAX_MACROS 6

#define MACRO_MAGIC 0xCAFEBABE

struct RelayStep {
    uint8_t  relay_mask;  // bitmask: 0b0001=R1, 0b0010=R2, 0b0100=R3, 0b1000=R4
    uint16_t duration;    // pulse duration ms
    uint16_t gap;         // pause after this step ms (ignored on last step)
};

struct Macro {
    uint32_t magic;       // must be MACRO_MAGIC for valid data
    char     name[16];
    char     icon[16];
    uint8_t  step_count;
    RelayStep steps[MAX_STEPS];
};

struct MacroConfig {
    uint8_t macro_count;  // how many macros are defined
    uint8_t tag_macro;    // which macro index fires on tag scan
    Macro   macros[MAX_MACROS];
};

// Default macros — mirrors the old RelayAction defaults
// so existing users aren't left with a blank config
static const MacroConfig DEFAULT_MACRO_CONFIG = {
    .macro_count = 5,
    .tag_macro   = 0,
    .macros = {
        { MACRO_MAGIC, "Unlock",     "", 1, {{ 0b0001, 1000, 0    }} },
        { MACRO_MAGIC, "Unlock All", "", 2, {{ 0b0001, 1000, 1000 },
                                              { 0b0001, 1000, 0    }} },
        { MACRO_MAGIC, "Lock",       "", 1, {{ 0b0010, 1000, 0    }} },
        { MACRO_MAGIC, "Trunk",      "", 1, {{ 0b0100, 1000, 0    }} },
        { MACRO_MAGIC, "Panic",      "", 1, {{ 0b1111, 1000, 0    }} },
    }
};

class MacroConfigManager {
public:
    MacroConfig config;

    void load() {
        Preferences prefs;
        prefs.begin(MACRO_NVS_NS, true);

        config.macro_count = prefs.getUChar("macro_count", DEFAULT_MACRO_CONFIG.macro_count);
        config.tag_macro   = prefs.getUChar("tag_macro",   DEFAULT_MACRO_CONFIG.tag_macro);

        // clamp in case of corrupt data
        if (config.macro_count > MAX_MACROS) config.macro_count = MAX_MACROS;
        if (config.tag_macro   >= config.macro_count) config.tag_macro = 0;

        for (uint8_t i = 0; i < config.macro_count; i++) {
            char key[16];
            snprintf(key, sizeof(key), "macro_%u", i);

            Macro temp;
            size_t read = prefs.getBytes(key, &temp, sizeof(Macro));

            if (read == sizeof(Macro) && temp.magic == MACRO_MAGIC) {
                // sanity check step count
                if (temp.step_count > MAX_STEPS) temp.step_count = MAX_STEPS;
                config.macros[i] = temp;
            } else {
                // blob missing, wrong size, or corrupt — fall back to default if available
                if (i < DEFAULT_MACRO_CONFIG.macro_count) {
                    config.macros[i] = DEFAULT_MACRO_CONFIG.macros[i];
                    ESP_LOGW(MACROTAG, "Macro %u invalid or missing from NVS, using default", i);
                }
            }
        }

        prefs.end();
        ESP_LOGI(MACROTAG, "Macros loaded: %u, tag macro: %u", config.macro_count, config.tag_macro);
    }

    void save(uint8_t index) {
        if (index >= config.macro_count) {
            ESP_LOGW(MACROTAG, "Save called with invalid index: %u", index);
            return;
        }

        Preferences prefs;
        prefs.begin(MACRO_NVS_NS, false);

        config.macros[index].magic = MACRO_MAGIC;

        char key[16];
        snprintf(key, sizeof(key), "macro_%u", index);
        prefs.putBytes(key, &config.macros[index], sizeof(Macro));
        prefs.putUChar("macro_count", config.macro_count);
        prefs.putUChar("tag_macro",   config.tag_macro);

        prefs.end();
        ESP_LOGI(MACROTAG, "Macro %u saved: %s", index, config.macros[index].name);
    }

    void saveAll() {
        for (uint8_t i = 0; i < config.macro_count; i++) {
            save(i);
        }
    }

    void clear() {
        Preferences prefs;
        prefs.begin(MACRO_NVS_NS, false);
        prefs.clear();
        prefs.end();
        config = DEFAULT_MACRO_CONFIG;
        ESP_LOGI(MACROTAG, "Macro config cleared — defaults restored");
    }

    void printConfig() {
        ESP_LOGI(MACROTAG, "Macro count: %u, tag macro: %u", config.macro_count, config.tag_macro);
        for (uint8_t i = 0; i < config.macro_count; i++) {
            Macro& m = config.macros[i];
            ESP_LOGI(MACROTAG, "  [%u] %s (icon: %s) steps: %u", i, m.name, m.icon, m.step_count);
            for (uint8_t s = 0; s < m.step_count; s++) {
                ESP_LOGI(MACROTAG, "    step %u: mask=0b%04b dur=%u gap=%u",
                    s, m.steps[s].relay_mask, m.steps[s].duration, m.steps[s].gap);
            }
        }
    }

    Macro& get(uint8_t index) {
        return config.macros[index];
    }

    // Set tag macro index — call save() after to persist
    void setTagMacro(uint8_t index) {
        if (index < config.macro_count) {
            config.tag_macro = index;
        }
    }

    // Add a new empty macro slot — call save() after to persist  
    bool addMacro(const char* name, const char* icon) {
        if (config.macro_count >= MAX_MACROS) {
            ESP_LOGW(MACROTAG, "Macro slots full");
            return false;
        }
        Macro& m = config.macros[config.macro_count];
        m.magic = MACRO_MAGIC;
        strncpy(m.name, name, sizeof(m.name) - 1);
        m.name[sizeof(m.name) - 1] = '\0';
        strncpy(m.icon, icon, sizeof(m.icon) - 1);
        m.icon[sizeof(m.icon) - 1] = '\0';
        m.step_count = 0;
        memset(m.steps, 0, sizeof(m.steps));
        config.macro_count++;
        return true;
    }

    // Remove a macro slot by index, shifts remaining down
    bool removeMacro(uint8_t index) {
        if (index >= config.macro_count) return false;

        for (uint8_t i = index; i < config.macro_count - 1; i++) {
            config.macros[i] = config.macros[i + 1];
        }
        config.macro_count--;

        // fix tag_macro if it pointed at removed or shifted slot
        if (config.tag_macro >= config.macro_count) {
            config.tag_macro = 0;
        }

        return true;
    }
};