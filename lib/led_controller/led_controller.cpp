#include "led_controller.h"

// Example blink sequences
static const uint16_t PATTERN_SOLID_SEQ[]              = {0, 3000};
static const uint16_t PATTERN_SLOW_BLINK_SEQ[]         = {500, 500};
static const uint16_t PATTERN_FAST_BLINK_SEQ[]         = {200, 200};
static const uint16_t PATTERN_DOUBLE_BLINK_SEQ[]       = {120,120,120,700};
static const uint16_t PATTERN_TRIPLE_BLINK_SEQ[]       = {120,120,120,120,120,700};
static const uint16_t PATTERN_TRIPLE_BLINK_SOLID_SEQ[] = {120,120,120,120,120,120,120,1000};
static const uint16_t PATTERN_FIVE_BLINK_SEQ[]         = {120,120,120,120,120,120,120,120,120,700};
static const uint16_t PATTERN_SOS_SEQ[]                = {220,220,220,220,220,740,660,220,660,220,660,740,220,220,220,220,220,740};

struct PatternDef { const uint16_t* sequence; uint8_t length; };
static const PatternDef PATTERN_TABLE[] = {
    {nullptr, 0}, {PATTERN_SOLID_SEQ, 2}, {PATTERN_SLOW_BLINK_SEQ,2},
    {PATTERN_FAST_BLINK_SEQ,2},{PATTERN_DOUBLE_BLINK_SEQ,4},{PATTERN_TRIPLE_BLINK_SEQ,6},{PATTERN_TRIPLE_BLINK_SOLID_SEQ,8},{PATTERN_FIVE_BLINK_SEQ, 10},
    {PATTERN_SOS_SEQ,18},{nullptr,0}  // placeholder for breathing
};

// --- Constructor ---
LEDController::LEDController(uint8_t led_pin, bool useNeoPixel, uint8_t neopixelPin)
: m_led_pin(led_pin), m_ledState(false), m_lastChange(0), m_pattern(nullptr),
  m_patternLength(0), m_step(0), m_running(false), m_patternEndTime(0),
  m_useNeoPixel(useNeoPixel), m_neopixelPin(neopixelPin), m_strip(nullptr),
  m_brightness(255), m_pwmChannel(0), m_breatheLevel(0), m_breatheDirection(1)
{
    if (m_useNeoPixel && m_neopixelPin == 0)
        m_useNeoPixel = false;
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
    ledcSetup(m_pwmChannel, 5000, 8); // 5kHz, 8-bit resolution
    ledcAttachPin(m_led_pin, m_pwmChannel);
    ledcWrite(m_pwmChannel, 0); // start OFF
}

// --- Set LED ---
void LEDController::setLED(bool on) {
    if (m_ledState == on) return;

    if (m_useNeoPixel && m_strip) {
        uint32_t color = on ? (m_customColor == 0xFFFFFFFF
                                ? m_strip->Color(m_brightness,m_brightness,m_brightness)
                                : m_customColor)
                            : m_strip->Color(0,0,0);
        m_strip->setPixelColor(0, color);
        m_strip->show();
    } else {
        // ledcWrite(m_pwmChannel, on ? 255 : 0);
        ledcWrite(m_pwmChannel, on ? 0 : 255); // LED Logic Inverted on ESP32C3

    }

    m_ledState = on;
}

// --- Internal pattern ---
void LEDController::setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color){
    m_pattern = sequence;
    m_patternLength = length;
    m_step = 0;
    m_lastChange = millis();
    m_patternEndTime = (sequence == PATTERN_SOLID_SEQ) ? millis() + duration : 0;
    m_customColor = color;
    setLED(true);
    m_running = true;
}

// void LEDController::setPatternInternal(const uint16_t* sequence, uint8_t length, uint32_t duration){
//     m_pattern = sequence;
//     m_patternLength = length;
//     m_step = 0;
//     m_lastChange = millis();
//     m_patternEndTime = (sequence == PATTERN_SOLID_SEQ) ? millis() + duration : 0;
//     setLED(true);
//     m_running = true;
// }

// --- Set pattern ---

// --- Set pattern ---
void LEDController::setPattern(LEDPattern pattern, uint32_t duration, uint32_t color){
    if (pattern >= sizeof(PATTERN_TABLE)/sizeof(PATTERN_TABLE[0])) return;

    if (pattern == PATTERN_BREATHING) {
        m_breatheLevel = 0;
        m_breatheDirection = 1;
        m_pattern = nullptr;
        m_running = true;
        m_customColor = color;

        if (duration > 0) m_patternEndTime = millis() + duration;
        else m_patternEndTime = 0;
        return;
    }

    const PatternDef& def = PATTERN_TABLE[pattern];
    if (!def.sequence) {
        m_pattern = nullptr;
        setLED(false);
        m_running = false;
        return;
    }

    setPatternInternal(def.sequence, def.length, duration, color);
}

// bool LEDController::isRunning() const { return m_running; }

// --- Breathing ---
void LEDController::breathe() {
    static unsigned long lastStep = 0;
    unsigned long now = millis();
    if (now - lastStep < 15) return;  // 15 ms per step
    lastStep = now;

    if (m_useNeoPixel && m_strip) {
        m_strip->setBrightness(m_breatheLevel);
        m_strip->setPixelColor(0, m_strip->Color(m_breatheLevel,m_breatheLevel,m_breatheLevel));
        m_strip->show();
    } else {
        ledcWrite(m_pwmChannel, m_breatheLevel);
    }

    // m_breatheLevel += m_breatheDirection;
    m_breatheLevel += m_breatheDirection * 2;  // bigger step = faster fade
    if (m_breatheLevel <= 0 || m_breatheLevel >= 255)
        m_breatheDirection = -m_breatheDirection;
}

// --- Pattern queue ---
void LEDController::enqueuePattern(const uint16_t* sequence, uint8_t length, uint32_t duration, uint32_t color) {
    // Check if queue is full
    uint8_t nextEnd = (m_queueEnd + 1) % MAX_QUEUE;
    if (nextEnd == m_queueStart) return;  // queue full

    // Add pattern at m_queueEnd
    m_queue[m_queueEnd].sequence = sequence;
    m_queue[m_queueEnd].length = length;
    m_queue[m_queueEnd].duration = duration;
    m_queue[m_queueEnd].color = color;

    // Move end forward
    m_queueEnd = nextEnd;
}

void LEDController::enqueuePattern(LEDPattern pattern, uint32_t duration, uint32_t color) {
    if (pattern >= sizeof(PATTERN_TABLE)/sizeof(PATTERN_TABLE[0])) return;
    const PatternDef& def = PATTERN_TABLE[pattern];
    if (!def.sequence) return;
    enqueuePattern(def.sequence, def.length, duration, color); // ✅ calls the raw-sequence version
}

// --- Update ---
void LEDController::update() {
    if (!m_running && !isQueueEmpty()) {
        PatternItem& next = m_queue[m_queueStart];
        m_queueStart = (m_queueStart + 1) % MAX_QUEUE;
        setPatternInternal(next.sequence, next.length, next.duration, next.color);
    }

    if (!m_running) return;

    unsigned long now = millis();

    // Breathing
    if (m_pattern == nullptr) {
        if (m_patternEndTime != 0 && now >= m_patternEndTime) {
            if (m_useNeoPixel && m_strip) {
                m_strip->setPixelColor(0, m_strip->Color(0,0,0));
                m_strip->show();
            } else {
                // ledcWrite(m_pwmChannel, 0);
                ledcWrite(m_pwmChannel, 255); //Flip LED Logic for ESP32C3 Internal LED
            }
            m_running = false;
            return;
        }
        breathe();
        return;
    }

    // Blink/solid patterns
    if (m_patternEndTime != 0 && now >= m_patternEndTime) {
        setLED(false);
        m_running = false;
        return;
    }

    uint16_t duration = m_pattern[m_step];
    if (duration == 0) {
        setLED(true);
        return;
    }

    if (now - m_lastChange >= duration) {
        m_lastChange = now;
        setLED(!m_ledState);

        m_step++;
        if (m_step >= m_patternLength) {
            m_running = false;
            setLED(false);
        }
    }
}