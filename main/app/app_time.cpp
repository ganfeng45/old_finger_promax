#include "blackboard.h"
#include "RTClib.h"
#define TAG "RTCLB"
RTC_PCF8563 rtc;
static CMD_MP3_PLAYER test_mp3;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void splitUInt64(uint64_t input, uint32_t *high, uint32_t *low)
{
    *low = (uint32_t)(input & 0xFFFFFFFF);
    *high = (uint32_t)((input >> 32) & 0xFFFFFFFF);
}
uint64_t combineUInt32(uint32_t high, uint32_t low)
{
    return ((uint64_t)high << 32) | low;
}
void store_data_to_bytes(uint32_t data1, uint32_t data2, uint16_t data3, uint8_t bytes[9])
{
    // 将 uint32_t 和 uint16_t 数据按字节存储到 uint8_t 类型的数组中
    bytes[0] = (data1 >> 24) & 0xFF; // data1 的最高字节
    bytes[1] = (data1 >> 16) & 0xFF; // data1 的次高字节
    bytes[2] = (data1 >> 8) & 0xFF;  // data1 的次低字节
    bytes[3] = data1 & 0xFF;         // data1 的最低字节

    bytes[4] = (data2 >> 24) & 0xFF; // data2 的最高字节
    bytes[5] = (data2 >> 16) & 0xFF; // data2 的次高字节
    bytes[6] = (data2 >> 8) & 0xFF;  // data2 的次低字节
    bytes[7] = data2 & 0xFF;         // data2 的最低字节

    bytes[8] = (data3 >> 8) & 0xFF; // data3 的高字节
    bytes[9] = data3 & 0xFF;        // data3 的低字节
}

void extract_data_from_bytes(const uint8_t bytes[9], uint32_t *data1, uint32_t *data2, uint16_t *data3)
{
    // 从 uint8_t 类型的数组中提取数据
    *data1 = ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) | bytes[3];
    *data2 = ((uint32_t)bytes[4] << 24) | ((uint32_t)bytes[5] << 16) | ((uint32_t)bytes[6] << 8) | bytes[7];
    *data3 = ((uint16_t)bytes[8] << 8) | bytes[9];
}

void task_8563(void *parm)
{
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    if (!rtc.begin())
    {

        while (1)
        {

            delay(10);
        }
    }

    if (rtc.lostPower())
    {
        ESP_LOGE(TAG, "RTC is NOT initialized, let's set the time!");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

        rtc.start();
    }
    rtc.start();

    DateTime now = rtc.now();
    ESP_LOGE(TAG, "rtc_unixtime:%ld", now.unixtime());
    esp32Time.setTime(now.unixtime());

    if (memory.begin())
    {

        uint8_t bytes233[10];
        uint16_t byte_data;

        uint32_t start_time, end_time;
        memory.readData(0x00, bytes233, 10);
        extract_data_from_bytes(bytes233, &start_time, &end_time, &byte_data);
        work_rec.putBytes(String(start_time).c_str(), bytes233, 10);

        ESP_LOGE(TAG, "start_time:%s end_time:%s ss:%s id:%s", String(start_time).c_str(), String(end_time).c_str(), String(end_time - start_time).c_str(), String(byte_data).c_str());
    }
    // DateTime on_time = rtc.now();
    uint32_t on_stamp = esp32Time.getEpoch();
    // vTaskDelete(NULL);

    while (true)
    {
        if (blackboard.face_auth_id)
        {
            blackboard.car_sta.deviceStatus = DEV_AUTH;

            // DateTime now = rtc.now();
            // uint32_t now_stamp__rtc = now.unixtime();
            /*  */
            uint32_t now_stamp = esp32Time.getEpoch();

            // ESP_LOGD(TAG, "now_stamp:%s rtc_time:%s", String(now_stamp).c_str(), String(now_stamp__rtc).c_str());
            ESP_LOGD(TAG, "now_stamp:%s ", String(now_stamp).c_str());

            if (1)
            {
                uint8_t byte_data = 42;
                uint8_t bytes[10];

                store_data_to_bytes(blackboard.car_sta.on_timestamp, now_stamp, blackboard.face_auth_id, bytes);
                memory.writeData(0x00, bytes, 10);
            }
        }
        else if (0)
        {
            DateTime now = rtc.now();
            uint32_t now_stamp__rtc = now.unixtime();
            /*  */
            uint32_t now_stamp = esp32Time.getEpoch();

            ESP_LOGE(TAG, "esp_stamp:%s rtc_time:%s", String(now_stamp).c_str(), String(now_stamp__rtc).c_str());
        }
        delay(5 * 1000);
    }

    vTaskDelete(NULL);
}