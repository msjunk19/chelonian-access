// #pragma once

// #include "esp_http_server.h"
// #include "esp_spiffs.h"
// #include "nvs_flash.h"
// #include "cJSON.h"
// #include <string.h>
// #include "macro_config.hpp"

// static httpd_handle_t server = NULL;
// static MacroConfigManager* macro_manager = NULL;

// // ============ LITTLEFS INITIALIZATION ============
// void init_littlefs() {
//     esp_vfs_littlefs_conf_t conf = {
//         .base_path = "/littlefs",
//         .partition_label = "littlefs",
//         .format_if_mount_failed = false,
//         .dont_mount = false,
//     };

//     esp_err_t ret = esp_vfs_littlefs_register(&conf);
//     if (ret != ESP_OK) {
//         if (ret == ESP_ERR_NOT_FOUND) {
//             ESP_LOGE("LITTLEFS", "littlefs partition not found");
//         } else if (ret == ESP_ERR_INVALID_ARG) {
//             ESP_LOGE("LITTLEFS", "littlefs invalid partition config");
//         } else {
//             ESP_LOGE("LITTLEFS", "Failed to mount littlefs: %s", esp_err_to_name(ret));
//         }
//         return;
//     }

//     size_t total = 0, used = 0;
//     ret = esp_littlefs_info("littlefs", &total, &used);
//     if (ret == ESP_OK) {
//         ESP_LOGI("LITTLEFS", "littlefs partition: total=%d bytes, used=%d bytes", total, used);
//     }
// }

// // ============ STATIC FILE HANDLER ============
// static esp_err_t static_file_handler(httpd_req_t *req) {
//     char filepath[256];
    
//     if (strcmp(req->uri, "/") == 0) {
//         snprintf(filepath, sizeof(filepath), "/littlefs/macros.html");
//     } else {
//         snprintf(filepath, sizeof(filepath), "/littlefs%s", req->uri);
//     }

//     FILE *f = fopen(filepath, "r");
//     if (!f) {
//         ESP_LOGW("HTTP", "File not found: %s", filepath);
//         httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
//         return ESP_FAIL;
//     }

//     // Set correct content type
//     if (strstr(req->uri, ".css")) {
//         httpd_resp_set_type(req, "text/css");
//     } else if (strstr(req->uri, ".js")) {
//         httpd_resp_set_type(req, "application/javascript");
//     } else {
//         httpd_resp_set_type(req, "text/html");
//     }

//     // Stream file to client
//     char buffer[512];
//     size_t read;
//     while ((read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
//         if (httpd_resp_send_chunk(req, buffer, read) != ESP_OK) {
//             fclose(f);
//             ESP_LOGE("HTTP", "Error sending file");
//             return ESP_FAIL;
//         }
//     }

//     fclose(f);
//     httpd_resp_send_chunk(req, NULL, 0);
//     return ESP_OK;
// }

// // ============ MACRO SERIALIZATION HELPERS ============

// /**
//  * Convert a RelayStep struct to JSON object
//  */
// static cJSON* relayStep_to_json(const RelayStep& step) {
//     cJSON *obj = cJSON_CreateObject();
//     cJSON_AddNumberToObject(obj, "relay_mask", step.relay_mask);
//     cJSON_AddNumberToObject(obj, "duration", step.duration);
//     cJSON_AddNumberToObject(obj, "gap", step.gap);
//     return obj;
// }

// /**
//  * Convert JSON object to RelayStep struct
//  */
// static bool json_to_relayStep(cJSON *obj, RelayStep& step) {
//     if (!obj) return false;

//     cJSON *mask_item = cJSON_GetObjectItem(obj, "relay_mask");
//     cJSON *dur_item = cJSON_GetObjectItem(obj, "duration");
//     cJSON *gap_item = cJSON_GetObjectItem(obj, "gap");

//     if (!mask_item || !dur_item || !gap_item) return false;

//     step.relay_mask = (uint8_t)mask_item->valueint & 0x0F;  // Clamp to 4 bits
//     step.duration = (uint16_t)dur_item->valueint;
//     step.gap = (uint16_t)gap_item->valueint;

//     return true;
// }

// /**
//  * Convert a Macro struct to JSON object (for sending to web UI)
//  */
// static cJSON* macro_to_json(const Macro& macro) {
//     cJSON *obj = cJSON_CreateObject();
    
//     cJSON_AddStringToObject(obj, "name", macro.name);
//     cJSON_AddStringToObject(obj, "icon", macro.icon);
//     cJSON_AddNumberToObject(obj, "step_count", macro.step_count);
//     cJSON_AddNumberToObject(obj, "updated_at", macro.updated_at);

//     cJSON *steps_array = cJSON_CreateArray();
//     for (uint8_t i = 0; i < macro.step_count && i < MAX_STEPS; i++) {
//         cJSON_AddItemToArray(steps_array, relayStep_to_json(macro.steps[i]));
//     }
//     cJSON_AddItemToObject(obj, "steps", steps_array);

//     return obj;
// }

// /**
//  * Convert JSON object to Macro struct (from web UI)
//  * Preserves magic number and updated_at timestamp
//  */
// static bool json_to_macro(cJSON *obj, Macro& macro) {
//     if (!obj) return false;

//     cJSON *name_item = cJSON_GetObjectItem(obj, "name");
//     cJSON *icon_item = cJSON_GetObjectItem(obj, "icon");
//     cJSON *steps_item = cJSON_GetObjectItem(obj, "steps");

//     if (!name_item || !icon_item || !steps_item) {
//         ESP_LOGW("MACRO", "Missing required fields in macro JSON");
//         return false;
//     }

//     // Update name and icon
//     strncpy(macro.name, name_item->valuestring ? name_item->valuestring : "", sizeof(macro.name) - 1);
//     macro.name[sizeof(macro.name) - 1] = '\0';

//     strncpy(macro.icon, icon_item->valuestring ? icon_item->valuestring : "", sizeof(macro.icon) - 1);
//     macro.icon[sizeof(macro.icon) - 1] = '\0';

//     // Update steps
//     macro.step_count = 0;
//     memset(macro.steps, 0, sizeof(macro.steps));

//     cJSON *step_item = NULL;
//     cJSON_ArrayForEach(step_item, steps_item) {
//         if (macro.step_count >= MAX_STEPS) break;
//         if (json_to_relayStep(step_item, macro.steps[macro.step_count])) {
//             macro.step_count++;
//         }
//     }

//     // Set magic and update timestamp
//     macro.magic = MACRO_MAGIC;
//     macro.updated_at = millis();

//     ESP_LOGI("MACRO", "Deserialized macro: %s, %u steps", macro.name, macro.step_count);
//     return true;
// }

// // ============ NVS API HANDLERS ============

// /**
//  * GET /api/nonce
//  * Returns a nonce for request signing
//  */
// static esp_err_t nonce_handler(httpd_req_t *req) {
//     static uint32_t nonce_counter = 0;
//     nonce_counter++;

//     cJSON *json = cJSON_CreateObject();
//     cJSON_AddNumberToObject(json, "nonce", nonce_counter);
    
//     char *json_str = cJSON_Print(json);
//     httpd_resp_set_type(req, "application/json");
//     httpd_resp_send(req, json_str, strlen(json_str));
    
//     cJSON_Delete(json);
//     free(json_str);
//     return ESP_OK;
// }

// /**
//  * GET /api/macros
//  * Load all macros from the MacroConfigManager and send as JSON
//  */
// static esp_err_t macros_get_handler(httpd_req_t *req) {
//     if (!macro_manager) {
//         httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Macro manager not initialized");
//         return ESP_FAIL;
//     }

//     cJSON *json = cJSON_CreateObject();
//     cJSON *macros_array = cJSON_CreateArray();

//     // Add macro count and tag macro
//     cJSON_AddNumberToObject(json, "macro_count", macro_manager->config.macro_count);
//     cJSON_AddNumberToObject(json, "tag_macro", macro_manager->config.tag_macro);

//     // Convert each macro struct to JSON
//     for (uint8_t i = 0; i < macro_manager->config.macro_count && i < MAX_MACROS; i++) {
//         const Macro& m = macro_manager->config.macros[i];
        
//         // Validate magic number
//         if (m.magic != MACRO_MAGIC) {
//             ESP_LOGW("MACRO", "Macro %u has invalid magic: 0x%08X", i, m.magic);
//             continue;
//         }

//         cJSON_AddItemToArray(macros_array, macro_to_json(m));
//     }

//     cJSON_AddItemToObject(json, "macros", macros_array);

//     char *json_str = cJSON_Print(json);
//     httpd_resp_set_type(req, "application/json");
//     httpd_resp_send(req, json_str, strlen(json_str));

//     ESP_LOGI("HTTP", "Sent %u macros to client", macro_manager->config.macro_count);

//     cJSON_Delete(json);
//     free(json_str);
//     return ESP_OK;
// }

// /**
//  * POST /api/macros
//  * Receive macro configuration from web UI and save to NVS
//  */
// static esp_err_t macros_post_handler(httpd_req_t *req) {
//     if (!macro_manager) {
//         httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Macro manager not initialized");
//         return ESP_FAIL;
//     }

//     char buf[8192] = {0};
//     int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    
//     if (ret <= 0) {
//         httpd_resp_set_type(req, "application/json");
//         httpd_resp_send(req, "{\"ok\":false,\"error\":\"Invalid request\"}", -1);
//         return ESP_FAIL;
//     }

//     cJSON *json = cJSON_Parse(buf);
//     if (!json) {
//         httpd_resp_set_type(req, "application/json");
//         httpd_resp_send(req, "{\"ok\":false,\"error\":\"Invalid JSON\"}", -1);
//         return ESP_FAIL;
//     }

//     // Extract payload
//     cJSON *macro_count_item = cJSON_GetObjectItem(json, "macro_count");
//     cJSON *tag_macro_item = cJSON_GetObjectItem(json, "tag_macro");
//     cJSON *macros_item = cJSON_GetObjectItem(json, "macros");

//     if (!macro_count_item || !tag_macro_item || !macros_item) {
//         cJSON_Delete(json);
//         httpd_resp_set_type(req, "application/json");
//         httpd_resp_send(req, "{\"ok\":false,\"error\":\"Missing required fields\"}", -1);
//         return ESP_FAIL;
//     }

//     int macro_count = macro_count_item->valueint;
//     int tag_macro = tag_macro_item->valueint;

//     // Validate input
//     if (macro_count < 0 || macro_count > MAX_MACROS || tag_macro >= macro_count) {
//         cJSON_Delete(json);
//         httpd_resp_set_type(req, "application/json");
//         httpd_resp_send(req, "{\"ok\":false,\"error\":\"Invalid macro count or tag_macro\"}", -1);
//         return ESP_FAIL;
//     }

//     // Update config
//     macro_manager->config.macro_count = macro_count;
//     macro_manager->config.tag_macro = tag_macro;

//     // Deserialize each macro
//     cJSON *macro_array = cJSON_GetObjectItem(json, "macros");
//     cJSON *macro_item = NULL;
//     int idx = 0;

//     cJSON_ArrayForEach(macro_item, macro_array) {
//         if (idx >= macro_count || idx >= MAX_MACROS) break;

//         if (!json_to_macro(macro_item, macro_manager->config.macros[idx])) {
//             ESP_LOGW("MACRO", "Failed to deserialize macro %d", idx);
//         }

//         idx++;
//     }

//     // Save to NVS via MacroConfigManager
//     macro_manager->saveAll();

//     cJSON_Delete(json);

//     // Send success response
//     httpd_resp_set_type(req, "application/json");
//     httpd_resp_send(req, "{\"ok\":true}", -1);
    
//     ESP_LOGI("HTTP", "Macros saved successfully (%u macros)", macro_count);
//     macro_manager->printConfig();

//     return ESP_OK;
// }

// // ============ HTTP SERVER SETUP ============
// httpd_handle_t start_usb_config_server(MacroConfigManager& manager) {
//     // Store reference to macro manager
//     macro_manager = &manager;

//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//     config.max_uri_handlers = 10;
//     config.stack_size = 8192;
//     config.uri_match_fn = httpd_uri_match_wildcard;

//     httpd_handle_t server = NULL;
    
//     if (httpd_start(&server, &config) == ESP_OK) {
//         ESP_LOGI("HTTP", "HTTP server started");

//         // Serve static files
//         httpd_uri_t uri_root = {
//             .uri = "/",
//             .method = HTTP_GET,
//             .handler = static_file_handler,
//             .user_ctx = NULL
//         };
//         httpd_register_uri_handler(server, &uri_root);

//         httpd_uri_t uri_static = {
//             .uri = "/*",
//             .method = HTTP_GET,
//             .handler = static_file_handler,
//             .user_ctx = NULL
//         };
//         httpd_register_uri_handler(server, &uri_static);

//         // API endpoints
//         httpd_uri_t uri_nonce = {
//             .uri = "/api/nonce",
//             .method = HTTP_GET,
//             .handler = nonce_handler,
//             .user_ctx = NULL
//         };
//         httpd_register_uri_handler(server, &uri_nonce);

//         httpd_uri_t uri_macros_get = {
//             .uri = "/api/macros",
//             .method = HTTP_GET,
//             .handler = macros_get_handler,
//             .user_ctx = NULL
//         };
//         httpd_register_uri_handler(server, &uri_macros_get);

//         httpd_uri_t uri_macros_post = {
//             .uri = "/api/macros",
//             .method = HTTP_POST,
//             .handler = macros_post_handler,
//             .user_ctx = NULL
//         };
//         httpd_register_uri_handler(server, &uri_macros_post);

//         ESP_LOGI("HTTP", "USB config server ready");
//     } else {
//         ESP_LOGE("HTTP", "Failed to start HTTP server");
//     }

//     return server;
// }

// // ============ CLEANUP ============
// void stop_usb_config_server(httpd_handle_t srv) {
//     if (srv) {
//         httpd_stop(srv);
//         macro_manager = NULL;
//         ESP_LOGI("HTTP", "HTTP server stopped");
//     }
// }