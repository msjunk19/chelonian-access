#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <array>
#include <pin_mapping.hpp>

class RFIDController {
public:
    RFIDController(uint8_t clk = PN532_SCK, uint8_t miso = PN532_MISO, uint8_t mosi = PN532_MOSI,
                   uint8_t ss = PN532_SS);
    ~RFIDController();
    bool begin();
    bool readCard(uint8_t* uid, uint8_t* uidLength);
    bool validateUID(const uint8_t* uid, uint8_t uidLength);
    uint32_t getFirmwareVersion();
    void printFirmwareVersion();
    void initializeDefaultUIDs();

private:
    Adafruit_PN532* m_nfc;  // Using SPI interface
};
