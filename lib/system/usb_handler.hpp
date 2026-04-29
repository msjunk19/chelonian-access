#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "macro_config.hpp"
#include "esp_log.h"

static const char* USBTAG = "USB";

class UsbCommandHandler {
public:

// static void loop(MacroConfigManager& m) {
//     static String buffer = "";

//     while (Serial.available()) {
//         char c = Serial.read();
        
//         if (c == '\n') {
//             buffer.trim();

//             if (buffer.length() == 0) {
//                 buffer = "";
//                 return;
//             }

//             Serial.print("DEBUG: Complete buffer: ");
//             Serial.println(buffer);

//             processCommand(buffer, m);
//             buffer = "";
//         }
//         else {
//             buffer += c;
//         }
//     }
// }
    static void loop(MacroConfigManager& m) {
        static String buffer = "";

        while (Serial.available()) {
            char c = Serial.read();

            if (c == '\n') {
                buffer.trim();

                if (buffer.length() == 0) {
                    buffer = "";
                    return;
                }

                processCommand(buffer, m);
                buffer = "";
            }
            else {
                buffer += c;
            }
        }
    }

private:

// static void processCommand(const String& cmd, MacroConfigManager& m) {

//     if (cmd == "GET_MACROS") {
//         handleGetMacros(m);
//         return;
//     }

//     if (cmd.startsWith("SAVE_MACROS:")) {
//         Serial.print("DEBUG: SAVE_MACROS buffer length: ");
//         Serial.println(cmd.length());
//         Serial.print("DEBUG: First 100 chars: ");
//         Serial.println(cmd.substring(0, 100));
        
//         Serial.println("SAVE_MACROS: Detected!");
//         String json = cmd.substring(strlen("SAVE_MACROS:"));
//         handleSaveMacros(json, m);
//         return;
//     }

//     Serial.println("{\"ok\":false,\"error\":\"unknown_command\"}");
// }


    static void processCommand(const String& cmd, MacroConfigManager& m) {

        if (cmd == "GET_MACROS") {
            // Serial.println("GET_MACROS: Detected!");

            handleGetMacros(m);
            return;
        }

        if (cmd.startsWith("SAVE_MACROS:")) {
            // Serial.println("SAVE_MACROS: Detected!");
            String json = cmd.substring(strlen("SAVE_MACROS:"));
            handleSaveMacros(json, m);
            return;
        }

        Serial.println("{\"ok\":false,\"error\":\"unknown_command\"}");
    }

    static void handleGetMacros(MacroConfigManager& m) {

        JsonDocument doc;

        doc["macro_count"] = m.config.macro_count;
        doc["tag_macro"] = m.config.tag_macro;

        JsonArray macros = doc["macros"].to<JsonArray>();

        for (uint8_t i = 0; i < m.config.macro_count; i++) {
            Macro& macro = m.config.macros[i];

            JsonObject mm = macros.add<JsonObject>();
            mm["name"] = macro.name;
            mm["icon"] = macro.icon;
            mm["step_count"] = macro.step_count;

            JsonArray steps = mm["steps"].to<JsonArray>();

            for (uint8_t s = 0; s < macro.step_count; s++) {
                JsonObject st = steps.add<JsonObject>();
                st["relay_mask"] = macro.steps[s].relay_mask;
                st["duration"] = macro.steps[s].duration;
                st["gap"] = macro.steps[s].gap;
            }
        }

        String out;
        serializeJson(doc, out);

        Serial.println(out);
    }
// static void handleSaveMacros(String json, MacroConfigManager& m) {

//     Serial.println("{\"debug\":\"ENTER SAVE\"}");

//     Serial.print("{\"debug\":\"LEN\":");
//     Serial.print(json.length());
//     Serial.println("}");

//     Serial.println("{\"debug\":\"RAW_START\"}");
//     Serial.println(json);
//     Serial.println("{\"debug\":\"RAW_END\"}");

//     Serial.println("{\"ok\":true}");
// }
    static void handleSaveMacros(String json, MacroConfigManager& m) {

        // Serial.println("{\"debug\":\"ENTER SAVE\"}");

        // IMPORTANT: sized document (fixes your 500 crash)
        // DynamicJsonDocument doc(16384);
        JsonDocument doc;


        DeserializationError err = deserializeJson(doc, json);

        if (err) {
            Serial.println("{\"ok\":false,\"error\":\"bad_json\"}");
            return;
        }

        uint8_t count = doc["macro_count"] | 0;
        uint8_t tag = doc["tag_macro"] | 0;

        if (count == 0 || count > MAX_MACROS) {
            Serial.println("{\"ok\":false,\"error\":\"invalid_count\"}");
            return;
        }

        m.config.macro_count = count;
        m.config.tag_macro = tag;

        JsonArray macros = doc["macros"].as<JsonArray>();
        uint32_t now = millis();

        for (uint8_t i = 0; i < count; i++) {

            JsonObject src = macros[i].as<JsonObject>();
            Macro& dst = m.config.macros[i];

            memset(&dst, 0, sizeof(Macro));

            dst.magic = MACRO_MAGIC;
            dst.updated_at = now;

            const char* name = src["name"] | "";
            const char* icon = src["icon"] | "";

            strncpy(dst.name, name, sizeof(dst.name) - 1);
            strncpy(dst.icon, icon, sizeof(dst.icon) - 1);

            uint8_t steps = src["step_count"] | 0;
            if (steps > MAX_STEPS) steps = MAX_STEPS;

            dst.step_count = steps;

            JsonArray arr = src["steps"].as<JsonArray>();

            if (arr.isNull()) {
                Serial.println("{\"ok\":false,\"error\":\"missing_steps\"}");
                return;
            }

            for (uint8_t s = 0; s < steps; s++) {

                if (s >= arr.size()) {
                    Serial.println("{\"ok\":false,\"error\":\"step_index_oob\"}");
                    return;
                }

                JsonObject st = arr[s].as<JsonObject>();

                dst.steps[s].relay_mask = st["relay_mask"] | 0;
                dst.steps[s].duration   = st["duration"] | 0;
                dst.steps[s].gap        = st["gap"] | 0;
            }
        }

        // Serial.println("{\"debug\":\"before_save\"}");

        m.saveAll();

        // Serial.println("{\"debug\":\"after_save\"}");
        Serial.println("{\"ok\":true}");
    }


};




// #pragma once
// #include <ArduinoJson.h>
// #include "macro_config.hpp"
// #include "esp_log.h"

// static const char* USBTAG = "USB";

// class UsbCommandHandler {
// public:

// static void loop(MacroConfigManager& macroConfigManager) {
//     static String buffer = "";

//     while (Serial.available()) {
//         char c = Serial.read();

//         if (c == '\n') {
//             buffer.trim();

//             if (buffer.length() == 0) {
//                 buffer = "";
//                 return;
//             }

//             processCommand(buffer, macroConfigManager);  // 🔥 ONLY THIS

//             buffer = "";
//         }
//         else {
//             buffer += c;
//         }
//     }
// }

// // static void loop(MacroConfigManager& macroConfigManager) {
// //     static String buffer = "";

// //     while (Serial.available()) {
// //         char c = Serial.read();

// //         if (c == '\n') {
// //             buffer.trim();

// //             if (buffer.length() == 0) {
// //                 buffer = "";
// //                 return;
// //             }

// //             if (buffer == "GET_MACROS") {
// //                 handleGetMacros(macroConfigManager);
// //             }
// //             else if (buffer.startsWith("SAVE_MACROS:")) {
// //                 String json = buffer.substring(strlen("SAVE_MACROS:"));
// //                 handleSaveMacros(json, macroConfigManager);
// //             }
// //             else {
// //                 Serial.println("{\"ok\":false,\"error\":\"unknown_command\"}");
// //             }

// //             buffer = "";
// //         }
// //         else {
// //             buffer += c;
// //         }
// //     }
// // }
    
// // void processCommand(const String& cmd, MacroConfigManager& m) {

// //     if (cmd == "GET_MACROS") {
// //         Serial.println("{\"ok\":true}");
// //         return;
// //     }

// //     Serial.println("{\"ok\":false}");
// // }

// void processCommand(const String& cmd, MacroConfigManager& m) {

//     if (cmd == "GET_MACROS") {
//         handleGetMacros(m);
//         return;
//     }

//     if (cmd.startsWith("SAVE_MACROS:")) {
//         String json = cmd.substring(strlen("SAVE_MACROS:"));
//         handleSaveMacros(json, m);
//         return;
//     }

//     Serial.println("{\"ok\":false,\"error\":\"unknown_command\"}");
// }

// static void begin(MacroConfigManager& m) {
//     static String buffer = "";

//     while (Serial.available()) {
//         char c = Serial.read();

//         if (c == '\n') {
//             buffer.trim();

//             if (buffer.length() == 0) {
//                 buffer = "";
//                 return;
//             }

//             processCommand(buffer, m);   // 🔥 THIS IS THE FIX

//             buffer = "";
//         } else {
//             buffer += c;
//         }
//     }
// }

// // static void begin() {
// //     static String buffer = "";

// //     while (Serial.available()) {
// //     char c = Serial.read();

// //     if (c == '\n') {
// //         buffer.trim();

// //         if (buffer.length() == 0) {
// //             buffer = "";
// //             return;
// //         }

// //         if (buffer == "GET_MACROS") {
// //             Serial.println("{\"ok\":true}");
// //         }

// //         buffer = "";
// //     }
// //     else {
// //         buffer += c;
// //     }
// // }
// // }


// private:

//     static void handleGetMacros(MacroConfigManager& m) {
//         JsonDocument doc;

//         doc["macro_count"] = m.config.macro_count;
//         doc["tag_macro"] = m.config.tag_macro;

//         JsonArray macros = doc["macros"].to<JsonArray>();

//         for (uint8_t i = 0; i < m.config.macro_count; i++) {
//             Macro& macro = m.config.macros[i];

//             JsonObject mm = macros.add<JsonObject>();
//             mm["name"] = macro.name;
//             mm["icon"] = macro.icon;
//             mm["step_count"] = macro.step_count;

//             JsonArray steps = mm["steps"].to<JsonArray>();

//             for (uint8_t s = 0; s < macro.step_count; s++) {
//                 JsonObject st = steps.add<JsonObject>();
//                 st["relay_mask"] = macro.steps[s].relay_mask;
//                 st["duration"] = macro.steps[s].duration;
//                 st["gap"] = macro.steps[s].gap;
//             }
//         }

//         String out;
//         serializeJson(doc, out);

//         // Serial.println(out);
//         Serial.print(out);
//         Serial.print("\n");
//     }

//     static void handleSaveMacros(String json, MacroConfigManager& m) {

// Serial.println("{\"debug\":\"ENTER SAVE\"}");

//         JsonDocument doc;
//         DeserializationError err = deserializeJson(doc, json);

//         if (err) {
//             Serial.println("{\"ok\":false,\"error\":\"bad_json\"}");
//             return;
//         }

//         uint8_t count = doc["macro_count"] | 0;
//         uint8_t tag = doc["tag_macro"] | 0;

//         if (count == 0 || count > MAX_MACROS) {
//             Serial.println("{\"ok\":false,\"error\":\"invalid_count\"}");
//             return;
//         }

//         m.config.macro_count = count;
//         m.config.tag_macro = tag;

//         JsonArray macros = doc["macros"].as<JsonArray>();
//         uint32_t now = millis();

//         for (uint8_t i = 0; i < count; i++) {

//             JsonObject src = macros[i].as<JsonObject>();
//             Macro& dst = m.config.macros[i];

//             memset(&dst, 0, sizeof(Macro));

//             dst.magic = MACRO_MAGIC;
//             dst.updated_at = now;

//             const char* name = src["name"] | "";
//             const char* icon = src["icon"] | "";

//             strncpy(dst.name, name, sizeof(dst.name) - 1);
//             strncpy(dst.icon, icon, sizeof(dst.icon) - 1);

//             uint8_t steps = src["step_count"] | 0;
//             if (steps > MAX_STEPS) steps = MAX_STEPS;

//             dst.step_count = steps;

//             // JsonArray arr = src["steps"].as<JsonArray>();

//             // for (uint8_t s = 0; s < steps; s++) {
//             //     JsonObject st = arr[s].as<JsonObject>();
//             JsonArray arr = src["steps"].as<JsonArray>();

//             if (arr.isNull()) {
//                 Serial.println("{\"ok\":false,\"error\":\"missing_steps\"}");
//                 return;
//             }

//             for (uint8_t s = 0; s < steps; s++) {

//                 if (s >= arr.size()) {
//                     Serial.println("{\"ok\":false,\"error\":\"step_index_oob\"}");
//                     return;
//                 }

//                 JsonObject st = arr[s].as<JsonObject>();
                
//                 dst.steps[s].relay_mask = st["relay_mask"] | 0;
//                 dst.steps[s].duration   = st["duration"] | 0;
//                 dst.steps[s].gap        = st["gap"] | 0;
//             }
//         }

//         Serial.println("{\"debug\":\"A before saveAll\"}");

//         m.saveAll();

//         Serial.println("{\"debug\":\"B after saveAll start\"}");

//         Serial.println("{\"ok\":true}");
//     }
// };