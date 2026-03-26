#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Default NeoPixel colors
namespace LEDColor {
    constexpr uint32_t WHITE  = 0xFFFFFF; // R=255, G=255, B=255
    constexpr uint32_t RED    = 0xFF0000;
    constexpr uint32_t GREEN  = 0x00B400; // 180 green
    constexpr uint32_t BLUE   = 0x0000FF;
    constexpr uint32_t PURPLE = 0xA000A0;

    constexpr uint32_t YELLOW = 0xFFC800; // 255, 200, 0
    // constexpr uint32_t ORANGE = 0xFF6400; // 255, 100, 0
    constexpr uint32_t ORANGE = 0xFF6400; // 255, 10, 0

    constexpr uint32_t AMBER  = 0xFF8C00; // 255, 140, 0
    constexpr uint32_t GOLD   = 0xFFAA14; // 255, 170, 20

    constexpr uint32_t CYAN   = 0x00B4B4; // 0, 180, 180
    constexpr uint32_t PINK   = 0xFF64B4; // 255, 100, 180
}

// namespace LEDColor {
//     constexpr uint32_t WHITE  = 0xFFFFFFFF;
//     constexpr uint32_t RED    = 0xFF0000;
//     constexpr uint32_t GREEN  = 0x00FF00;
//     constexpr uint32_t BLUE   = 0x0000FF;
//     constexpr uint32_t PURPLE = 0x800080;
//     constexpr uint32_t YELLOW = 0xFFFF00;
//     constexpr uint32_t ORANGE = 0xFFA500;
// }

enum LEDPattern {
    PATTERN_NONE,
    PATTERN_SOLID,
    PATTERN_SLOW_BLINK,
    PATTERN_FAST_BLINK,
    PATTERN_DOUBLE_BLINK,
    PATTERN_TRIPLE_BLINK,
    PATTERN_QUAD_BLINK,
    PATTERN_FIVE_BLINK,
    PATTERN_SOS,
    PATTERN_BREATHING
};

class LEDController {
public:
    LEDController(uint8_t led_pin, bool useNeoPixel = false, uint8_t neopixelPin = 0);

    void begin();
    void update();
    void setLED(bool on);

    void setPattern(LEDPattern pattern, uint32_t duration = 3000, uint32_t color = LEDColor::WHITE);
    bool isRunning() const { return m_running; }

    // Pattern queueing
    void enqueuePattern(LEDPattern pattern, uint32_t duration = 3000, uint32_t color = LEDColor::WHITE);
    void enqueuePattern(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color = LEDColor::WHITE);

private:
    // Internal helpers
    void setupPWM();
    void setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color = LEDColor::WHITE);
    void breathe();
    bool m_runOnce;

    // in led_controller3.h — add to private members:
    static const uint8_t MAX_BLINK_COUNT = 10;
    uint16_t m_generatedPattern[MAX_BLINK_COUNT * 2];
    uint8_t  m_repeatCount;
    uint8_t  m_repeatTotal;

    // add to public:
    void setBlinkPattern(uint8_t count, uint32_t duration, uint32_t color = 0xFFFFFFFF);

    private:
    uint8_t generateBlinkPattern(uint8_t count, uint16_t onMs, uint16_t offMs, uint16_t gapMs);

    // Pattern queue struct
    // struct PatternItem {
    //     const uint16_t* sequence;
    //     uint8_t length;
    //     uint32_t duration;
    //     uint32_t color;
    // };

    // in the header, update PatternItem:
    struct PatternItem {
        uint16_t sequence[MAX_BLINK_COUNT * 2];  // own copy, not a pointer
        uint8_t  length;
        uint32_t duration;
        uint32_t color;
        bool     runOnce;
        // in PatternItem struct, add:
        bool isBreathing;
    };

    static const uint8_t MAX_QUEUE = 5;
    PatternItem m_queue[MAX_QUEUE];
    uint8_t m_queueStart = 0;
    uint8_t m_queueEnd = 0;
    bool isQueueEmpty() const { return m_queueStart == m_queueEnd; }

    // LED state
    uint8_t m_led_pin;
    bool m_ledState;
    unsigned long m_lastChange;
    const uint16_t* m_pattern;
    uint8_t m_patternLength;
    uint8_t m_step;
    bool m_running;
    uint32_t m_patternEndTime;

    // NeoPixel / PWM
    bool m_useNeoPixel;
    uint8_t m_neopixelPin;
    Adafruit_NeoPixel* m_strip;
    uint32_t m_customColor;
    uint8_t m_brightness;
    uint8_t m_pwmChannel;

    // Breathing
    uint8_t m_breatheLevel;
    int8_t m_breatheDirection;
};

//wokring old version
// #pragma once
// #include <Arduino.h>
// #include <Adafruit_NeoPixel.h>

// namespace LEDColor {
//     constexpr uint32_t WHITE  = 0xFFFFFF; // R=255, G=255, B=255
//     constexpr uint32_t RED    = 0xFF0000;
//     constexpr uint32_t GREEN  = 0x00B400; // 180 green
//     constexpr uint32_t BLUE   = 0x0000FF;
//     constexpr uint32_t PURPLE = 0xA000A0;

//     constexpr uint32_t YELLOW = 0xFFC800; // 255, 200, 0
//     // constexpr uint32_t ORANGE = 0xFF6400; // 255, 100, 0
//     constexpr uint32_t ORANGE = 0xFF6400; // 255, 10, 0

//     constexpr uint32_t AMBER  = 0xFF8C00; // 255, 140, 0
//     constexpr uint32_t GOLD   = 0xFFAA14; // 255, 170, 20

//     constexpr uint32_t CYAN   = 0x00B4B4; // 0, 180, 180
//     constexpr uint32_t PINK   = 0xFF64B4; // 255, 100, 180
// }

// enum LEDPattern {
//     PATTERN_NONE,
//     PATTERN_SOLID,
//     PATTERN_SLOW_BLINK,
//     PATTERN_FAST_BLINK,
//     PATTERN_DOUBLE_BLINK,
//     PATTERN_TRIPLE_BLINK,
//     PATTERN_TRIPLE_BLINK_SOLID,
//     PATTERN_FIVE_BLINK,
//     PATTERN_SOS,
//     PATTERN_BREATHING
// };

// class LEDController {
// public:
//     LEDController(uint8_t led_pin, bool useNeoPixel = false, uint8_t neopixelPin = 0);

//     void begin();
//     void update();
//     void setLED(bool on);

//     void setPattern(LEDPattern pattern, uint32_t duration = 3000, uint32_t color = LEDColor::WHITE);
//     bool isRunning() const { return m_running; }

//     // Pattern queueing
//     void enqueuePattern(LEDPattern pattern, uint32_t duration = 3000, uint32_t color = LEDColor::WHITE);
//     void enqueuePattern(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color = LEDColor::WHITE);

// private:
//     // Internal helpers
//     void setupPWM();
//     void setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color = LEDColor::WHITE);
//     void breathe();

//     // Pattern queue struct
//     struct PatternItem {
//         const uint16_t* sequence;
//         uint8_t length;
//         uint32_t duration;
//         uint32_t color;
//     };

//     static const uint8_t MAX_QUEUE = 5;
//     PatternItem m_queue[MAX_QUEUE];
//     uint8_t m_queueStart = 0;
//     uint8_t m_queueEnd = 0;
//     bool isQueueEmpty() const { return m_queueStart == m_queueEnd; }

//     // LED state
//     uint8_t m_led_pin;
//     // bool inverted; //ESP32C3 LED Logic inverted from ESP32DEV
//     bool m_ledState;
//     unsigned long m_lastChange;
//     const uint16_t* m_pattern;
//     uint8_t m_patternLength;
//     uint8_t m_step;
//     bool m_running;
//     uint32_t m_patternEndTime;

//     // NeoPixel / PWM
//     bool m_useNeoPixel;
//     uint8_t m_neopixelPin;
//     Adafruit_NeoPixel* m_strip;
//     uint32_t m_customColor;
//     uint8_t m_brightness;
//     uint8_t m_pwmChannel;

//     // Breathing
//     uint8_t m_breatheLevel;
//     int8_t m_breatheDirection;
// };