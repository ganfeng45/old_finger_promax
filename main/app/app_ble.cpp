// #include "tasks.h"
#include "blackboard.h"
// #include "blackboard_c.h"
// #include "DEV_PIN.h"
#include "util/myutil.h"
#include <stdio.h>
#include "SPIFFS.h"
// #include <Update.h>
// #include "mbedtls/md5.h" // MD5库

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "blackboard.h"
#include "esp_bt_device.h"

#define TAG "app_test"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
// static CMD_MP3_PLAYER mp3_ble_test;
SemaphoreHandle_t xSema_BLE = NULL;
String rxValue;
#define SERVICE_UUID "0000FFE0-0000-1000-8000-00805F9B34FB" // UART service UUID
#define CHARACTERISTIC_UUID_RX "0000FFE1-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
extern void facdID_ctl(bool onoff);


struct DATA_CMD
{
    DATA_CMD(uint8_t type, uint8_t length, uint8_t *data)
    {

        ESP_LOGD(TAG, "CMD:%02X DATA_LEN: %d", type, length);
        this->CMD_TYPE = type;
        this->DATA_LEN = length;
        memcpy(this->DATA_ARRAY, data, length);
    }
    uint8_t DST_ID;
    uint8_t SRC_ID;
    uint8_t CMD_TYPE;
    uint8_t DATA_LEN;
    uint8_t DATA_ARRAY[64];
    uint8_t CHECK_SUM = 0;
    uint8_t END_TAG = 0XAA;
};

void printBuffer(const uint8_t *buffer, size_t size)
{
    String buffer_str;
    for (size_t i = 0; i < size; ++i)
    {
        buffer_str += String(buffer[i], HEX);
        if (i < size - 1)
            buffer_str += " ";
    }
    ESP_LOGD(TAG, "ble reply: %s", buffer_str.c_str());
}

void cmd_reply(DATA_CMD &cmd, size_t bufferSize)
{
    uint8_t buffer[128] = {0};
    size_t requiredSize = sizeof(cmd.DST_ID) + sizeof(cmd.SRC_ID) + sizeof(cmd.CMD_TYPE) +
                          sizeof(cmd.DATA_LEN) + cmd.DATA_LEN + sizeof(cmd.CHECK_SUM) +
                          sizeof(cmd.END_TAG);

    // Check if buffer size is sufficient
    if (bufferSize < requiredSize)
    {
        // Handle error: buffer too small
        return;
    }
    cmd.DST_ID = 0x20;
    cmd.SRC_ID = 0x01;
    // Copy fields to buffer
    size_t offset = 0;
    memcpy(buffer + offset, &cmd.DST_ID, sizeof(cmd.DST_ID));
    offset += sizeof(cmd.DST_ID);
    memcpy(buffer + offset, &cmd.SRC_ID, sizeof(cmd.SRC_ID));
    offset += sizeof(cmd.SRC_ID);
    memcpy(buffer + offset, &cmd.CMD_TYPE, sizeof(cmd.CMD_TYPE));
    offset += sizeof(cmd.CMD_TYPE);
    memcpy(buffer + offset, &cmd.DATA_LEN, sizeof(cmd.DATA_LEN));
    offset += sizeof(cmd.DATA_LEN);
    memcpy(buffer + offset, cmd.DATA_ARRAY, cmd.DATA_LEN);
    offset += cmd.DATA_LEN;
    /*  */
    cmd.CHECK_SUM = cmd.SRC_ID + cmd.CMD_TYPE + cmd.DATA_LEN;
    for (int i = 0; i < cmd.DATA_LEN; i++)
    {
        cmd.CHECK_SUM += cmd.DATA_ARRAY[i];
    }
    /*  */
    memcpy(buffer + offset, &cmd.CHECK_SUM, sizeof(cmd.CHECK_SUM));
    offset += sizeof(cmd.CHECK_SUM);
    memcpy(buffer + offset, &cmd.END_TAG, sizeof(cmd.END_TAG));
    printBuffer(buffer, requiredSize);
    pRxCharacteristic->setValue(buffer, requiredSize);
    pRxCharacteristic->notify();
}

void serializeDataCmd(DATA_CMD &cmd, uint8_t *buffer, size_t bufferSize)
{
    size_t requiredSize = sizeof(cmd.DST_ID) + sizeof(cmd.SRC_ID) + sizeof(cmd.CMD_TYPE) +
                          sizeof(cmd.DATA_LEN) + cmd.DATA_LEN + sizeof(cmd.CHECK_SUM) +
                          sizeof(cmd.END_TAG);

    // Check if buffer size is sufficient
    if (bufferSize < requiredSize)
    {
        // Handle error: buffer too small
        return;
    }
    cmd.DST_ID = 0x20;
    cmd.SRC_ID = 0x01;
    // Copy fields to buffer
    size_t offset = 0;
    memcpy(buffer + offset, &cmd.DST_ID, sizeof(cmd.DST_ID));
    offset += sizeof(cmd.DST_ID);
    memcpy(buffer + offset, &cmd.SRC_ID, sizeof(cmd.SRC_ID));
    offset += sizeof(cmd.SRC_ID);
    memcpy(buffer + offset, &cmd.CMD_TYPE, sizeof(cmd.CMD_TYPE));
    offset += sizeof(cmd.CMD_TYPE);
    memcpy(buffer + offset, &cmd.DATA_LEN, sizeof(cmd.DATA_LEN));
    offset += sizeof(cmd.DATA_LEN);
    memcpy(buffer + offset, cmd.DATA_ARRAY, cmd.DATA_LEN);
    offset += cmd.DATA_LEN;
    /*  */
    cmd.CHECK_SUM = cmd.SRC_ID + cmd.CMD_TYPE + cmd.DATA_LEN;
    for (int i = 0; i < cmd.DATA_LEN; i++)
    {
        cmd.CHECK_SUM += cmd.DATA_ARRAY[i];
    }
    /*  */
    memcpy(buffer + offset, &cmd.CHECK_SUM, sizeof(cmd.CHECK_SUM));
    offset += sizeof(cmd.CHECK_SUM);
    memcpy(buffer + offset, &cmd.END_TAG, sizeof(cmd.END_TAG));
}
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        ESP_LOGD(TAG, "onConnect");
        play_mp3_dec("/spiffs/link.mp3");

        // facdID_ctl(false);
        // deviceConnected = true;
        // mp3_ble_test.mp3_path = "/spiffs/connect.mp3";
        // broker.publish("mp3_player", &mp3_ble_test);
        // char *handle_arg = "/spiffs/link.mp3";
        // esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        delay(1000);
    };

    void onDisconnect(BLEServer *pServer)
    {
        ESP_LOGD(TAG, "onDisconnect");

        // facdID_ctl(true);
        // deviceConnected = false;
        // mp3_ble_test.mp3_path = "/spiffs/disconnect.mp3";
        // broker.publish("mp3_player", &mp3_ble_test);
    }
};


int ble_reply(DATA_CMD data_cmd, uint8_t *buffer, int buffer_size)
{

    return 0; // Return the number of bytes written to the buffer
}
StaticJsonDocument<1024> doc_fc_info;
String fc_info(String name, String id)
{
    doc_fc_info.clear();
    doc_fc_info["name"] = name;
    doc_fc_info["id"] = id;
    String json2Str;
    serializeJson(doc_fc_info, json2Str);
    return json2Str;
}
String uint2String(uint8_t *data, int lenght)
{
    String str = "";
    for (int i = 0; i < lenght; i++)
    {
        str += (char)data[i];
    }
    return str;
}

String enroll_info_comb;
//  bool face_enroll_r();
extern bool face_enroll_r(uint16_t *face_enroll_id);

void face_enroll_rdec(void *parm)
{
    ESP_LOGD(TAG, "face_enroll_rdec");
    uint16_t res = 0;
    bool en_res = face_enroll_r(&res);

    ESP_LOGD(TAG, "face_enroll_rdec res:%d", en_res);
    if (en_res)
    {
        ESP_LOGI(TAG, "face_enroll_rdec ok res:%d", enroll_info_comb.length());
        fc_cfg_local.putString(String(res).c_str(), enroll_info_comb);
    }
    vTaskDelete(NULL);
}
extern uint8_t face_empty_d();
extern uint8_t face_delete_one(uint16_t index);
extern int fg_delete_one(uint16_t id);

void empty_all_dec(void *parm)
{
    ESP_LOGD(TAG, "empty_all_dec");
#ifdef DEV_FG
    if (fg_delete_one(0xffff) == 0)
    {
        ESP_LOGD(TAG, "fg.empty() ok");
        fg_cfg_local.clear();
        uint8_t data[1] = {0}; // Example data
        DATA_CMD cmd(0x09, sizeof(data), data);
        cmd_reply(cmd, 7);
    }
    else
    {
        uint8_t data[1] = {0}; // Example data
        data[0] = 0x01;
        DATA_CMD cmd(0x09, sizeof(data), data);
        cmd_reply(cmd, 7);
    }
#else
    if (face_empty_d() == 0)
    {
        ESP_LOGD(TAG, "face.empty() ok");
        fc_cfg_local.clear();
        uint8_t data[1] = {0}; // Example data
        DATA_CMD cmd(0x09, sizeof(data), data);
        cmd_reply(cmd, 7);
    }
    else
    {
        uint8_t data[1] = {0}; // Example data
        data[0] = 0x01;
        DATA_CMD cmd(0x09, sizeof(data), data);
        cmd_reply(cmd, 7);
    }
#endif
    vTaskDelete(NULL);
}
extern void extract_data_from_bytes(const uint8_t bytes[9], uint32_t *data1, uint32_t *data2, uint16_t *data3);
extern void finger_collect_dec(void *param);
extern void finger_collect(const String &topic, void *param);
extern int print_all(String nms);

// uint8_t *hexStringToBytes(const char *hexString, size_t *bytesLength)
// {
//   size_t length = strlen(hexString);
//   *bytesLength = (length + 1) / 2;
//   uint8_t *bytes = (uint8_t *)malloc(*bytesLength);
//   for (size_t i = 0; i < *bytesLength; i++)
//   {
//     sscanf(hexString + i * 2, "%2hhx", &bytes[i]);
//   }
//   return bytes;
// }

void parseDataCmd(String hexValue)
{
    DATA_CMD data_cmd(hexValue[2], hexValue[3], reinterpret_cast<uint8_t *>(&hexValue[4]));
    ESP_LOGI(TAG, "CMD_TYPE: %02X", data_cmd.CMD_TYPE);
    if (data_cmd.CMD_TYPE == 0x84)
    {
        /* 读取rtc时间 */
        ESP_LOGD(TAG, "rtc cmd : %02X", data_cmd.CMD_TYPE);
        uint8_t ble_buffer[12] = {0x20, 0x01, 0x84, 0x06, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8B, 0xAA};
        DateTime now = rtc.now();
        uint8_t years = now.year() - 2000;
        uint8_t data[6] = {now.second(), now.minute(), now.hour(), now.day(), now.month(), years}; // Example data

        DATA_CMD cmd(0x84, sizeof(data), data);
        uint8_t buffer[20];
        serializeDataCmd(cmd, buffer, sizeof(buffer));
        printBuffer(buffer, 12);
        pRxCharacteristic->setValue(buffer, 12);
        pRxCharacteristic->notify();
    }
    else if (data_cmd.CMD_TYPE == 0x02)
    {
        ESP_LOGD(TAG, "设置rtc时间 cmd : %02X", data_cmd.CMD_TYPE);
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
        rtc.adjust(DateTime(data_cmd.DATA_ARRAY[5], data_cmd.DATA_ARRAY[4], data_cmd.DATA_ARRAY[3], data_cmd.DATA_ARRAY[2], data_cmd.DATA_ARRAY[1], data_cmd.DATA_ARRAY[0]));
        DateTime now = rtc.now();
        uint32_t now_stamp = now.unixtime();
        ESP_LOGE(TAG, "now_stamp:%s", String(now_stamp).c_str());
        uint8_t data[1] = {0}; // Example data
        DATA_CMD cmd(0x02, sizeof(data), data);
        cmd_reply(cmd, sizeof(data) + 6);
    }
    else if (data_cmd.CMD_TYPE == 0x81)
    {
        /* 获取系统参数 */
        ESP_LOGD(TAG, "sysinfo cmd : %02X", data_cmd.CMD_TYPE);
        uint8_t buffer[50] = {0};

        uint8_t data[29] = {0}; // Example data
#ifdef DEV_FG
        data[23] = 0x03;
        data[24 + 2] = print_all("finger_cfg");
        data[25 + 2] = 50;
#else
        data[23] = 0x04;
        data[24] = print_all("face_cfg");
        data[25] = 50;
#endif

        DATA_CMD cmd(0x81, countNonZeroElements(data, sizeof(data)), data);
        // cmd_reply(cmd, 35);
        cmd_reply(cmd, countNonZeroElements(data, sizeof(data)) + 6);

        // printBuffer(buffer, 35);
        // pRxCharacteristic->setValue(buffer, 35);
        // pRxCharacteristic->notify();
    }
    else if (data_cmd.CMD_TYPE == 0x53)
    {
        ESP_LOGD(TAG, "注册指纹 cmd : %02X", data_cmd.CMD_TYPE);
        CMD_ENROLL cmd_enrool;
        uint8_t data[5] = {0}; // Example data

        finger_collect_dec(&cmd_enrool);
        ESP_LOGD(TAG, " cmd enrool : %d %s", cmd_enrool.index, cmd_enrool.reply.c_str());
        if (cmd_enrool.index)
        {
            data[0] = 0x00;
            data[1] = (cmd_enrool.index << 8) & 0xFF;
            data[2] = (cmd_enrool.index) & 0xFF;
        }
        else
        {
            data[0] = 0x01;
            data[1] = 0xFF;
            data[2] = 0xFF;
        }
        DATA_CMD cmd(0x53, sizeof(data), data);
        cmd_reply(cmd, 11);
    }
    else if (data_cmd.CMD_TYPE == 0x54)
    {
        ESP_LOGD(TAG, "绑定指纹 cmd : %02X", data_cmd.CMD_TYPE);
        uint16_t enroll_id = (data_cmd.DATA_ARRAY[0] << 8) | (data_cmd.DATA_ARRAY[1]);
        ESP_LOGD(TAG, "finger_enroll_rdec res:%d", enroll_id);
        if (true)
        {
            // 复制enrollData到data_cmd.DATA_ARRAY
            // data_cmd.DATA_ARRAY[0] = (enroll_id >> 8) & 0xFF; // Extract the high byte
            // data_cmd.DATA_ARRAY[1] = enroll_id & 0xFF;
            size_t pres = fg_cfg_local.putBytes(String(enroll_id).c_str(), data_cmd.DATA_ARRAY, data_cmd.DATA_LEN);
            ESP_LOGI(TAG, "face_enroll_pes:%d", pres);
            uint8_t data[3];
            data[0] = pres ? 0x0 : 0x1;
            data[1] = print_all("finger_cfg");
            data[2] = 50;
            DATA_CMD cmd(0x54, sizeof(data), data);
            cmd_reply(cmd, 9);
        }
    }
    else if (data_cmd.CMD_TYPE == 0x55)
    {
        uint16_t delete_id = data_cmd.DATA_ARRAY[0] << 8 | data_cmd.DATA_ARRAY[1];
        ESP_LOGD(TAG, "删除指纹 cmd : %02X id:%d", data_cmd.CMD_TYPE, delete_id);
        if (!fg_delete_one(delete_id))
        {
            ESP_LOGD(TAG, "HHHH fg_delete_one ok");
            if (fg_cfg_local.remove(String(delete_id).c_str()))
            {
                ESP_LOGD(TAG, "SSS fg_delete_one ok");
                uint8_t data[3] = {0}; // Example data
                DATA_CMD cmd(0x55, sizeof(data), data);
                cmd_reply(cmd, sizeof(data) + 6);
            }
        }
    }

    else if (data_cmd.CMD_TYPE == 0x63)
    {
        ESP_LOGD(TAG, "注册人脸 cmd : %02X", data_cmd.CMD_TYPE);
        uint8_t data[3] = {0}; // Example data
        uint16_t enroll_id = 0;
        bool enroll_res = face_enroll_r(&enroll_id);
        ESP_LOGD(TAG, "face_enroll_rdec res:%d", enroll_res);
        if (enroll_res)
        {
            data[0] = 0x00;
            data[1] = (enroll_id << 8) & 0xFF;
            data[2] = (enroll_id) & 0xFF;
        }
        else
        {
            data[0] = 0x01;
            data[1] = 0xFF;
            data[2] = 0xFF;
        }

        DATA_CMD cmd(0x63, sizeof(data), data);
        cmd_reply(cmd, 9);
    }
    else if (data_cmd.CMD_TYPE == 0x64)
    {
        ESP_LOGD(TAG, "绑定人脸信息 cmd : %02X", data_cmd.CMD_TYPE);
        // printBuffer(data_cmd.DATA_ARRAY, data_cmd.DATA_LEN);
        // uint8_t enrollData[data_cmd.DATA_LEN];
        // memcpy(enrollData, data_cmd.DATA_ARRAY, data_cmd.DATA_LEN);
        uint16_t enroll_id = (data_cmd.DATA_ARRAY[0] << 8) | (data_cmd.DATA_ARRAY[1]);
        // bool enroll_res = face_enroll_r(&enroll_id);
        ESP_LOGD(TAG, "face_enroll_rdec res:%d", enroll_id);
        if (true)
        {
            // 复制enrollData到data_cmd.DATA_ARRAY
            // data_cmd.DATA_ARRAY[0] = (enroll_id >> 8) & 0xFF; // Extract the high byte
            // data_cmd.DATA_ARRAY[1] = enroll_id & 0xFF;
            size_t pres = fc_cfg_local.putBytes(String(enroll_id).c_str(), data_cmd.DATA_ARRAY, data_cmd.DATA_LEN);
            ESP_LOGI(TAG, "face_enroll_pes:%d", pres);
            uint8_t data[3];
            data[0] = pres ? 0x0 : 0x1;
            data[1] = print_all("face_cfg");
            data[2] = 50;
            DATA_CMD cmd(0x64, sizeof(data), data);
            cmd_reply(cmd, 9);
        }

        // xTaskCreate(face_enroll_rdec, "face_enroll_dec", 1024 * 5, NULL, 5, NULL);

        // static CMD_IOT_CL_FG info_fg_cl;
        // info_fg_cl.driver_name = str;
        // info_fg_cl.driver_num = str;
        // broker.publish("face_enroll", &info_fg_cl);
    }
    else if (data_cmd.CMD_TYPE == 0x09)
    {
        /* 清空全部信息 */
        ESP_LOGD(TAG, "empty all : %02X", data_cmd.CMD_TYPE);
        xTaskCreate(empty_all_dec, "empty_all_dec", 1024 * 5, NULL, 5, NULL);
    }
    else if (data_cmd.CMD_TYPE == 0x60)
    {
        // fc_cfg_local.freeEntries
        // size_t pres = fc_cfg_local.putString(String("res").c_str(), "enroll_info_comb");
        // ESP_LOGD(TAG, "pres:%d", pres);
        ESP_LOGD(TAG, "获取本地人脸信息 cmd : %02X", data_cmd.CMD_TYPE);
        const char *namespace_name = "face_cfg";
        //  = NULL;
        nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
        while (it != NULL)
        {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
            String driver_info = fc_cfg_local.getString(info.key);
            ESP_LOGE(TAG, "key '%s', type '%d' data:%s\n", info.key, info.type, driver_info.c_str());
            uint8_t data[34] = {0}; // Example data
            fc_cfg_local.getBytes(info.key, data, 34);

            DATA_CMD cmd(0x60, countNonZeroElements(data, sizeof(data)), data);
            ESP_LOGI(TAG, "face_cfg_local_data:%d", countNonZeroElements(data, sizeof(data)));
            cmd_reply(cmd, countNonZeroElements(data, sizeof(data)) + 6);

            // if (info.type == NVS_TYPE_STR)
            // {
            //     Serial.println(all_prt.getString(info.key))
            // }
            it = nvs_entry_next(it);
        }
    }
    else if (data_cmd.CMD_TYPE == 0x65)
    {
        uint16_t delete_id = data_cmd.DATA_ARRAY[0] << 8 | data_cmd.DATA_ARRAY[1];
        ESP_LOGD(TAG, "删除人脸 cmd : %02X id:%d", data_cmd.CMD_TYPE, delete_id);
        if (!face_delete_one(delete_id))
        {
            ESP_LOGD(TAG, "HHHH face_delete_one ok");
            if (fc_cfg_local.remove(String(delete_id).c_str()))
            {
                ESP_LOGD(TAG, "SSS face_delete_one ok");
                uint8_t data[3] = {0}; // Example data
                DATA_CMD cmd(0x65, sizeof(data), data);
                cmd_reply(cmd, sizeof(data) + 6);
            }
        }
    }
    else if (data_cmd.CMD_TYPE == 0x61)
    {
        ESP_LOGD(TAG, "获取本地人脸使用信息 cmd : %02X", data_cmd.CMD_TYPE);
        uint8_t page_size = data_cmd.DATA_ARRAY[0];
        int n_count = 0;

        // nvs_iterator_t it = NULL;
        nvs_iterator_t it= nvs_entry_find("nvs", "work_rec", NVS_TYPE_ANY);
        while (it != NULL)
        {
            n_count++;
            uint8_t data[46] = {0}; // Example data
            nvs_entry_info_t info;
            nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
            ESP_LOGE(TAG, "key '%s', type '%d' ", info.key, info.type);
            if (info.type == 66)
            {
                uint8_t data_wk[10] = {0}; // Example data
                work_rec.getBytes(info.key, data_wk, 10);
                uint16_t driver_id;
                uint32_t start_time, end_time;
                extract_data_from_bytes(data_wk, &start_time, &end_time, &driver_id);

                // 将时间戳转换为tm结构

                /* 添加驾驶员信息 */
                fc_cfg_local.getBytes(String(driver_id).c_str(), data, 34);
                /* 开始时间 */
                int drivr_info_len = countNonZeroElements(data, sizeof(data));
                struct tm *timeinfo;
                time_t timestamp = start_time; // 示例时间戳（2024年1月1日的时间戳）
                timeinfo = localtime(&timestamp);
                ESP_LOGE(TAG, "year '%d' ", timeinfo->tm_year);
                data[drivr_info_len] = timeinfo->tm_sec;
                data[drivr_info_len + 1] = timeinfo->tm_min;
                data[drivr_info_len + 2] = timeinfo->tm_hour;
                data[drivr_info_len + 3] = timeinfo->tm_mday;
                data[drivr_info_len + 4] = timeinfo->tm_mon + 1;
                data[drivr_info_len + 5] = timeinfo->tm_year - 100;
                drivr_info_len = countNonZeroElements(data, sizeof(data));
                /* 结束时间 */
                timestamp = end_time;
                timeinfo = localtime(&timestamp);
                ESP_LOGE(TAG, "year '%d' ", timeinfo->tm_year - 100);
                data[drivr_info_len] = timeinfo->tm_sec;
                data[drivr_info_len + 1] = timeinfo->tm_min;
                data[drivr_info_len + 2] = timeinfo->tm_hour;
                data[drivr_info_len + 3] = timeinfo->tm_mday;
                data[drivr_info_len + 4] = timeinfo->tm_mon + 1;
                data[drivr_info_len + 5] = timeinfo->tm_year - 100;
                drivr_info_len = countNonZeroElements(data, sizeof(data));
                DATA_CMD cmd(0x61, drivr_info_len, data);
                cmd_reply(cmd, drivr_info_len + 6);
            }

            it = nvs_entry_next(it);
            if (n_count >= page_size)
            {
                break;
            }
        }
        nvs_release_iterator(it);
    }
    else if (data_cmd.CMD_TYPE == 0x08)
    {
        ESP_LOGD(TAG, "设置蓝牙名称 cmd : %02X", data_cmd.CMD_TYPE);
        if (!data_cmd.DATA_ARRAY[0])
        {
            return;
        }
        uint8_t name_len = data_cmd.DATA_ARRAY[0];
        uint8_t ble_name[255];
        memcpy(ble_name, data_cmd.DATA_ARRAY + 1, name_len);
        // str(ble_name, data_cmd.DATA_ARRAY+1, name_len);
        // strncmp(ble_name, data_cmd.DATA_ARRAY+1, name_len);
        ble_name[name_len] = '\0';
        ESP_LOGD(TAG, "设置蓝牙名称 cmd : %s", ble_name);
        uint8_t data[1];
        String bleNameString(reinterpret_cast<const char *>(ble_name), name_len);
        ESP_LOGD(TAG, "设置蓝牙名称233 cmd : %s", bleNameString.c_str());

        // if (0)
        if (dev_cfg.putString("DEVICE_NAME", bleNameString) == 0)
        {
            data[0] = 0x01;
        }
        else
        {
            data[0] = 0x00;
        }
        DATA_CMD cmd(0x61, 1, data);
        cmd_reply(cmd, 1 + 6);
        delay(3 * 1000);
        ESP.restart();
    }
}
void task_cmd(void *pvParameters)
{
    // ESP_LOGD(TAG, "Received Value: %s", hexValue.c_str());
    if (xSemaphoreTake(xSema_BLE, 0) == pdTRUE)
    {
        ESP_LOGD(TAG, "task_cmd");
        String hexValue;
        for (size_t i = 0; i < rxValue.length(); i++)
        {
            char buf[5];
            sprintf(buf, " %02X", (unsigned char)rxValue[i]);
            hexValue += String(buf);
        }

        parseDataCmd(rxValue);
        xSemaphoreGive(xSema_BLE);
    }
    else
    {
        ESP_LOGW(TAG, "parseDataCmd task busy");
    }
    vTaskDelete(NULL);
}
class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        rxValue = stdStringToArduinoString(pCharacteristic->getValue());
        xTaskCreatePinnedToCore(&task_cmd,  // task
                                "task_cmd", // task name
                                4096,       // stack size
                                NULL,       // parameters
                                10,         // priority
                                NULL,       // returned task handle
                                0           // pinned core
        );
    }
};

void ble_task_setup()
{
    xSema_BLE = xSemaphoreCreateBinary();
    xSemaphoreGive(xSema_BLE);
    String dev_name = "demo";
    if (dev_cfg.getString("DEVICE_NAME", "001") == "001")
    {
        uint64_t chipId = ESP.getEfuseMac();
        dev_name = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
        dev_name = dev_name.substring(0, 6);
    }
    else
    {
        dev_name = dev_cfg.getString("DEVICE_NAME", "001");
        if (dev_name.length() > 16)
        {
            // uint64_t chipId = ESP.getEfuseMac();
            // dev_name = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
            dev_name = dev_name.substring(10);
            ESP_LOGE(TAG, "MACstr:%s", dev_name.c_str());
        }
    }
    ESP_LOGE(TAG, "ble_name:%s", dev_name.c_str());

    BLEDevice::init(("LY_LOCK_" + dev_name).c_str());
    // BLEDevice::init("LY_LOCK_001");
    // const uint8_t *bleAddr = esp_bt_dev_get_address(); // 查询蓝牙的mac地址，务必要在蓝牙初始化后方可调用！
    // // sprintf(MACstr, "%02x:%02x:%02x:%02x:%02x:%02x\n", bleAddr[0], bleAddr[1], bleAddr[2], bleAddr[3], bleAddr[4], bleAddr[5]);
    // dev_cfg.putString("MACstr", MACstr);
    // ESP_LOGE(TAG, "MACstr:%s", MACstr);
    // 状态服务
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    // pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    // pTxCharacteristic->addDescriptor(new BLE2902());

    pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    pRxCharacteristic->addDescriptor(new BLE2902());

    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisingData;
    String shortuid = "FFE0";
    advertisingData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advertisingData.setPartialServices(BLEUUID(arduinoStringToStdString(shortuid)));
    // BLEAddress addr = BLEDevice::getAddress();
    String bleAddress = stdStringToArduinoString(BLEDevice::getAddress().toString());
    ESP_LOGD(TAG, "bleAddress:%s", bleAddress.c_str());
    esp_bd_addr_t *addr = BLEDevice::getAddress().getNative();
    char cdata[2];
    cdata[0] = 9;
    cdata[1] = 0xFF; // 0x02

    char mac_data[8];
    mac_data[0] = 0x58;
    mac_data[1] = 0x44;

    bleAddress.replace(":", "");

    for (int i = 0; i < 6; i++)
    {
        String byteString = bleAddress.substring(i * 2, i * 2 + 2);
        mac_data[2 + i] = strtol(byteString.c_str(), NULL, 16);
    }

    String(mac_data, 8);
    advertisingData.addData(arduinoStringToStdString(String(cdata, 2) + String(mac_data, 8)));
    String adv_data = stdStringToArduinoString(advertisingData.getPayload());
    ESP_LOGE(TAG, "adv_data:%s", adv_data.c_str());

    pAdvertising->setAdvertisementData(advertisingData);
    pAdvertising->start();

    Serial.println("Waiting for a client connection to notify...");
}
void ble_task_loop()
{

    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

void task_ble(void *params)
{
    delay(1000 * 2);

    ESP_LOGE(TAG, "speed_ble_entry");

    ble_task_setup();
    while (1)
    {
        // rfid_cfg_local.getString("21199412300221", "doc2str_iic");
        ble_task_loop();
        delay(400);
    }
}
