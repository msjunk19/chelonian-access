#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "config.hpp"
#include <DNSServer.h>
#include <LittleFS.h>

const byte DNS_PORT = 53;
DNSServer dnsServer;

WebServer server(80);

// Hardcoded lengths
constexpr uint8_t SSID_LEN = 32;
constexpr uint8_t PASS_LEN = 32;

// -------------------------
// EEPROM helpers
inline bool isSetupComplete() {
    return EEPROM.read(AP_MAGIC_ADDR) == AP_MAGIC_VALUE;
}

inline void saveAPCredentials(const String &ssid, const String &pass) {
    for (int i = 0; i < SSID_LEN; i++) {
        char c = i < ssid.length() ? ssid[i] : 0;
        EEPROM.write(AP_SSID_ADDR + i, c);
    }

    for (int i = 0; i < PASS_LEN; i++) {
        char c = i < pass.length() ? pass[i] : 0;
        EEPROM.write(AP_PASS_ADDR + i, c);
    }

    EEPROM.write(AP_MAGIC_ADDR, AP_MAGIC_VALUE);
    EEPROM.commit();
}

// -------------------------
// LittleFS helpers
inline bool initFileSystem() {
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        return false;
    }
    Serial.println("LittleFS mounted successfully");
    return true;
}

inline bool safeLittleFSBegin() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed, formatting...");
        if (!LittleFS.format()) {
            Serial.println("LittleFS format failed!");
            return false;
        }
        if (!LittleFS.begin()) {
            Serial.println("LittleFS still failed after format!");
            return false;
        }
    }
    return true;
}

// Helper to serve a file from LittleFS
inline void serveFile(const char* path, const char* type) {
    server.on(path, HTTP_GET, [=]() {
        File file = LittleFS.open(path, "r");
        if (!file) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(file, type);
        file.close();
    });
}

// -------------------------
// Start AP
inline void startAP() {

    // if (!initFileSystem()) {
    //     // Minimal fallback AP if FS fails
    //     WiFi.softAP("AC_FALLBACK_AP");
    //     server.begin();
    //     return;
    // }
    if (!safeLittleFSBegin()) {
        // Minimal fallback AP if FS fails
        Serial.println("Starting fallback AP due to LittleFS issues");
        WiFi.softAP("AC_FALLBACK_AP");
        server.begin();   // must still call this
        return;
    }

    char ssid[SSID_LEN + 1];
    char pass[PASS_LEN + 1];

        // What do we do if we find WiFi Credentials in the EEPROM?
    if (isSetupComplete()) {
        for (int i = 0; i < SSID_LEN; i++)
            ssid[i] = EEPROM.read(AP_SSID_ADDR + i);
        ssid[SSID_LEN] = 0;

        for (int i = 0; i < PASS_LEN; i++)
            pass[i] = EEPROM.read(AP_PASS_ADDR + i);
        pass[PASS_LEN] = 0;

        // -------- SAFETY CHECK --------
        if (strlen(pass) > 0 && strlen(pass) < 8) {
            Serial.println("Invalid stored password, reverting to default AP");
            WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS);
            dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
            server.begin();
            return;
        }
        // ------------------------------
            // Start User Configured AP
        WiFi.softAP(ssid, pass);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
        server.begin();
    } else {
        // No User Configured AP, Start Default

        // WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS); //Default AP, with Password
        WiFi.softAP("AC_FALLBACK_AP"); // SSID for the portal
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
        server.begin();
    }
}
// inline void startAP() {
//     char ssid[SSID_LEN + 1];
//     char pass[PASS_LEN + 1];

//     if (isSetupComplete()) {
//         for (int i = 0; i < SSID_LEN; i++)
//             ssid[i] = EEPROM.read(AP_SSID_ADDR + i);
//         ssid[SSID_LEN] = 0;

//         for (int i = 0; i < PASS_LEN; i++)
//             pass[i] = EEPROM.read(AP_PASS_ADDR + i);
//         pass[PASS_LEN] = 0;

//         WiFi.softAP(ssid, pass);
//     } else {
//         // WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS);
//           WiFi.softAP("ESP32-Portal"); // SSID for the portal
//             dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
//             server.begin();
//     }
// }

    // -------------------------
    // Web server setup
    inline void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");
        if (!file) {
            server.send(200, "text/html", "<h1>Fallback page</h1>");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/style.css", HTTP_GET, []() {
        File file = LittleFS.open("/style.css", "r");
        if (!file) {
            server.send(404, "text/css", "");
            return;
        }
        server.streamFile(file, "text/css");
        file.close();
    });

    server.on("/script.js", HTTP_GET, []() {
        File file = LittleFS.open("/script.js", "r");
        if (!file) {
            server.send(404, "application/javascript", "");
            return;
        }
        server.streamFile(file, "application/javascript");
        file.close();
    });
    // Save credentials handler
    server.on("/save", HTTP_POST, []() {
        if (server.hasArg("ssid") && server.hasArg("pass")) {
            String ssid = server.arg("ssid");
            String pass = server.arg("pass");

            if (ssid.length() > 0 && pass.length() > 0) {
                saveAPCredentials(ssid, pass);
                server.send(200, "text/html", "<h1>Saved! Restarting...</h1>");
                delay(1000);
                ESP.restart();
            } else {
                server.send(400, "text/html", "<h1>Error: SSID/Password required</h1>");
            }
        } else {
            server.send(400, "text/html", "<h1>Error: Invalid request</h1>");
        }
    });



    // -------------------------
    // Captive portal detection endpoints

    // Android
    server.on("/generate_204", []() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    // iOS
    server.on("/hotspot-detect.html", []() {
        server.send(200, "text/html",
                    "<html><body><script>window.location.href='/'</script></body></html>");
    });

    // Windows
    server.on("/connecttest.txt", []() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    // Catch-all (important)
    server.onNotFound([]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.begin();
    Serial.println("Web server started");
}

// -------------------------
// Loop handler
inline void handleClient() {
    dnsServer.processNextRequest();
    server.handleClient();
}

// #pragma once
// #include <WiFi.h>
// #include <WebServer.h>
// #include <EEPROM.h>
// #include "config.hpp"
// #include <DNSServer.h>

// const byte DNS_PORT = 53;
// DNSServer dnsServer;

// WebServer server(80);

// // Hardcoded lengths
// constexpr uint8_t SSID_LEN = 32;
// constexpr uint8_t PASS_LEN = 32;

// // -------------------------
// // EEPROM helpers
// inline bool isSetupComplete() {
//     return EEPROM.read(AP_MAGIC_ADDR) == AP_MAGIC_VALUE;
// }

// inline void saveAPCredentials(const String &ssid, const String &pass) {
//     for (int i = 0; i < SSID_LEN; i++) {
//         char c = i < ssid.length() ? ssid[i] : 0;
//         EEPROM.write(AP_SSID_ADDR + i, c);
//     }

//     for (int i = 0; i < PASS_LEN; i++) {
//         char c = i < pass.length() ? pass[i] : 0;
//         EEPROM.write(AP_PASS_ADDR + i, c);
//     }

//     EEPROM.write(AP_MAGIC_ADDR, AP_MAGIC_VALUE);
//     EEPROM.commit();
// }

// // -------------------------
// // Start AP
// inline void startAP() {
//     char ssid[SSID_LEN + 1];
//     char pass[PASS_LEN + 1];

//         // What do we do if we find WiFi Credentials in the EEPROM?
//     if (isSetupComplete()) {
//         for (int i = 0; i < SSID_LEN; i++)
//             ssid[i] = EEPROM.read(AP_SSID_ADDR + i);
//         ssid[SSID_LEN] = 0;

//         for (int i = 0; i < PASS_LEN; i++)
//             pass[i] = EEPROM.read(AP_PASS_ADDR + i);
//         pass[PASS_LEN] = 0;

//         // -------- SAFETY CHECK --------
//         if (strlen(pass) > 0 && strlen(pass) < 8) {
//             Serial.println("Invalid stored password, reverting to default AP");
//             WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS);
//             dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
//             server.begin();
//             return;
//         }
//         // ------------------------------
//             // Start User Configured AP
//         WiFi.softAP(ssid, pass);
//         dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
//         server.begin();
//     } else {
//         // No User Configured AP, Start Default

//         // WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS); //Default AP, with Password
//         WiFi.softAP("AC_FALLBACK_AP"); // SSID for the portal
//         dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
//         server.begin();
//     }
// }
// // inline void startAP() {
// //     char ssid[SSID_LEN + 1];
// //     char pass[PASS_LEN + 1];

// //     if (isSetupComplete()) {
// //         for (int i = 0; i < SSID_LEN; i++)
// //             ssid[i] = EEPROM.read(AP_SSID_ADDR + i);
// //         ssid[SSID_LEN] = 0;

// //         for (int i = 0; i < PASS_LEN; i++)
// //             pass[i] = EEPROM.read(AP_PASS_ADDR + i);
// //         pass[PASS_LEN] = 0;

// //         WiFi.softAP(ssid, pass);
// //     } else {
// //         // WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS);
// //           WiFi.softAP("ESP32-Portal"); // SSID for the portal
// //             dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
// //             server.begin();
// //     }
// // }

// // -------------------------
// // Web server + captive portal
// inline void setupWebServer() {

//     // Main setup page
//     server.on("/", []() {
//         server.send(200, "text/html",
//                     "<h1>Device Setup</h1>"
//                     "<form method='POST' action='/save'>"
//                     "SSID: <input name='ssid'><br>"
//                     "Password: <input name='pass'><br>"
//                     "<input type='submit' value='Save'>"
//                     "</form>");
//     });

//     // Save credentials
//     server.on("/save", []() {
//         if (server.hasArg("ssid") && server.hasArg("pass")) {
//             String ssid = server.arg("ssid");
//             String pass = server.arg("pass");

//             if (ssid.length() > 0 && pass.length() > 0) {
//                 saveAPCredentials(ssid, pass);
//                 server.send(200, "text/html", "<h1>Saved! Restarting...</h1>");
//                 delay(1000);
//                 ESP.restart();
//             } else {
//                 server.send(400, "text/html", "<h1>Error: SSID/Password required</h1>");
//             }
//         } else {
//             server.send(400, "text/html", "<h1>Error: Invalid request</h1>");
//         }
//     });

//     // -------------------------
//     // Captive portal detection endpoints

//     // Android
//     server.on("/generate_204", []() {
//         server.sendHeader("Location", "/", true);
//         server.send(302, "text/plain", "");
//     });

//     // iOS
//     server.on("/hotspot-detect.html", []() {
//         server.send(200, "text/html",
//                     "<html><body><script>window.location.href='/'</script></body></html>");
//     });

//     // Windows
//     server.on("/connecttest.txt", []() {
//         server.sendHeader("Location", "/", true);
//         server.send(302, "text/plain", "");
//     });

//     // Catch-all (important)
//     server.onNotFound([]() {
//         server.sendHeader("Location", "/", true);
//         server.send(302, "text/plain", "");
//     });

//     server.begin();
// }

// // -------------------------
// // Loop handler
// inline void handleClient() {
//     dnsServer.processNextRequest();
//     server.handleClient();
// }