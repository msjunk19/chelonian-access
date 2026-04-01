// #pragma once
// #include <Preferences.h>
// #include "esp_log.h"
// #include <pin_mapping.hpp>

// static const char* RELAYCFGTAG = "RELAYCFG";
// static const char* RELAY_NVS_NS = "relay_cfg";

// // Relay activation modes
// enum class RelayMode : uint8_t {
//     SINGLE     = 0,  // one relay, one pulse
//     DOUBLE     = 1,  // one relay, two pulses
//     SEQUENTIAL = 2,  // relay 1 then relay 2
//     ALL        = 3   // all relays simultaneously
// };

// // Named relay actions — maps to real world functions
// enum class RelayAction : uint8_t {
//     UNLOCK     = 0,
//     UNLOCK_ALL = 1,
//     LOCK       = 2,
//     TRUNK      = 3,
//     PANIC      = 4,
//     COUNT      = 5  // keep last — used for array sizing
// };

// // Per-action configuration
// // struct RelayActionConfig {
// //     RelayMode mode      = RelayMode::SINGLE;
// //     uint8_t   relay     = RELAY_1;   // primary relay
// //     uint8_t   relay2    = RELAY_2;   // secondary relay (sequential only)
// //     uint16_t  duration  = 500;       // pulse duration ms
// //     uint16_t  duration2 = 500;       // secondary duration ms (sequential only)
// //     uint16_t  gap       = 500;       // gap between pulses (double only)
// // };

// struct RelayActionConfig {
//     RelayMode mode      = RelayMode::SINGLE;
//     uint8_t   relay     = RELAY_1;
//     uint8_t   relay2    = RELAY_2;
//     uint16_t  duration  = 500;
//     uint16_t  duration2 = 500;
//     uint16_t  gap       = 500;

//     // Default constructor
//     RelayActionConfig() = default;

//     // Parameterized constructor
//     constexpr RelayActionConfig(RelayMode m, uint8_t r, uint8_t r2, 
//                                 uint16_t d, uint16_t d2, uint16_t g)
//         : mode(m), relay(r), relay2(r2), 
//           duration(d), duration2(d2), gap(g) {}
// };

// // Default configs per action
// static const RelayActionConfig DEFAULT_CONFIGS[(uint8_t)RelayAction::COUNT] = {
//     // UNLOCK     — single pulse on relay 1
//     { RelayMode::SINGLE,     RELAY_1, RELAY_2, 500, 500, 500 },
//     // UNLOCK_ALL — double pulse on relay 1
//     { RelayMode::DOUBLE,     RELAY_1, RELAY_2, 500, 500, 500 },
//     // LOCK       — single pulse on relay 2
//     { RelayMode::SINGLE,     RELAY_2, RELAY_2, 500, 500, 500 },
//     // TRUNK      — single pulse on relay 3
//     { RelayMode::SINGLE,     RELAY_3, RELAY_3, 500, 500, 500 },
//     // PANIC      — all relays simultaneously
//     { RelayMode::ALL,        RELAY_1, RELAY_2, 500, 500, 500 },
// };

// static const char* ACTION_NAMES[(uint8_t)RelayAction::COUNT] = {
//     "unlock", "unlock_all", "lock", "trunk", "panic"
// };

// class RelayConfigManager {
// public:
//     RelayActionConfig configs[(uint8_t)RelayAction::COUNT];

//     void load() {
//         Preferences prefs;
//         prefs.begin(RELAY_NVS_NS, true);

//         for (uint8_t i = 0; i < (uint8_t)RelayAction::COUNT; i++) {
//             char key[24];

//             snprintf(key, sizeof(key), "%s_mode",  ACTION_NAMES[i]);
//             configs[i].mode = (RelayMode)prefs.getUChar(key, (uint8_t)DEFAULT_CONFIGS[i].mode);

//             snprintf(key, sizeof(key), "%s_relay",  ACTION_NAMES[i]);
//             configs[i].relay = prefs.getUChar(key, DEFAULT_CONFIGS[i].relay);

//             snprintf(key, sizeof(key), "%s_relay2", ACTION_NAMES[i]);
//             configs[i].relay2 = prefs.getUChar(key, DEFAULT_CONFIGS[i].relay2);

//             snprintf(key, sizeof(key), "%s_dur",    ACTION_NAMES[i]);
//             configs[i].duration = prefs.getUShort(key, DEFAULT_CONFIGS[i].duration);

//             snprintf(key, sizeof(key), "%s_dur2",   ACTION_NAMES[i]);
//             configs[i].duration2 = prefs.getUShort(key, DEFAULT_CONFIGS[i].duration2);

//             snprintf(key, sizeof(key), "%s_gap",    ACTION_NAMES[i]);
//             configs[i].gap = prefs.getUShort(key, DEFAULT_CONFIGS[i].gap);
//         }

//         prefs.end();
//         ESP_LOGI(RELAYCFGTAG, "Relay configs loaded");
//     }

//     void save(RelayAction action) {
//         uint8_t i = (uint8_t)action;
//         Preferences prefs;
//         prefs.begin(RELAY_NVS_NS, false);

//         char key[24];
//         snprintf(key, sizeof(key), "%s_mode",   ACTION_NAMES[i]); prefs.putUChar(key,  (uint8_t)configs[i].mode);
//         snprintf(key, sizeof(key), "%s_relay",  ACTION_NAMES[i]); prefs.putUChar(key,  configs[i].relay);
//         snprintf(key, sizeof(key), "%s_relay2", ACTION_NAMES[i]); prefs.putUChar(key,  configs[i].relay2);
//         snprintf(key, sizeof(key), "%s_dur",    ACTION_NAMES[i]); prefs.putUShort(key, configs[i].duration);
//         snprintf(key, sizeof(key), "%s_dur2",   ACTION_NAMES[i]); prefs.putUShort(key, configs[i].duration2);
//         snprintf(key, sizeof(key), "%s_gap",    ACTION_NAMES[i]); prefs.putUShort(key, configs[i].gap);

//         prefs.end();
//         ESP_LOGI(RELAYCFGTAG, "Relay config saved for action: %s", ACTION_NAMES[i]);
//     }

//     void saveAll() {
//         for (uint8_t i = 0; i < (uint8_t)RelayAction::COUNT; i++) {
//             save((RelayAction)i);
//         }
//     }

//     void clear() {
//         Preferences prefs;
//         prefs.begin(RELAY_NVS_NS, false);
//         prefs.clear();
//         prefs.end();
//         for (uint8_t i = 0; i < (uint8_t)RelayAction::COUNT; i++) {
//             configs[i] = DEFAULT_CONFIGS[i];
//         }
//         ESP_LOGI(RELAYCFGTAG, "Relay configs cleared — defaults restored");
//     }

//     void printConfig() {
//         for (uint8_t i = 0; i < (uint8_t)RelayAction::COUNT; i++) {
//             ESP_LOGI(RELAYCFGTAG, "%s: mode=%d relay=%d relay2=%d dur=%d dur2=%d gap=%d",
//                 ACTION_NAMES[i],
//                 (uint8_t)configs[i].mode,
//                 configs[i].relay,
//                 configs[i].relay2,
//                 configs[i].duration,
//                 configs[i].duration2,
//                 configs[i].gap);
//         }
//     }

//     // Get config for a specific action
//     RelayActionConfig& get(RelayAction action) {
//         return configs[(uint8_t)action];
//     }
// };

// // #pragma once
// // #include <Preferences.h>
// // #include "esp_log.h"
// // #include <pin_mapping.hpp>

// // static const char* RELAYCFGTAG = "RELAYCFG";
// // static const char* RELAY_NVS_NS = "relay_cfg";

// // // Relay activation modes
// // enum class RelayMode : uint8_t {
// //     SINGLE     = 0,
// //     DOUBLE     = 1,
// //     SEQUENTIAL = 2,
// //     ALL        = 3
// // };

// // struct RelayConfig {
// //     RelayMode mode = RelayMode::SEQUENTIAL;

// //     // Single pulse
// //     uint8_t  single_relay    = 0;    // relay 0 (first relay)
// //     uint16_t single_duration = 500;

// //     // Double pulse
// //     uint8_t  double_relay    = 0;
// //     uint16_t double_duration = 500;
// //     uint16_t double_gap      = 500;

// //     // Sequential
// //     uint8_t  seq_r1          = 0;    // first relay
// //     uint8_t  seq_r2          = 1;    // second relay
// //     uint16_t seq_r1_duration = 500;
// //     uint16_t seq_r2_duration = 500;

// //     // All simultaneous
// //     uint16_t all_duration    = 500;
// // };

// // class RelayConfigManager {
// // public:
// //     RelayConfig config;

// //     void load() {
// //         Preferences prefs;
// //         prefs.begin(RELAY_NVS_NS, true);

// //         config.mode           = (RelayMode)prefs.getUChar("mode",           (uint8_t)RelayMode::SEQUENTIAL);
// //         config.single_relay   = prefs.getUChar("single_relay",   RELAY_1);
// //         config.double_relay   = prefs.getUChar("double_relay",   RELAY_1);
// //         config.double_duration= prefs.getUShort("double_dur",    500);
// //         config.double_gap     = prefs.getUShort("double_gap",    500);
// //         config.seq_r1         = prefs.getUChar("seq_r1",         RELAY_1);
// //         config.seq_r2         = prefs.getUChar("seq_r2",         RELAY_2);
// //         config.seq_r1_duration= prefs.getUShort("seq_r1_dur",    500);
// //         config.seq_r2_duration= prefs.getUShort("seq_r2_dur",    500);
// //         config.all_duration   = prefs.getUShort("all_dur",       500);

// //         prefs.end();
// //         ESP_LOGI(RELAYCFGTAG, "Relay config loaded — mode: %d", (uint8_t)config.mode);
// //     }

// //     void save() {
// //         Preferences prefs;
// //         prefs.begin(RELAY_NVS_NS, false);

// //         prefs.putUChar("mode",          (uint8_t)config.mode);
// //         prefs.putUChar("single_relay",  config.single_relay);
// //         prefs.putUShort("single_dur",   config.single_duration);
// //         prefs.putUChar("double_relay",  config.double_relay);
// //         prefs.putUShort("double_dur",   config.double_duration);
// //         prefs.putUShort("double_gap",   config.double_gap);
// //         prefs.putUChar("seq_r1",        config.seq_r1);
// //         prefs.putUChar("seq_r2",        config.seq_r2);
// //         prefs.putUShort("seq_r1_dur",   config.seq_r1_duration);
// //         prefs.putUShort("seq_r2_dur",   config.seq_r2_duration);
// //         prefs.putUShort("all_dur",      config.all_duration);

// //         prefs.end();
// //         ESP_LOGI(RELAYCFGTAG, "Relay config saved — mode: %d", (uint8_t)config.mode);
// //     }

// //     void clear() {
// //         Preferences prefs;
// //         prefs.begin(RELAY_NVS_NS, false);
// //         prefs.clear();
// //         prefs.end();
// //         config = RelayConfig(); // reset to defaults
// //         ESP_LOGI(RELAYCFGTAG, "Relay config cleared — defaults restored");
// //     }

// //     void printConfig() {
// //         ESP_LOGI(RELAYCFGTAG, "Mode: %d", (uint8_t)config.mode);
// //         ESP_LOGI(RELAYCFGTAG, "Single: relay=%d dur=%dms", config.single_relay, config.single_duration);
// //         ESP_LOGI(RELAYCFGTAG, "Double: relay=%d dur=%dms gap=%dms", config.double_relay, config.double_duration, config.double_gap);
// //         ESP_LOGI(RELAYCFGTAG, "Sequential: r1=%d(%dms) r2=%d(%dms)", config.seq_r1, config.seq_r1_duration, config.seq_r2, config.seq_r2_duration);
// //         ESP_LOGI(RELAYCFGTAG, "All: dur=%dms", config.all_duration);
// //     }
// // };