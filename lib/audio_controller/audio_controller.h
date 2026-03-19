#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>  // For SoftwareSerial on ESP32-C3
#ifdef USINGMP3
#include <JQ6500_Serial.h>
#endif

// JQ6500 Status constants
#define MP3_STATUS_STOPPED 0
#define MP3_STATUS_PLAYING 1
#define MP3_STATUS_PAUSED 2

// JQ6500 Source constants
#define MP3_SRC_BUILTIN 4
#define MP3_SRC_SDCARD 1

class AudioContoller {
public:
    AudioContoller(uint8_t rx_pin = 20, uint8_t tx_pin = 21);  // ESP32-C3 Serial1 pins
    ~AudioContoller();                                         // Add destructor

    bool begin();
    void setVolume(uint8_t volume);
    void playTrack(uint8_t track);
    void reset();  // JQ6500 reset method

    // Status monitoring methods
    uint8_t getStatus() const;
    uint8_t getVolume() const;
    uint16_t getCurrentPosition() const;  // Current position in seconds

    // Source control methods
    void setSource(uint8_t source);
    uint8_t getSource() const;  // Note: JQ6500 doesn't actually have a getSource command
#ifdef UNIT_TEST
    void setPosition(uint16_t seconds);  // For testing purposes
#endif
    uint8_t getLastPlayedTrack() const;

    // Sound effect constants
    static constexpr uint8_t SOUND_STARTUP = 1;
    static constexpr uint8_t SOUND_WAITING = 2;
    static constexpr uint8_t SOUND_ACCEPTED = 3;
    static constexpr uint8_t SOUND_DENIED_1 = 4;
    static constexpr uint8_t SOUND_DENIED_2 = 5;
    static constexpr uint8_t SOUND_DENIED_3 = 6;

private:
    bool m_initialized{false};
    uint8_t m_current_volume{50}; // testing with xbox speaker, no DAC
    // uint8_t m_current_volume{20}; // original volume
    uint8_t m_rx_pin;
    uint8_t m_tx_pin;
    uint8_t m_current_source{
        MP3_SRC_BUILTIN};  // Track source internally since JQ6500 doesn't provide getSource
    uint8_t m_last_played_track{0};  // Store the last played track

#ifdef USINGMP3
    // State for MP3 player
    JQ6500_Serial* player{nullptr};
#endif
    bool audio_enabled{false};
};
