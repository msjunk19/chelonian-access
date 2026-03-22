#pragma once
#include <array>
#include "pin_mapping.hpp"

#include <Adafruit_PN532.h>
#include <Arduino.h>

class RFIDController {
public:
    RFIDController(uint8_t clk = PN532_SCK, uint8_t miso = PN532_MISO, uint8_t mosi = PN532_MOSI,
                   uint8_t ss = PN532_SS);
    ~RFIDController();
    bool begin();
    bool readCard(uint8_t* uid, uint8_t* uidLength);
    bool validateUID(const uint8_t* uid, uint8_t uidLength);
    // void addUID4B(const uint8_t* uid);
    // void addUID7B(const uint8_t* uid);
    uint32_t getFirmwareVersion();
    void printFirmwareVersion();
    void initializeDefaultUIDs();

private:

    Adafruit_PN532* m_nfc;  // Using SPI interface
    
    // static constexpr uint8_t MAX_4B_UIDS = 1;
    // static constexpr uint8_t MAX_7B_UIDS = 2;
    // std::array<std::array<uint8_t, 4>, MAX_4B_UIDS> m_uids4B{};
    // std::array<std::array<uint8_t, 7>, MAX_7B_UIDS> m_uids7B{};
    // uint8_t m_num4BUIDs{0};
    // uint8_t m_num7BUIDs{0};

    // static bool compare4BUID(const uint8_t* uid1, const uint8_t* uid2);
    // static bool compare7BUID(const uint8_t* uid1, const uint8_t* uid2);
};
