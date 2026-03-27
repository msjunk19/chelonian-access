#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "config.hpp"
#include <wifi_auth_endpoints.hpp>

static const char* WIFICFG_TAG    = "WIFICFG";
static const char* WIFICFG_NVS_NS = "wifi_cfg";

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

constexpr uint8_t SSID_LEN = 32;
constexpr uint8_t PASS_LEN = 64;

// -------------------------
// NVS helpers

inline bool isSetupComplete() {
    Preferences prefs;
    prefs.begin(WIFICFG_NVS_NS, true);
    bool exists = prefs.isKey("ap_ssid");
    prefs.end();
    return exists;
}

inline void saveAPCredentials(const String& ssid, const String& pass) {
    Preferences prefs;
    prefs.begin(WIFICFG_NVS_NS, false);
    prefs.putString("ap_ssid", ssid);
    prefs.putString("ap_pass", pass);
    prefs.end();
    ESP_LOGI(WIFICFG_TAG, "AP credentials saved");
}

inline bool loadAPCredentials(char* ssid, char* pass) {
    Preferences prefs;
    prefs.begin(WIFICFG_NVS_NS, true);
    String s = prefs.getString("ap_ssid", "");
    String p = prefs.getString("ap_pass", "");
    prefs.end();

    if (s.length() == 0) return false;

    strncpy(ssid, s.c_str(), SSID_LEN); ssid[SSID_LEN] = 0;
    strncpy(pass, p.c_str(), PASS_LEN); pass[PASS_LEN] = 0;
    return true;
}

inline void clearAPCredentials() {
    Preferences prefs;
    prefs.begin(WIFICFG_NVS_NS, false);
    prefs.clear();
    prefs.end();
    ESP_LOGI(WIFICFG_TAG, "AP credentials cleared");
}

// -------------------------
// LittleFS

inline bool safeLittleFSBegin() {
    if (!LittleFS.begin()) {
        ESP_LOGW(WIFICFG_TAG, "LittleFS mount failed, formatting...");
        if (!LittleFS.format()) {
            ESP_LOGE(WIFICFG_TAG, "LittleFS format failed!");
            return false;
        }
        if (!LittleFS.begin()) {
            ESP_LOGE(WIFICFG_TAG, "LittleFS still failed after format!");
            return false;
        }
    }
    return true;
}

// -------------------------
// Start AP

inline void startAP() {
    if (!safeLittleFSBegin()) {
        ESP_LOGE(WIFICFG_TAG, "Starting fallback AP due to LittleFS failure");
        WiFi.softAP("AC_FALLBACK_AP");
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); 
        // dnsServer.start(DNS_PORT, DOMAIN, WiFi.softAPIP());
        // dnsServer.start(DNS_PORT, "chelonian.local", WiFi.softAPIP());


        server.begin();
        return;
    }

    char ssid[SSID_LEN + 1];
    char pass[PASS_LEN + 1];

    if (loadAPCredentials(ssid, pass)) {
        size_t passLen = strlen(pass);

        // WPA2 passwords must be 8-63 chars or absent (open network)
        if (passLen > 0 && passLen < 8) {
            ESP_LOGW(WIFICFG_TAG, "Invalid stored password, reverting to default AP");
            WiFi.softAP(DEFAULT_SSID, DEFAULT_PASS);
            dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
            // dnsServer.start(DNS_PORT, DOMAIN, WiFi.softAPIP());
            // dnsServer.start(DNS_PORT, "chelonian.local", WiFi.softAPIP());

            server.begin();
            return;
        }

        WiFi.softAP(ssid, passLen > 0 ? pass : nullptr);
        ESP_LOGI(WIFICFG_TAG, "Started user AP: %s", ssid);
    } else {
        WiFi.softAP("AC_FALLBACK_AP");
        ESP_LOGI(WIFICFG_TAG, "Started fallback AP");
    }

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    // dnsServer.start(DNS_PORT, DOMAIN, WiFi.softAPIP());
    // dnsServer.start(DNS_PORT, "chelonian.local", WiFi.softAPIP());


    server.begin();
}

// -------------------------
// Web server

inline void setupWebServer(std::function<void(PhoneCommand)> onCommand) {

    // Root — serve control page if configured, setup page if not
    server.on("/", HTTP_GET, []() {
        const char* path = isSetupComplete() ? "/control.html" : "/index.html";
        File file = LittleFS.open(path, "r");
        if (!file) {
            server.send(200, "text/html",
                isSetupComplete()
                    ? "<h1>User AP Configured. User page missing!</h1>"
                    : "<h1>Default AP. Setup page missing!</h1>");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/style.css", HTTP_GET, []() {
        File file = LittleFS.open("/style.css", "r");
        if (!file) { server.send(404, "text/css", ""); return; }
        server.streamFile(file, "text/css");
        file.close();
    });

    server.on("/script.js", HTTP_GET, []() {
        File file = LittleFS.open("/script.js", "r");
        if (!file) { server.send(404, "application/javascript", ""); return; }
        server.streamFile(file, "application/javascript");
        file.close();
    });

    server.on("/204", []() {                            // Steam
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    // Save AP credentials (setup page form submission)
    server.on("/save", HTTP_POST, []() {
        if (!server.hasArg("ssid") || !server.hasArg("pass")) {
            server.send(400, "text/html", "<h1>Error: Invalid request</h1>");
            return;
        }

        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        if (ssid.length() == 0 || pass.length() == 0) {
            server.send(400, "text/html", "<h1>Error: SSID/Password required</h1>");
            return;
        }

        saveAPCredentials(ssid, pass);
        server.send(200, "text/html", "<h1>Saved! Restarting...</h1>");
        delay(1000);
        ESP.restart();
    });

    // -------------------------
    // Auth endpoints (/pair, /cmd, /unpair)
    setupAuthEndpoints(onCommand);

    // -------------------------
    // Captive portal detection

    server.on("/generate_204", []() {           // Android
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.on("/hotspot-detect.html", []() {    // iOS
        server.send(200, "text/html",
            "<html><body><script>window.location.href='/'</script></body></html>");
    });

    server.on("/connecttest.txt", []() {        // Windows
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

        // Fix existing handlers — add a body to all redirects
    server.on("/generate_204", []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/html", "<a href='http://192.168.4.1/'>Click here</a>");
    });

    server.on("/hotspot-detect.html", []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/html", "<a href='http://192.168.4.1/'>Click here</a>");
    });

    server.on("/connecttest.txt", []() {
        server.send(200, "text/plain", "Microsoft Connect Test");
    });

    server.on("/204", []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/html", "<a href='http://192.168.4.1/'>Click here</a>");
    });

    // New handlers
    server.on("/redirect", []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/html", "<a href='http://192.168.4.1/'>Click here</a>");
    });

    server.on("/favicon.ico", []() {
        server.send(204, "text/plain", "");
    });

    server.on("/ipv6check", []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/html", "<a href='http://192.168.4.1/'>Click here</a>");
    });

    // Catch-all — 403 on user AP, redirect on setup AP
    server.onNotFound([]() {
        ESP_LOGI(WIFICFG_TAG, "Not found: %s", server.uri().c_str());

        if (isSetupComplete()) {
            File file = LittleFS.open("/403.html", "r");
            if (!file) {
                server.send(403, "text/html",
                    "<h1>403 Forbidden 🐰</h1>"
                    "<p>How did you get down my bunny trail?!</p>");
                return;
            }
            server.streamFile(file, "text/html");
            file.close();
        } else {
            server.sendHeader("Location", "http://192.168.4.1/", true);
            server.send(302, "text/html", "<a href='http://192.168.4.1/'>Click here</a>");
        }
    });

    ESP_LOGI(WIFICFG_TAG, "Web server started");
}

// -------------------------
// Loop handler

inline void handleClient() {
    dnsServer.processNextRequest();
    server.handleClient();
    updatePairingWindow();  // keep pairing window timeout ticking
}