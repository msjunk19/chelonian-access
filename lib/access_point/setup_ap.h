#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "config.hpp"
#include <DNSServer.h>

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
// Start AP
inline void startAP() {
    char ssid[SSID_LEN + 1];
    char pass[PASS_LEN + 1];

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

        WiFi.softAP(ssid, pass);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
        server.begin();
    } else {
        WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Redirect all DNS to AP
        server.begin();
    }
}


// -------------------------
// Web server + captive portal
inline void setupWebServer() {

    // Main setup page
    server.on("/", []() {
        server.send(200, "text/html",
                    "<h1>Device Setup</h1>"
                    "<form method='POST' action='/save'>"
                    "SSID: <input name='ssid'><br>"
                    "Password: <input name='pass'><br>"
                    "<input type='submit' value='Save'>"
                    "</form>");
    });

    // Save credentials
    server.on("/save", []() {
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
}

// -------------------------
// Loop handler
inline void handleClient() {
    dnsServer.processNextRequest();
    server.handleClient();
}