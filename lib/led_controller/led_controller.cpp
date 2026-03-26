#include "led_controller.h"

// Solid and breathing are the only hardcoded sequences
static const uint16_t PATTERN_SOLID_SEQ[] = {0, 3000};

struct PatternDef { const uint16_t* sequence; uint8_t length; };
static const PatternDef PATTERN_TABLE[] = {
    {nullptr, 0},           // PATTERN_NONE
    {PATTERN_SOLID_SEQ, 2}, // PATTERN_SOLID
    {nullptr, 0},           // PATTERN_SLOW_BLINK   (generated)
    {nullptr, 0},           // PATTERN_FAST_BLINK   (generated)
    {nullptr, 0},           // PATTERN_DOUBLE_BLINK (generated)
    {nullptr, 0},           // PATTERN_TRIPLE_BLINK (generated)
    {nullptr, 0},           // PATTERN_QUAD_BLINK (generated)
    {nullptr, 0},           // PATTERN_FIVE_BLINK (generated)
    {nullptr, 0},           // PATTERN_SOS          (not used this way)
    {nullptr, 0}            // PATTERN_BREATHING
};

// Blink timing constants
static const uint16_t BLINK_ON_MS   = 120;
static const uint16_t BLINK_OFF_MS  = 120;
static const uint16_t BLINK_GAP_MS  = 700;
static const uint16_t SLOW_ON_MS    = 500;
static const uint16_t SLOW_OFF_MS   = 500;
static const uint16_t FAST_ON_MS    = 200;
static const uint16_t FAST_OFF_MS   = 200;

// SOS stays hardcoded since it's a fixed message
static const uint16_t PATTERN_SOS_SEQ[] = {
    220,220,220,220,220,740,
    660,220,660,220,660,740,
    220,220,220,220,220,740
};

// --- Constructor ---
LEDController::LEDController(uint8_t led_pin, bool useNeoPixel, uint8_t neopixelPin)
: m_led_pin(led_pin), m_ledState(false), m_lastChange(0), m_pattern(nullptr),
  m_patternLength(0), m_step(0), m_running(false), m_patternEndTime(0),
  m_useNeoPixel(useNeoPixel), m_neopixelPin(neopixelPin), m_strip(nullptr),
  m_brightness(255), m_pwmChannel(0), m_breatheLevel(0), m_breatheDirection(1),
  m_repeatCount(0), m_repeatTotal(0)
{
    if (m_useNeoPixel && m_neopixelPin == 0)
        m_useNeoPixel = false;
    memset(m_generatedPattern, 0, sizeof(m_generatedPattern));
}

// --- Begin ---
void LEDController::begin() {
    if (m_useNeoPixel) {
        m_strip = new Adafruit_NeoPixel(1, m_neopixelPin, NEO_GRB + NEO_KHZ800);
        m_strip->begin();
        m_strip->setBrightness(m_brightness);
        m_strip->clear();
        m_strip->show();
    } else {
        setupPWM();
    }
}

// --- Setup PWM ---
void LEDController::setupPWM() {
    ledcSetup(m_pwmChannel, 5000, 8);
    ledcAttachPin(m_led_pin, m_pwmChannel);
    ledcWrite(m_pwmChannel, 0);
}

// --- Set LED ---
void LEDController::setLED(bool on) {
    if (m_ledState == on) return;

    if (m_useNeoPixel && m_strip) {
        uint32_t color = on ? (m_customColor == 0xFFFFFFFF
                                ? m_strip->Color(m_brightness, m_brightness, m_brightness)
                                : m_customColor)
                            : m_strip->Color(0, 0, 0);
        m_strip->setPixelColor(0, color);
        m_strip->show();
    } else {
        // ledcWrite(m_pwmChannel, on ? 255 : 0); //LED Logic for ESP32Devkit
        ledcWrite(m_pwmChannel, on ? 0 : 255); // LED Logic Inverted on ESP32C3
    }

    m_ledState = on;
}

// --- Generate blink pattern into m_generatedPattern ---
// Format: ON, OFF, ON, OFF, ..., LONG_GAP
// Returns the number of steps written
uint8_t LEDController::generateBlinkPattern(uint8_t count, uint16_t onMs, uint16_t offMs, uint16_t gapMs) {
    uint8_t steps = 0;
    for (uint8_t i = 0; i < count; i++) {
        m_generatedPattern[steps++] = onMs;
        // Last blink uses the gap instead of a short off
        m_generatedPattern[steps++] = (i == count - 1) ? gapMs : offMs;
    }
    return steps;
}

// --- Internal pattern start ---
// void LEDController::setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color) {
//     m_pattern = sequence;
//     m_patternLength = length;
//     m_step = 0;
//     unsigned long now = millis();
//     m_lastChange = now;
//     m_patternEndTime = (duration > 0) ? now + duration : 0;
//     m_customColor = color;
//     m_repeatCount = 0;
//     setLED(true);
//     m_running = true;
// }

void LEDController::setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color) {
    m_pattern = sequence;
    m_patternLength = length;
    m_step = 0;
    unsigned long now = millis();
    m_lastChange = now;
    m_patternEndTime = (duration > 0) ? now + duration : 0;
    m_customColor = color;
    m_repeatCount = 0;
    m_runOnce = (duration == 0);  // ← new flag
    setLED(true);
    m_running = true;
}

// --- Set pattern (public) ---
void LEDController::setPattern(LEDPattern pattern, uint32_t duration, uint32_t color) {
    if (pattern >= sizeof(PATTERN_TABLE) / sizeof(PATTERN_TABLE[0])) return;

    // Breathing
    if (pattern == PATTERN_BREATHING) {
        m_breatheLevel = 0;
        m_breatheDirection = 1;
        m_pattern = nullptr;
        m_running = true;
        m_customColor = color;
        m_patternEndTime = (duration > 0) ? millis() + duration : 0;
        return;
    }

    // Solid
    if (pattern == PATTERN_SOLID) {
        setPatternInternal(PATTERN_SOLID_SEQ, 2, duration, color);
        return;
    }

    // SOS
    if (pattern == PATTERN_SOS) {
        setPatternInternal(PATTERN_SOS_SEQ, 18, duration, color);
        return;
    }

    // Generated blink patterns — map enum to count + timing
    uint8_t count = 0;
    uint16_t onMs = BLINK_ON_MS, offMs = BLINK_OFF_MS, gapMs = BLINK_GAP_MS;

    switch (pattern) {
        case PATTERN_SLOW_BLINK:   count = 1; onMs = SLOW_ON_MS; offMs = SLOW_OFF_MS; gapMs = SLOW_OFF_MS; break;
        case PATTERN_FAST_BLINK:   count = 1; onMs = FAST_ON_MS; offMs = FAST_OFF_MS; gapMs = FAST_OFF_MS; break;
        case PATTERN_DOUBLE_BLINK: count = 2; break;
        case PATTERN_TRIPLE_BLINK: count = 3; break;
        case PATTERN_QUAD_BLINK: count = 4; break;
        case PATTERN_FIVE_BLINK: count = 5; break;
        default: setLED(false); m_running = false; return;
    }

    uint8_t len = generateBlinkPattern(count, onMs, offMs, gapMs);
    setPatternInternal(m_generatedPattern, len, duration, color);
}

// --- Set pattern by blink count (generates pattern dynamically) ---
void LEDController::setBlinkPattern(uint8_t count, uint32_t duration, uint32_t color) {
    if (count == 0 || count > MAX_BLINK_COUNT) return;
    uint8_t len = generateBlinkPattern(count, BLINK_ON_MS, BLINK_OFF_MS, BLINK_GAP_MS);
    setPatternInternal(m_generatedPattern, len, duration, color);
}

// --- Breathing ---
void LEDController::breathe() {
    static unsigned long lastStep = 0;
    unsigned long now = millis();
    if (now - lastStep < 15) return;
    lastStep = now;

    if (m_useNeoPixel && m_strip) {
        m_strip->setBrightness(m_breatheLevel);
        // m_strip->setPixelColor(0, m_strip->Color(m_breatheLevel, m_breatheLevel, m_breatheLevel));
        uint8_t r = ((m_customColor >> 16) & 0xFF) * m_breatheLevel / 255;
        uint8_t g = ((m_customColor >> 8)  & 0xFF) * m_breatheLevel / 255;
        uint8_t b = ( m_customColor        & 0xFF) * m_breatheLevel / 255;
        m_strip->setPixelColor(0, m_strip->Color(r, g, b));
        m_strip->show();
    } else {
        ledcWrite(m_pwmChannel, m_breatheLevel);
    }

    m_breatheLevel += m_breatheDirection * 2;
    if (m_breatheLevel <= 0 || m_breatheLevel >= 255)
        m_breatheDirection = -m_breatheDirection;
}

// --- Pattern queue ---
// void LEDController::enqueuePattern(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color) {
//     uint8_t nextEnd = (m_queueEnd + 1) % MAX_QUEUE;
//     if (nextEnd == m_queueStart) return;

//     m_queue[m_queueEnd].sequence = sequence;
//     m_queue[m_queueEnd].length = length;
//     m_queue[m_queueEnd].duration = duration;
//     m_queue[m_queueEnd].color = color;
//     m_queueEnd = nextEnd;
// }

void LEDController::enqueuePattern(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color) {
    uint8_t nextEnd = (m_queueEnd + 1) % MAX_QUEUE;
    if (nextEnd == m_queueStart) return;
    m_queue[m_queueEnd].isBreathing = false;  // ← set it here, once

    memcpy(m_queue[m_queueEnd].sequence, sequence, length * sizeof(uint16_t));  // ← copy
    m_queue[m_queueEnd].length = length;
    m_queue[m_queueEnd].duration = duration;
    m_queue[m_queueEnd].color = color;
    m_queue[m_queueEnd].runOnce = (duration == 0);
    m_queueEnd = nextEnd;
}


void LEDController::enqueuePattern(LEDPattern pattern, uint32_t duration, uint32_t color) {
    if (pattern == PATTERN_BREATHING) {
        uint8_t nextEnd = (m_queueEnd + 1) % MAX_QUEUE;
        if (nextEnd == m_queueStart) return;
        m_queue[m_queueEnd].isBreathing = true;  // ← was missing
        m_queue[m_queueEnd].length = 0;
        m_queue[m_queueEnd].duration = duration;
        m_queue[m_queueEnd].color = color;
        m_queue[m_queueEnd].runOnce = (duration == 0);
        m_queueEnd = nextEnd;
        return;
    }

    // For solid and SOS, use the hardcoded sequences
    if (pattern == PATTERN_SOLID) {
        enqueuePattern(PATTERN_SOLID_SEQ, 2, duration, color);
        return;
    }
    if (pattern == PATTERN_SOS) {
        enqueuePattern(PATTERN_SOS_SEQ, 18, duration, color);
        return;
    }

    // For generated blink patterns, build into temp buffer then copy via enqueuePattern
    uint8_t count = 0;
    uint16_t onMs = BLINK_ON_MS, offMs = BLINK_OFF_MS, gapMs = BLINK_GAP_MS;

    switch (pattern) {
        case PATTERN_SLOW_BLINK:   count = 1; onMs = SLOW_ON_MS; offMs = SLOW_OFF_MS; gapMs = SLOW_OFF_MS; break;
        case PATTERN_FAST_BLINK:   count = 1; onMs = FAST_ON_MS; offMs = FAST_OFF_MS; gapMs = FAST_OFF_MS; break;
        case PATTERN_DOUBLE_BLINK: count = 2; break;
        case PATTERN_TRIPLE_BLINK: count = 3; break;
        default: return;
    }

    uint8_t len = generateBlinkPattern(count, onMs, offMs, gapMs);
    enqueuePattern(m_generatedPattern, len, duration, color);  // copies into queue slot
}



void LEDController::update() {

    if (!m_running && !isQueueEmpty()) {
        PatternItem& next = m_queue[m_queueStart];
        m_queueStart = (m_queueStart + 1) % MAX_QUEUE;

    if (next.isBreathing) {
        Serial.println("DEBUG: dequeuing breathing");  // ← add this
        m_breatheLevel = 0;
        m_breatheDirection = 1;
        m_pattern = nullptr;
        Serial.print("DEBUG: m_pattern is null: ");    // ← and this
        Serial.println(m_pattern == nullptr);
        m_running = true;
        m_customColor = next.color;
        m_runOnce = next.runOnce;
        m_patternEndTime = (next.duration > 0) ? millis() + next.duration : 0;
    } else {
        Serial.println("DEBUG: dequeuing normal pattern");  // ← and this
        setPatternInternal(next.sequence, next.length, next.duration, next.color);
    }
    }

    if (!m_running) return;

    unsigned long now = millis();

    // Breathing
    if (m_pattern == nullptr) {
        if (m_patternEndTime != 0 && now >= m_patternEndTime) {
            if (m_useNeoPixel && m_strip) {
                m_strip->setPixelColor(0, m_strip->Color(0, 0, 0));
                m_strip->show();
            } else {
                ledcWrite(m_pwmChannel, 0);
            }
            m_running = false;
            return;
        }
        breathe();
        return;
    }

    uint16_t stepDuration = m_pattern[m_step];
    if (stepDuration == 0) {
        if (m_patternEndTime != 0 && now >= m_patternEndTime) {
            setLED(false);
            m_running = false;
        } else {
            setLED(true);
        }
        return;
    }

    if (now - m_lastChange >= stepDuration) {
        m_lastChange = now;
        setLED(!m_ledState);
        m_step++;

        if (m_step >= m_patternLength) {
            if (m_runOnce || (m_patternEndTime != 0 && now >= m_patternEndTime)) {
                setLED(false);
                m_running = false;
            } else {
                m_step = 0;
                setLED(true);
            }
        }
    }
}



// working old version
// #include "led_controller.h"

// // Example blink sequences
// static const uint16_t PATTERN_SOLID_SEQ[]              = {0, 3000};
// static const uint16_t PATTERN_SLOW_BLINK_SEQ[]         = {500, 500};
// static const uint16_t PATTERN_FAST_BLINK_SEQ[]         = {200, 200};
// static const uint16_t PATTERN_DOUBLE_BLINK_SEQ[]       = {120,120,120,700};
// static const uint16_t PATTERN_TRIPLE_BLINK_SEQ[]       = {120,120,120,120,120,700};
// static const uint16_t PATTERN_TRIPLE_BLINK_SOLID_SEQ[] = {120,120,120,120,120,120,120,1000};
// static const uint16_t PATTERN_FIVE_BLINK_SEQ[]         = {120,120,120,120,120,120,120,120,120,700};
// static const uint16_t PATTERN_SOS_SEQ[]                = {220,220,220,220,220,740,660,220,660,220,660,740,220,220,220,220,220,740};

// struct PatternDef { const uint16_t* sequence; uint8_t length; };
// static const PatternDef PATTERN_TABLE[] = {
//     {nullptr, 0}, {PATTERN_SOLID_SEQ, 2}, {PATTERN_SLOW_BLINK_SEQ,2},
//     {PATTERN_FAST_BLINK_SEQ,2},{PATTERN_DOUBLE_BLINK_SEQ,4},{PATTERN_TRIPLE_BLINK_SEQ,6},{PATTERN_TRIPLE_BLINK_SOLID_SEQ,8},{PATTERN_FIVE_BLINK_SEQ, 10},
//     {PATTERN_SOS_SEQ,18},{nullptr,0}  // placeholder for breathing
// };

// // --- Constructor ---
// LEDController::LEDController(uint8_t led_pin, bool useNeoPixel, uint8_t neopixelPin)
// : m_led_pin(led_pin), m_ledState(false), m_lastChange(0), m_pattern(nullptr),
//   m_patternLength(0), m_step(0), m_running(false), m_patternEndTime(0),
//   m_useNeoPixel(useNeoPixel), m_neopixelPin(neopixelPin), m_strip(nullptr),
//   m_brightness(255), m_pwmChannel(0), m_breatheLevel(0), m_breatheDirection(1)
// {
//     if (m_useNeoPixel && m_neopixelPin == 0)
//         m_useNeoPixel = false;
// }

// // --- Begin ---
// void LEDController::begin() {
//     if (m_useNeoPixel) {
//         m_strip = new Adafruit_NeoPixel(1, m_neopixelPin, NEO_GRB + NEO_KHZ800);
//         m_strip->begin();
//         m_strip->setBrightness(m_brightness);
//         m_strip->clear();
//         m_strip->show();
//     } else {
//         setupPWM();
//     }
// }

// // --- Setup PWM ---
// void LEDController::setupPWM() {
//     ledcSetup(m_pwmChannel, 5000, 8); // 5kHz, 8-bit resolution
//     ledcAttachPin(m_led_pin, m_pwmChannel);
//     ledcWrite(m_pwmChannel, 0); // start OFF
// }

// // --- Set LED ---
// void LEDController::setLED(bool on) {
//     if (m_ledState == on) return;

//     if (m_useNeoPixel && m_strip) {
//         uint32_t color = on ? (m_customColor == 0xFFFFFFFF
//                                 ? m_strip->Color(m_brightness,m_brightness,m_brightness)
//                                 : m_customColor)
//                             : m_strip->Color(0,0,0);
//         m_strip->setPixelColor(0, color);
//         m_strip->show();
//     } else {
//         // ledcWrite(m_pwmChannel, on ? 255 : 0); //LED Logic for ESP32Devkit
//         ledcWrite(m_pwmChannel, on ? 0 : 255); // LED Logic Inverted on ESP32C3

//     }

//     m_ledState = on;
// }

// // --- Internal pattern ---
// void LEDController::setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color){
//     m_pattern = sequence;
//     m_patternLength = length;
//     m_step = 0;
//     m_lastChange = millis();
//     m_patternEndTime = (sequence == PATTERN_SOLID_SEQ) ? millis() + duration : 0;
//     m_customColor = color;
//     setLED(true);
//     m_running = true;
// }

// // --- Set pattern ---
// void LEDController::setPattern(LEDPattern pattern, uint32_t duration, uint32_t color){
//     if (pattern >= sizeof(PATTERN_TABLE)/sizeof(PATTERN_TABLE[0])) return;

//     if (pattern == PATTERN_BREATHING) {
//         m_breatheLevel = 0;
//         m_breatheDirection = 1;
//         m_pattern = nullptr;
//         m_running = true;
//         m_customColor = color;

//         if (duration > 0) m_patternEndTime = millis() + duration;
//         else m_patternEndTime = 0;
//         return;
//     }

//     const PatternDef& def = PATTERN_TABLE[pattern];
//     if (!def.sequence) {
//         m_pattern = nullptr;
//         setLED(false);
//         m_running = false;
//         return;
//     }

//     setPatternInternal(def.sequence, def.length, duration, color);
// }

// // --- Breathing ---
// void LEDController::breathe() {
//     static unsigned long lastStep = 0;
//     unsigned long now = millis();
//     if (now - lastStep < 15) return;  // 15 ms per step
//     lastStep = now;

//     if (m_useNeoPixel && m_strip) {
//         m_strip->setBrightness(m_breatheLevel);
//         m_strip->setPixelColor(0, m_strip->Color(m_breatheLevel,m_breatheLevel,m_breatheLevel));
//         m_strip->show();
//     } else {
//         ledcWrite(m_pwmChannel, m_breatheLevel);
//     }

//     // m_breatheLevel += m_breatheDirection;
//     m_breatheLevel += m_breatheDirection * 2;  // bigger step = faster fade
//     if (m_breatheLevel <= 0 || m_breatheLevel >= 255)
//         m_breatheDirection = -m_breatheDirection;
// }

// // --- Pattern queue ---
// void LEDController::enqueuePattern(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color) {
//     // Check if queue is full
//     uint8_t nextEnd = (m_queueEnd + 1) % MAX_QUEUE;
//     if (nextEnd == m_queueStart) return;  // queue full

//     // Add pattern at m_queueEnd
//     m_queue[m_queueEnd].sequence = sequence;
//     m_queue[m_queueEnd].length = length;
//     m_queue[m_queueEnd].duration = duration;
//     m_queue[m_queueEnd].color = color;

//     // Move end forward
//     m_queueEnd = nextEnd;
// }

// void LEDController::enqueuePattern(LEDPattern pattern, uint32_t duration, uint32_t color) {
//     if (pattern >= sizeof(PATTERN_TABLE)/sizeof(PATTERN_TABLE[0])) return;
//     const PatternDef& def = PATTERN_TABLE[pattern];
//     if (!def.sequence) return;
//     enqueuePattern(def.sequence, def.length, duration, color); // ✅ calls the raw-sequence version
// }

// // --- Update ---
// void LEDController::update() {
//     if (!m_running && !isQueueEmpty()) {
//         PatternItem& next = m_queue[m_queueStart];
//         m_queueStart = (m_queueStart + 1) % MAX_QUEUE;
//         setPatternInternal(next.sequence, next.length, next.duration, next.color);
//     }

//     if (!m_running) return;

//     unsigned long now = millis();

//     // Breathing
//     if (m_pattern == nullptr) {
//         if (m_patternEndTime != 0 && now >= m_patternEndTime) {
//             if (m_useNeoPixel && m_strip) {
//                 m_strip->setPixelColor(0, m_strip->Color(0,0,0));
//                 m_strip->show();
//             } else {
//                 // ledcWrite(m_pwmChannel, 0); //Logic for ESP32Devkit
//                 ledcWrite(m_pwmChannel, 255); //Flip LED Logic for ESP32C3 Internal LED
//             }
//             m_running = false;
//             return;
//         }
//         breathe();
//         return;
//     }

//     // Blink/solid patterns
//     if (m_patternEndTime != 0 && now >= m_patternEndTime) {
//         setLED(false);
//         m_running = false;
//         return;
//     }

//     uint16_t duration = m_pattern[m_step];
//     if (duration == 0) {
//         setLED(true);
//         return;
//     }

//     if (now - m_lastChange >= duration) {
//         m_lastChange = now;
//         setLED(!m_ledState);

//         m_step++;
//         if (m_step >= m_patternLength) {
//             m_running = false;
//             setLED(false);
//         }
//     }
// }