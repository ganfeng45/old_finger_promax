#include "zc_face.h"
#define TAG "ZCFACE"
#define GET_CMD_PACKET(...)                             \
    uint8_t data[] = {__VA_ARGS__};                     \
    Face_Packet packet(cmd_id, sizeof(data),            \
                       data);                           \
    writeStructuredPacket(packet);                      \
    if (getStructuredPacket(&packet) != FINGERPRINT_OK) \
        return FINGERPRINT_PACKETRECIEVEERR;            \
    if (packet.type != FINGERPRINT_ACKPACKET)           \
        return FINGERPRINT_PACKETRECIEVEERR;

#define SEND_CMD_PACKET(...)     \
    GET_CMD_PACKET(__VA_ARGS__); \
    return packet.data[0];
Zc_Face::Zc_Face(HardwareSerial *hs)
{
    hwSerial = hs;
    mySerial = hwSerial;
}
uint8_t Zc_Face::getStructuredPacket(Face_Packet *packet,
                                     uint16_t timeout)
{
    uint8_t byte;
    uint16_t idx = 0, timer = 0;

#ifdef FINGERPRINT_DEBUG
    // Serial.print("<- ");
    ESP_LOGD(TAG, "<- ");
#endif

    while (true)
    {
        while (!mySerial->available())
        {
            delay(1);
            timer++;
            if (timer >= timeout)
            {
#ifdef FINGERPRINT_DEBUG
                // Serial.println("Timed out");
                ESP_LOGE(TAG, "Timed out");

#endif
                return FACE_TIMEOUT;
            }
        }
        byte = mySerial->read();
#ifdef FINGERPRINT_DEBUG
        // Serial.print("0x");
        // Serial.print(byte, HEX);
        // Serial.print(", ");
        ESP_LOGD(TAG, "%02x", byte);

#endif
        switch (idx)
        {
        case 0:
            if (byte != (FACE_STARTCODE >> 8))
                continue;
            packet->start_code = (uint16_t)byte << 8;
            break;
        case 1:
            packet->start_code |= byte;
            if (packet->start_code != FACE_STARTCODE)
                return FACE_BADPACKET;
            break;
        case 2:
            packet->cmd_id = byte;
            break;
        case 3:
            packet->length = (uint16_t)byte << 8;
            break;
        case 4:
            packet->length |= byte;
            break;
        default:
            packet->data[idx - 5] = byte;
            if ((idx - 5) == packet->length)
            {
#ifdef FINGERPRINT_DEBUG
                // Serial.println(" OK ");
                ESP_LOGD(TAG, "OK");

#endif
                return FACE_OK;
            }
            break;
        }
        idx++;
        if ((idx + 5) >= sizeof(packet->data))
        {
            return FACE_BADPACKET;
        }
    }
    // Shouldn't get here so...
    return FACE_BADPACKET;
}

void Zc_Face::writeStructuredPacket(const Face_Packet &packet)
{

    mySerial->write((uint8_t)(packet.start_code >> 8));
    mySerial->write((uint8_t)(packet.start_code & 0xFF));
    mySerial->write(packet.cmd_id);

    uint16_t wire_length = packet.length;
    mySerial->write((uint8_t)(wire_length >> 8));
    mySerial->write((uint8_t)(wire_length & 0xFF));

#ifdef FINGERPRINT_DEBUG
    // Serial.print("-> 0x");
    // Serial.print((uint8_t)(packet.start_code >> 8), HEX);
    // Serial.print(", 0x");
    // Serial.print((uint8_t)(packet.start_code & 0xFF), HEX);

    // Serial.print(", 0x");
    // Serial.print(packet.cmd_id, HEX);

    // Serial.print(", 0x");
    // Serial.print((uint8_t)(wire_length >> 8), HEX);
    // Serial.print(", 0x");
    // Serial.print((uint8_t)(wire_length & 0xFF), HEX);
#endif

    uint8_t sum = packet.cmd_id ^ (packet.length >> 8) ^ (packet.length & 0xFF);
    for (uint8_t i = 0; i < packet.length; i++)
    {
        mySerial->write(packet.data[i]);
        sum ^= packet.data[i];
#ifdef FINGERPRINT_DEBUG
        // Serial.print(", 0x");
        // Serial.print(packet.data[i], HEX);
#endif
    }

    mySerial->write((uint8_t)(sum));
    //   mySerial->write((uint8_t)(sum & 0xFF));

#ifdef FINGERPRINT_DEBUG
    // Serial.print(", 0x");
    // Serial.println((uint8_t)(sum), HEX);
    // Serial.print(", 0x");
    // Serial.println((uint8_t)(sum & 0xFF), HEX);
#endif

    return;
}
uint16_t Zc_Face::getID(uint16_t *facesta)
{
    // bool face_r = false;
    *facesta = 1;
    int faceNotecount = 0;
    uint8_t data[2] = {0x00, 0x02};
    Face_Packet packet(0x12, 0x02, data);
    writeStructuredPacket(packet);
    while (true)
    {
        getStructuredPacket(&packet, 1000);
        if (packet.cmd_id == 0x00)
        {
            break;
        }
        else if (packet.cmd_id == 0x01)
        {
            /* note message */
            if (packet.data[0] == 0x01)
            {

                /* 人脸状态消息 */
                faceNotecount++;
                int16_t face_sta = packet.data[2] << 8 | packet.data[1];
                // ESP_LOGI("TAG", "人脸状态消息 %s", String(face_sta).c_str());

                if (face_sta == 0x00)
                {
                    /* 检测到人脸 */
                    if (faceNotecount > 1)
                    {

                        *facesta = 0;
                    }
                }
                else if (face_sta == 1)
                {
                    /* 未检测到人脸 */
                }
            }
        }

        else
        {
            // Serial.println("error");
        }
    }
    if (packet.cmd_id == 0x00)
    {
        if (packet.data[0] == 0x12 && packet.data[1] == 0x00)
        {
            // Serial.println("检测ok");

            // return packet.data[3];
            // ESP_LOGI("TAG", "face.id:%u --%u",packet.data[2],packet.data[3] );

            return ((uint16_t)packet.data[2] << 8) | packet.data[3];
        }
        else
        {
            // Serial.println("检测失败");

            return 0;
        }
    }

    return 0;
}
uint8_t Zc_Face::clean_enroll()
{
    uint8_t data[2] = {0x00, 0x0A};
    Face_Packet packet(0x23, 0x00, data);
    writeStructuredPacket(packet);
    getStructuredPacket(&packet, 2000);
    if (packet.cmd_id == 0x00)
    {
        if (packet.data[0] == 0x23 && packet.data[1] == 0x00)
        {
            // Serial.println("清除ok");
            ESP_LOGD(TAG, "清除ok");
        }
        else
        {
            // Serial.println("清除失败");
            ESP_LOGD(TAG, "清除失败");
        }
    }
    return 1;
}
uint8_t Zc_Face::getFaceSta()
{
    uint8_t data[2] = {0x00, 0x0A};
    Face_Packet packet(0x11, 0x00, data);
    writeStructuredPacket(packet);
    getStructuredPacket(&packet, 400);
    if (packet.cmd_id == 0x00)
    {
        if (packet.data[0] == 0x11 && (packet.data[1] == 0x00 || packet.data[1] == 0x01))
        {
            // Serial.println("清空🆗");
            return FACE_OK;
        }
        else
        {
            // Serial.println("清除失败");
            return FACE_TIMEOUT;
        }
    }
    return FACE_TIMEOUT;
}

uint8_t Zc_Face::empty()
{
    uint8_t data[2] = {0x00, 0x0A};
    Face_Packet packet(0x21, 0x00, data);
    writeStructuredPacket(packet);
    getStructuredPacket(&packet, 1000);
    if (packet.cmd_id == 0x00)
    {
        if (packet.data[0] == 0x21 && packet.data[1] == 0x00)
        {
            // Serial.println("清空🆗");
            return FACE_OK;
        }
        else
        {
            // Serial.println("清除失败");
            return FACE_TIMEOUT;
        }
    }
    return FACE_TIMEOUT;
}
uint8_t Zc_Face::deleteone(uint16_t index)
{
    uint8_t data[2] = {0x00, 0x0A};
    data[0] = index >> 8;
    data[1] = index & 0xFF;
    Face_Packet packet(0x20, 0x02, data);
    writeStructuredPacket(packet);
    getStructuredPacket(&packet, 2000);
    if (packet.cmd_id == 0x00)
    {
        if (packet.data[0] == 0x20 && packet.data[1] == 0x00)
        {
            // Serial.println("清空🆗");
            return FACE_OK;
        }
        else
        {
            // Serial.println("清除失败");
            return packet.data[1];
        }
    }
    return FACE_TIMEOUT;
}
uint8_t Zc_Face::getFaceCount(uint8_t *table)
{
    uint8_t data[2] = {0x00, 0x0A};
    Face_Packet packet(0x24, 0x00, data);
    writeStructuredPacket(packet);
    getStructuredPacket(&packet, 2000);
    if (packet.cmd_id == 0x00)
    {
        if (packet.data[0] == 0x24 && packet.data[1] == FACE_OK)
        {
            if (table != NULL)
            {
                memcpy(table, &packet.data[3], 100);
            }
            return packet.data[2];
        }
        else
        {
            // Serial.println("清除失败");
            return packet.data[1];
        }
    }
    return FACE_TIMEOUT;
}
