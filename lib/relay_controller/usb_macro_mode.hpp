#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.hpp"
#include "macro_config.hpp"

static const char* USBMACROTAG = "USBMACRO";

MacroConfigManager macroConfigManager;

/*
--------------------------------------------------
BOOT DETECTION
--------------------------------------------------
*/

inline bool shouldEnterUsbConfigMode()
{
    pinMode(PAIRING_BUTTON_PIN, INPUT_PULLUP);

    uint32_t start = millis();

    while (millis() - start < USB_CONFIG_HOLD_MS)
    {
        if (digitalRead(PAIRING_BUTTON_PIN) != LOW)
        {
            return false;
        }

        delay(10);
    }

    return true;
}

/*
--------------------------------------------------
SERIALIZE MACROS -> JSON
--------------------------------------------------
*/

inline String macrosToJson()
{
    JsonDocument doc;

    doc["macro_count"] = macroConfigManager.config.macro_count;
    doc["tag_macro"]   = macroConfigManager.config.tag_macro;

    JsonArray macros = doc["macros"].to<JsonArray>();
    for (uint8_t i = 0; i < macroConfigManager.config.macro_count; i++) {
        Macro& m = macroConfigManager.config.macros[i];
        JsonObject macro = macros.add<JsonObject>();
        macro["name"] = m.name;
        macro["icon"] = m.icon;
        macro["step_count"] = m.step_count;

        JsonArray steps = macro["steps"].to<JsonArray>();
        for (uint8_t s = 0; s < m.step_count; s++) {
            JsonObject step = steps.add<JsonObject>();
            step["relay_mask"] = m.steps[s].relay_mask;
            step["duration"]   = m.steps[s].duration;
            step["gap"]        = m.steps[s].gap;
        }
    }

    String out;
    serializeJson(doc, out);
    return out;
}

/*
--------------------------------------------------
DESERIALIZE JSON -> MACROS
--------------------------------------------------
*/

inline bool saveMacrosFromJson(const String& json)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err)
    {
        ESP_LOGE(USBMACROTAG, "Invalid JSON");
        return false;
    }

    // Parse macro configuration
    uint8_t count = doc["macro_count"] | 0;
    if (count == 0 || count > MAX_MACROS) {
        ESP_LOGE(USBMACROTAG, "Invalid macro_count");
        return false;
    }

    uint8_t tag_macro = doc["tag_macro"] | 0;
    if (tag_macro >= count) {
        ESP_LOGE(USBMACROTAG, "Invalid tag_macro");
        return false;
    }

    macroConfigManager.config.macro_count = count;
    macroConfigManager.config.tag_macro   = tag_macro;

    uint32_t now = millis();
    JsonArray macros = doc["macros"].as<JsonArray>();
    for (uint8_t i = 0; i < count; i++) {
        JsonObject m = macros[i].as<JsonObject>();
        Macro& macro = macroConfigManager.config.macros[i];

        const char* name = m["name"] | "";
        const char* icon = m["icon"] | "";
        strncpy(macro.name, name, sizeof(macro.name) - 1);
        macro.name[sizeof(macro.name) - 1] = '\0';
        strncpy(macro.icon, icon, sizeof(macro.icon) - 1);
        macro.icon[sizeof(macro.icon) - 1] = '\0';
        macro.magic = MACRO_MAGIC;
        macro.updated_at = now;

        uint8_t step_count = m["step_count"] | 0;
        if (step_count > MAX_STEPS)
            step_count = MAX_STEPS;

        macro.step_count = step_count;

        // Clear unused steps
        for (uint8_t s = step_count; s < MAX_STEPS; s++) {
            macro.steps[s].relay_mask = 0;
            macro.steps[s].duration   = 0;
            macro.steps[s].gap        = 0;
        }

        JsonArray steps = m["steps"].as<JsonArray>();
        for (uint8_t s = 0; s < step_count; s++) {
            JsonObject step = steps[s].as<JsonObject>();
            macro.steps[s].relay_mask = step["relay_mask"] | 0;
            macro.steps[s].duration = step["duration"] | 500;
            macro.steps[s].gap = step["gap"] | 0;
        }
    }

    macroConfigManager.saveAll();

    ESP_LOGI(USBMACROTAG, "USB macros saved: count=%d tag=%d", count, tag_macro);

    return true;
}

/*
--------------------------------------------------
USB CONFIG MODE LOOP
--------------------------------------------------
Protocol:

GET_MACROS
SAVE_MACROS:{json}
--------------------------------------------------
*/

inline void startUsbMacroConfigMode()
{
    Serial.begin(115200);
    delay(1000);

    macroConfigManager.load();

    ESP_LOGI(
        USBMACROTAG,
        "USB Macro Config Mode Started"
    );

    Serial.println("{\"mode\":\"usb_config\"}");

    while (true)
    {
        if (Serial.available())
        {
            String line =
                Serial.readStringUntil('\n');

            line.trim();

            if (line == "GET_MACROS")
            {
                Serial.println(macrosToJson());
            }
            else if (
                line.startsWith("SAVE_MACROS:")
            )
            {
                String json =
                    line.substring(
                        strlen("SAVE_MACROS:")
                    );

                bool ok =
                    saveMacrosFromJson(json);

                if (ok)
                {
                    Serial.println(
                        "{\"ok\":true}"
                    );
                }
                else
                {
                    Serial.println(
                        "{\"ok\":false}"
                    );
                }
            }
            else
            {
                Serial.println(
                    "{\"error\":\"unknown_command\"}"
                );
            }
        }

        delay(5);
    }
}