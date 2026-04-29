#include <Arduino.h>
#include <cstdlib>
#include <exception>
#include <esp_log.h>

#include "exception_handler.h"

static const char* TAG = "ExceptionHandler";

void globalExceptionHandler() {
    ESP_LOGE(TAG, "\n\n=== UNHANDLED EXCEPTION ===");

    // Print exception info if available
    if (std::current_exception()) {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "Exception Type: %s", e.what());
        } catch (...) {
            ESP_LOGE(TAG, "Unknown exception type");
        }
    }

// Print stack trace if available (ESP32 specific)
#ifdef ESP32
    ESP_LOGE(TAG, "\nStack Trace:");
#endif

    ESP_LOGE(TAG, "\nSystem will restart in 1 second to recover from unhandled exception...");
    // A short delay to allow any buffered log messages to be sent over serial before restart.
#ifndef UNIT_TEST
    delay(1000);
    ESP.restart();  // Perform a software reset to recover from the unhandled exception.
#endif
}

#ifndef UNIT_TEST
void setupGlobalExceptionHandler() {
    std::set_terminate(globalExceptionHandler);
}
#else
#include <cstdio>  // For printf
void setupGlobalExceptionHandler() {
    printf("[Mock Exception Handler] Global exception handler setup skipped for unit test.\n");
    // In a real mock, you might set a flag or a mock object to track calls.
}
#endif
