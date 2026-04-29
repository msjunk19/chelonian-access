// #pragma once

// #include <Arduino.h>
// #include <driver/gpio.h>
// #include "esp_log.h"

// enum class BootMode {
//     NORMAL,
//     USB_CONFIG
// };

// class BootModeDetector {
// private:
//     static const char* TAG;
//     static constexpr gpio_num_t BOOT_BUTTON_PIN = GPIO_NUM_0;  // GPIO0 is boot button
//     static constexpr uint32_t HOLD_TIME_MS = 2000;              // 2 second hold requirement
//     static constexpr uint32_t DEBOUNCE_MS = 50;
    
//     BootMode detectedMode;
    
// public:
//     BootModeDetector() : detectedMode(BootMode::NORMAL) {}
    
//     /**
//      * Detect boot mode by checking if boot button is held during startup
//      * Call this very early in setup(), before any other initialization
//      * 
//      * Returns: BootMode::USB_CONFIG if button held for 2+ seconds
//      *          BootMode::NORMAL if button not pressed or released early
//      */
//     BootMode detect() {
//         ESP_LOGI(TAG, "Detecting boot mode...");
        
//         // Configure boot button as input with pull-up
//         gpio_config_t io_conf = {
//             .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
//             .mode = GPIO_MODE_INPUT,
//             .pull_up_en = GPIO_PULLUP_ENABLE,
//             .pull_down_en = GPIO_PULLDOWN_DISABLE,
//             .intr_type = GPIO_INTR_DISABLE
//         };
//         gpio_config(&io_conf);
        
//         // Stabilize pin reading
//         delay(10);
        
//         // Boot button pulls GPIO0 LOW when pressed
//         if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
//             ESP_LOGI(TAG, "Boot button detected - monitoring hold time...");
            
//             uint32_t pressStartTime = millis();
//             uint32_t lastStatusTime = pressStartTime;
            
//             // Monitor button while held
//             while (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
//                 uint32_t holdTime = millis() - pressStartTime;
                
//                 // Print status every 500ms
//                 if (millis() - lastStatusTime >= 500) {
//                     ESP_LOGI(TAG, "Holding... %lu ms", holdTime);
//                     lastStatusTime = millis();
//                 }
                
//                 // Check if threshold reached
//                 if (holdTime >= HOLD_TIME_MS) {
//                     detectedMode = BootMode::USB_CONFIG;
//                     ESP_LOGI(TAG, "Hold threshold reached!");
                    
//                     // Wait for release
//                     while (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
//                         delay(DEBOUNCE_MS);
//                     }
//                     ESP_LOGI(TAG, "Button released");
//                     delay(100);
//                     printBootMode();
//                     return detectedMode;
//                 }
                
//                 delay(DEBOUNCE_MS);
//             }
            
//             // Released before threshold
//             ESP_LOGI(TAG, "Button released before threshold");
//             detectedMode = BootMode::NORMAL;
//         } else {
//             ESP_LOGI(TAG, "Boot button not pressed");
//             detectedMode = BootMode::NORMAL;
//         }
        
//         printBootMode();
//         return detectedMode;
//     }
    
//     /**
//      * Get the detected boot mode without re-detecting
//      */
//     BootMode getMode() const {
//         return detectedMode;
//     }
    
//     /**
//      * Check if we're in USB config mode
//      */
//     bool isUSBConfigMode() const {
//         return detectedMode == BootMode::USB_CONFIG;
//     }
    
//     /**
//      * Check if we're in normal mode
//      */
//     bool isNormalMode() const {
//         return detectedMode == BootMode::NORMAL;
//     }
    
//     /**
//      * Print the detected mode to log
//      */
//     void printBootMode() const {
//         ESP_LOGI(TAG, "=== BOOT MODE: %s ===",
//             detectedMode == BootMode::USB_CONFIG ? "USB_CONFIG" : "NORMAL");
//     }
// };

// // Static member initialization
// const char* BootModeDetector::TAG = "BootMode";
