#include "audio_controller.h"

#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "Audio";

AudioContoller::AudioContoller(uint8_t rx_pin, uint8_t tx_pin)
    : m_rx_pin(rx_pin), m_tx_pin(tx_pin), player(nullptr) {}

AudioContoller::~AudioContoller() {
    if (player != nullptr) {
        delete player;
        player = nullptr;
    }
}

bool AudioContoller::begin() {
    delay(100);  // Small delay to ensure module is ready
    ESP_LOGE(TAG, "%lu - Initializing audio controller...", millis());

    if (player == nullptr) {
        // Use UART1 for ESP32-C3
        player = new JQ6500_Serial(1);  // UART1
        ESP_LOGE(TAG, "%lu - Created JQ6500_Serial on UART1", millis());
    }

    // Map UART1 to your pins (20 = RX, 21 = TX)
    player->begin(9600, SERIAL_8N1, m_rx_pin, m_tx_pin);

    // Ensure using built-in flash memory (not SD card)
    setSource(MP3_SRC_BUILTIN);
    delay(2000);  // Allow module to settle

    // Set initial volume
    player->setVolume(m_current_volume);

    // Mark audio as enabled
    audio_enabled = true;

    // Print module status
    uint8_t status = player->getStatus();
    ESP_LOGE(TAG, "%lu - JQ6500 status: %d", millis(), status);

    m_initialized = true;
    ESP_LOGE(TAG, "%lu - Initialized successfully. Volume: %d, Source: %s",
             millis(),
             m_current_volume,
             m_current_source == MP3_SRC_BUILTIN ? "Built-in" : "SD Card");

    return true;
}

void AudioContoller::setVolume(uint8_t volume) {
    if (!m_initialized) {
        return;
    }
    ESP_LOGE(TAG, "%lu - Volume changing from %d to ", millis(), m_current_volume);

    // Clamp volume between 0-30
    if (volume > 30) {
        m_current_volume = 30;
    } else {
        m_current_volume = volume;
    }

    if (audio_enabled) {
        player->setVolume(m_current_volume);
    } else {
        ESP_LOGE(TAG, " (audio disabled)");
    }
}

void AudioContoller::playTrack(uint8_t track) {
    if (!m_initialized) {
        return;
    }
    ESP_LOGE(TAG, "%lu - Playing track %d", millis(), track);

    if (audio_enabled) {
        // Ensure track is within valid range (0-99)
        if (track > 99) {
            ESP_LOGE(TAG, "Track number %d is out of range (0-99)", track);
            return;
        }
        // Play the track using JQ6500
        player->playFileByIndexNumber(track);
        ESP_LOGE(TAG, "Track %d started", track);
        m_last_played_track = track;
    }
}

void AudioContoller::reset() {
    if (player != nullptr) {
        ESP_LOGE(TAG, "%lu - Resetting audio player", millis());
        player->reset();
    }
}

uint8_t AudioContoller::getStatus() const {
    if (!m_initialized || !audio_enabled) {
        return MP3_STATUS_STOPPED;
    }

    uint8_t status = player->getStatus();
    return status;
}

uint8_t AudioContoller::getVolume() const {
    if (!m_initialized || !audio_enabled) {
        return 0;
    }

    // Return cached value as getVolume() from JQ6500 can be unreliable
    return m_current_volume;
}

uint16_t AudioContoller::getCurrentPosition() const {
    if (!m_initialized || !audio_enabled) {
        return 0;
    }

    return player->currentFilePositionInSeconds();
}

void AudioContoller::setSource(uint8_t source) {
    if (!m_initialized || !audio_enabled) {
        return;
    }

    // Only allow built-in flash or SD card sources
    if (source == MP3_SRC_BUILTIN || source == MP3_SRC_SDCARD) {
        m_current_source = source;
        player->setSource(source);
    }
}

uint8_t AudioContoller::getSource() const {
    // Return our cached source value since JQ6500 doesn't provide a getSource method
    return m_current_source;
}

uint8_t AudioContoller::getLastPlayedTrack() const {
    return m_last_played_track;
}

#ifdef UNIT_TEST
void AudioContoller::setPosition(uint16_t seconds) {
    if (!m_initialized || !audio_enabled) {
        return;
    }
    player->setPosition(seconds);
}
#endif
