// #ifdef DEBUG_FLAG
// #define debug(x) Serial.print(x)
// #define debugln(x) Serial.println(x)
// #else
// #define debug(x)
// #define debugln(x)
// #endif
#include <esp32_can.h> /* https://github.com/collin80/esp32_can */
#define MIN(x, y) ((x) < (y) ? (x) : (y))
// --- shield pins ---------------------

#define SHIELD_LED_PIN GPIO_NUM_NC
#define SHIELD_CAN_RX GPIO_NUM_6
#define SHIELD_CAN_TX GPIO_NUM_7
#define TAG "app_can"
#include "blackboard.h"
byte i = 0;
CAN_FRAME can_message;
CAN_FRAME_FD message;
void setup_can()
{

    ESP_LOGD(TAG, "CAN INIT");

    /* setup CAN @500kbps */

    CAN0.setCANPins(SHIELD_CAN_TX, SHIELD_CAN_RX);
    if (!CAN0.begin(125000))
    {
        ESP_LOGD(TAG, " CAN...............FAIL");
        // delay(500);
        // ESP.restart();
    }
    ESP_LOGD(TAG, " CAN.................OK");
    CAN0.watchFor();
    // CAN0.watchFor(0X3F1);
    // CAN0.watchFor(0x257);
    // CAN0.watchForRange(uint32_t id1, uint32_t id2)
}
int left_distance = 99, right_distance = 99;
bool ifsend = false;
int can_count = 0;
void send_userMsg()
{
    if (ifsend)
    {
        delay(2000);
        return;
    }

    // if (blackboard.face_auth_id)
    // blackboard.face_auth_id = 3;
    if (blackboard.face_auth_id)
    {
        uint8_t data[46] = {0}; // Example data
        fc_cfg_local.getBytes(String(blackboard.face_auth_id).c_str(), data, 34);
        int drivr_info_len = countNonZeroElements(data, sizeof(data));
        printf("-- %d\n", ++can_count);

        for (int i = 0; i < drivr_info_len; i++)
        {
            printf("%02X ", data[i]);
        }
        printf("\n");
        CAN_FRAME txFrame;
        txFrame.rtr = 0;
        txFrame.id = 0x257;
        txFrame.extended = false;
        txFrame.length = 8;
        txFrame.data.uint8[0] = 0x20;
        txFrame.data.uint8[1] = drivr_info_len;
        if (drivr_info_len % 7 == 0)
        {
            txFrame.data.uint8[2] = drivr_info_len / 7;
        }
        else
        {
            txFrame.data.uint8[2] = (drivr_info_len / 7) + 1;
        }
        for (int i = 3; i < 8; i++)
        {
            txFrame.data.uint8[i] = 0xff;
        }
        CAN0.sendFrame(txFrame);
        CAN0.sendFrame(txFrame);

        txFrame.id = 0x258;

        if (drivr_info_len > 0)
        {
            txFrame.id = 0x258;
            int packet_index = 1; 

            for (int i = 0; i < drivr_info_len; i += 7)
            {
                txFrame.data.uint8[0] = packet_index++; 
                int j;
                for (j = 0; j < 7 && (i + j) < drivr_info_len; j++)
                {
                    txFrame.data.uint8[j + 1] = data[i + j]; // 插入数据
                }
                for (; j < 7; j++)
                {
                    txFrame.data.uint8[j + 1] = 0xFF;
                }

                txFrame.length = 8; 
                CAN0.sendFrame(txFrame);
            }
        }
        ifsend = true;
    }
}
void loop_can()
{

    // if (CAN0.read(can_message))
    {

        {
            // Serial.println(can_message.data.uint8[2], DEC);
            // blackboard.car_speed = can_message.data.uint8[2] / 10.0;

            // CAN_FRAME txFrame;
            // txFrame.rtr = 0;
            // txFrame.id = 0x3F1;
            // txFrame.extended = false;
            // txFrame.length = 8;
            // txFrame.data.uint8[0] = 0x03;
            // txFrame.data.uint8[1] = 0x1A;
            // txFrame.data.uint8[2] = 0xFF;
            // txFrame.data.uint8[3] = 0x5D;
            // CAN0.sendFrame(txFrame);
            send_userMsg();
            delay(500);
        }
    }
}
void task_twai_entry(void *params)
{
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    ESP_LOGD(TAG, "task_twai_entry Driver installed");

    setup_can();
    // delay(5000);
    while (true)
    {
        delay(250);
        loop_can();
    }
    vTaskDelete(NULL);
}