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
//     static constexpr gpio_num_t BOOT_BUTTON_PIN = GPIO_NUM_9;  // ESP32-C3 uses GPIO9
//     static constexpr uint32_t HOLD_TIME_MS = 2000;
//     static constexpr uint32_t DEBOUNCE_MS = 50;
    
//     BootMode detectedMode;
    
// public:
//     BootModeDetector() : detectedMode(BootMode::NORMAL) {}
    
//     BootMode detect() {
//         Serial.println("\n\n========== BOOT MODE DETECTION START ==========");
//         Serial.println("ESP32-C3 Boot Button (GPIO9) Detection");
//         Serial.printf("Hold time threshold: %lu ms\n", HOLD_TIME_MS);
//         Serial.println("Configuring GPIO9...");
        
//         // Configure boot button as input with pull-up
//         gpio_config_t io_conf = {
//             .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
//             .mode = GPIO_MODE_INPUT,
//             .pull_up_en = GPIO_PULLUP_ENABLE,
//             .pull_down_en = GPIO_PULLDOWN_DISABLE,
//             .intr_type = GPIO_INTR_DISABLE
//         };
        
//         esp_err_t ret = gpio_config(&io_conf);
//         Serial.printf("GPIO config result: %d (0=success)\n", ret);
        
//         // Stabilize pin reading
//         delay(10);
        
//         // Read initial pin state
//         int pinLevel = gpio_get_level(BOOT_BUTTON_PIN);
//         Serial.printf("Initial pin level: %d (0=pressed, 1=not pressed)\n", pinLevel);
        
//         // Boot button pulls GPIO9 LOW when pressed
//         if (pinLevel == 0) {
//             Serial.println("\n>>> BOOT BUTTON DETECTED AS PRESSED <<<");
//             Serial.println("Monitoring hold duration...");
            
//             uint32_t pressStartTime = millis();
//             uint32_t lastStatusTime = pressStartTime;
//             uint32_t maxHoldDetected = 0;
            
//             // Monitor button while held
//             while (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
//                 uint32_t holdTime = millis() - pressStartTime;
//                 maxHoldDetected = holdTime;
                
//                 // Print status every 500ms
//                 if (millis() - lastStatusTime >= 500) {
//                     Serial.printf("  [%lu ms] Still holding...\n", holdTime);
//                     lastStatusTime = millis();
//                 }
                
//                 // Check if threshold reached
//                 if (holdTime >= HOLD_TIME_MS) {
//                     detectedMode = BootMode::USB_CONFIG;
//                     Serial.printf("\n!!! HOLD THRESHOLD REACHED (%lu ms) !!!\n", holdTime);
//                     Serial.println("Setting mode to: USB_CONFIG");
                    
//                     // Wait for release
//                     Serial.println("Waiting for button release...");
//                     while (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
//                         delay(DEBOUNCE_MS);
//                     }
//                     Serial.println("Button released");
//                     delay(100);
//                     printBootMode();
//                     Serial.println("========== BOOT MODE DETECTION END ==========\n");
//                     return detectedMode;
//                 }
                
//                 delay(DEBOUNCE_MS);
//             }
            
//             // Released before threshold
//             Serial.printf("\n!!! BUTTON RELEASED EARLY (only %lu ms hold) !!!\n", maxHoldDetected);
//             detectedMode = BootMode::NORMAL;
//         } else {
//             Serial.println("Boot button NOT pressed (pin is HIGH)");
//             detectedMode = BootMode::NORMAL;
//         }
        
//         printBootMode();
//         Serial.println("========== BOOT MODE DETECTION END ==========\n");
//         return detectedMode;
//     }
    
//     BootMode getMode() const {
//         return detectedMode;
//     }
    
//     bool isUSBConfigMode() const {
//         return detectedMode == BootMode::USB_CONFIG;
//     }
    
//     bool isNormalMode() const {
//         return detectedMode == BootMode::NORMAL;
//     }
    
//     void printBootMode() const {
//         const char* modeStr = (detectedMode == BootMode::USB_CONFIG) ? "USB_CONFIG" : "NORMAL";
//         Serial.printf("\n>>> FINAL BOOT MODE: %s <<<\n", modeStr);
//     }
// };

// const char* BootModeDetector::TAG = "BootMode";