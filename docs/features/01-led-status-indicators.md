---
title: LED Status Indicators
parent: Feature Guides
nav_order: 1
---

# Feature: LED Status Indicators

**Complexity**: 🟢 Low
**Hardware Required**: ✅ None (uses onboard LED)
**User Value**: ⭐⭐⭐ Essential

## Overview

Use the ESP32-C3 SuperMini's onboard blue LED (GPIO8) to provide visual feedback about system status, card reads, and access decisions. The ESP32-C3 also supports additional GPIO pins for external LEDs if needed.

## Benefits

- Immediate visual feedback without audio
- Helps with debugging and system monitoring
- Low power consumption
- No additional hardware required
- Works in noisy environments where audio might be missed

## Implementation Checklist

### Phase 1: Basic LED Controller
- [X] Create `LEDController` class in `include/led_controller.h`
- [ ] Implement LED control methods:
  - [ ] `void begin()` - Initialize LED pins
  - [ ] `void setSystemReady()` - Solid LED to show system ready
  - [ ] `void setScanning()` - Blink pattern during card read
  - [ ] `void setAccessGranted()` - Fast blink or solid pattern
  - [ ] `void setAccessDenied()` - Rapid flash pattern
  - [ ] `void setError()` - Distinct error pattern

### Phase 2: LED Patterns
- [X] Define LED pattern constants:
  ```cpp
  enum LEDPattern {
    PATTERN_OFF,
    PATTERN_SOLID,
    PATTERN_SLOW_BLINK,
    PATTERN_FAST_BLINK,
    PATTERN_DOUBLE_BLINK,
    PATTERN_TRIPLE_BLINK,
    PATTERN_SOS,
    PATTERN_BREATHING  // ESP32 PWM feature
  };
  ```
- [X] Implement non-blocking pattern generator using millis()
- [X] Add pattern queue for sequential patterns
- [X] Use ESP32 hardware PWM for smooth effects

### Phase 3: Status Indicators
- [ ] **System States**:
  - [X] Power on: Smooth fade in/out (using PWM)
  - [ ] Ready: Slow breathing pattern (In consideration due to the project being battery based)
  - [ ] WiFi connecting: Double blink pattern (WiFi not integrated yet)
  - [ ] WiFi connected: Triple blink then solid (WiFi not integrated yet)
  - [ ] Waiting (after 10s): Double blink every 2 seconds (In consideration due to the project being battery based)
  - [ ] Card detected: Rapid blink (This would run repeatedly, not being implemented)

- [ ] **Access States**:
  - [X] Valid card: 3 fast blinks then solid for 2 seconds
  - [X] Invalid card: 5 very rapid flashes
  - [ ] Brute force lockout: SOS pattern
  - [ ] Door unlocked: Solid LED while relay active

- [ ] **Error States**:
  - [ ] RFID reader error: Continuous rapid blink
  - [ ] Audio player error: Long-short-long pattern
  - [ ] WiFi error: Slow double blink

### Phase 4: Integration
- [X] Add `LEDController` instance to `main.cpp` (Not added to main, but implemented in access_service, effectively the same)
- [ ] Call LED methods at appropriate points:
  - [X] `setup()`: Show initialization pattern (Again, implemented in access_service, which is the only loop in main())
  - [ ] Card read attempts (Card reads are too frequent for LED feedback, card reads multiple times per second)
  - [X] Access granted/denied events
  - [ ] Error conditions 
  - [ ] WiFi/BLE status changes (WiFi/Bluetooth not yet implemented)
- [X] Ensure LED patterns don't block main loop

### Phase 5: Advanced Features (Optional)
- [X] Use ESP32 LEDC (PWM) for smooth fading
- [X] Multiple LED support via additional GPIOs
- [ ] Configurable brightness levels
- [ ] Custom pattern builder via serial/web commands
- [X] WS2812B addressable LED strip support (Single LED Currently)

## Example Code Structure

```cpp
class LEDController {
public:
    LEDController(uint8_t led_pin = 8); // GPIO8 for blue LED
    void begin();
    void update(); // Call in loop() for non-blocking patterns
    void setPattern(LEDPattern pattern);
    void playSequence(LEDPattern* patterns, uint8_t count);
    void setBrightness(uint8_t level); // 0-255 using PWM

private:
    uint8_t pin;
    LEDPattern current_pattern;
    unsigned long last_update;
    uint8_t pattern_state;
    uint8_t brightness;

    // ESP32 PWM specific
    uint8_t pwm_channel;
    void setupPWM();
};
```

## ESP32-C3 Specific Features

```cpp
// ESP32-C3 PWM LED control
void LEDController::setupPWM() {
    // Configure PWM functionalitites
    ledcSetup(pwm_channel, 5000, 8); // 5kHz, 8-bit resolution
    ledcAttachPin(pin, pwm_channel);
}

void LEDController::setBrightness(uint8_t level) {
    ledcWrite(pwm_channel, level);
}

// Smooth breathing effect using PWM
void LEDController::breathe() {
    static uint8_t brightness = 0;
    static int8_t direction = 1;

    brightness += direction;
    if (brightness == 255 || brightness == 0) {
        direction = -direction;
    }

    ledcWrite(pwm_channel, brightness);
}
```

## Testing Checklist

- [X] Test all LED patterns individually
- [X] Verify patterns are non-blocking
- [ ] Test pattern transitions
- [ ] Verify correct patterns for each system state
- [ ] Test LED visibility in different lighting conditions
- [ ] Test PWM brightness levels
- [ ] Measure current consumption with LEDs active
- [ ] Test error condition indicators
- [ ] Verify patterns work during WiFi/BLE operations (Neither Implemented Yet)

## Future Enhancements

- External WS2812B LED strip for detailed status
- Different colors for different access levels (RGB LED) (In Progress...)
- LED matrix display for card UID display
- Light sensor for automatic brightness (using ADC)
- Synchronize LED patterns with audio feedback (Patterns currently run before Audio)
- Web interface for custom pattern configuration
