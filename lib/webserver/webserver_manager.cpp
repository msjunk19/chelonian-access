#include "webserver_manager.h"
#include <Arduino.h>

void initWebServer(AsyncWebServer &server) {

    // Serve HTML pages
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    // API endpoint to simulate card scan
    server.on("/api/simulate", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("Simulated valid card scan");
        // Call your access logic here
        request->send(200, "text/plain", "Access Granted");
    });

    // Add more routes here later
    // server.on("/logs", HTTP_GET, ...);

    server.begin();
}