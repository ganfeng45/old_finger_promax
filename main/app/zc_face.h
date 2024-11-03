#pragma once
#include <Arduino.h>

// #define FINGERPRINT_DEBUG

#define FACE_STARTCODE 0xEFAA

#define FACE_OK 0x00 //!< Command execution is complete

#define FACE_BADPACKET 0xFE //!< Bad packet was sent
#define FACE_TIMEOUT 0xFF   //!< Timeout was reached

struct Face_Packet
{
    uint16_t start_code; ///< "Wakeup" code for packet detection

    uint8_t cmd_id;    ///< Length of packet
    uint16_t length;   ///< Length of packet
    uint8_t data[512]; ///< The raw buffer for packet payload

    // 构造函数
    Face_Packet(uint8_t cmd_id, uint16_t length, uint8_t *data)
    {
        this->start_code = FACE_STARTCODE;
        this->length = length;
        this->cmd_id = cmd_id;
        memcpy(this->data, data, length);
    }
};

class Zc_Face
{
public:
    Zc_Face(HardwareSerial *hs);
    void begin(uint32_t baud);
    void writeStructuredPacket(const Face_Packet &packet);
    uint8_t getStructuredPacket(Face_Packet *packet, uint16_t timeout);
    uint16_t getID(uint16_t *facesta);
    uint8_t clean_enroll();
    uint8_t empty();
    uint8_t deleteone(uint16_t index);
    uint8_t getFaceCount(uint8_t *table);
    uint8_t getFaceSta();

private:
    Stream *mySerial;
    HardwareSerial *hwSerial;
};
