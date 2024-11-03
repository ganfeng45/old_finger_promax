#include "blackboard.h"
#include "zc_face.h"
/* nvs */
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#define TAG "app_face"
SemaphoreHandle_t xSema_FACE = NULL;
TaskHandle_t faceID_handle = NULL;
bool faceId_control = true;
#define FC_PW_PIN GPIO_NUM_2
extern void mp3_player_dec_d(void *parm);

Zc_Face face = Zc_Face(&Serial);
static CMD_MP3_PLAYER mp3_face;
static CMD_BLE_REPLY ble_face;
StaticJsonDocument<1024> doc_replay;

bool enroll_face(uint16_t *res)
{
    uint16_t templateCount = 0;
    mp3_face.mp3_path = "/spiffs/look_cam.mp3";
    broker.publish("mp3_player", &mp3_face);
    bool enroll_res = true;

    uint8_t data[35] = {0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x01, 0x0A};
    // face.getID();
    face.clean_enroll();

    for (int i = 0; i < 5; i++)
    {

        if (enroll_res)
        {

            Face_Packet packet(0x13, 0x23, data);

            packet.data[33] = 0x01 << i;
            face.writeStructuredPacket(packet);
            while (true)
            {
                face.getStructuredPacket(&packet, 1000);
                if (packet.cmd_id == 0x00)
                {
                    break;
                }
                else
                {
                    switch (i)
                    {
                    case 0:
                        mp3_face.mp3_path = "/spiffs/look_cam.mp3";
                        broker.publish("mp3_player", &mp3_face);
                        ESP_LOGI(TAG, "中间");
                        break;

                    case 1:
                        mp3_face.mp3_path = "/spiffs/look_right.mp3";
                        broker.publish("mp3_player", &mp3_face);
                        ESP_LOGI(TAG, "右边");
                        break;

                    case 2:
                        mp3_face.mp3_path = "/spiffs/look_left.mp3";
                        broker.publish("mp3_player", &mp3_face);
                        ESP_LOGI(TAG, "左边");
                        break;

                    case 3:
                        mp3_face.mp3_path = "/spiffs/look_down.mp3";
                        broker.publish("mp3_player", &mp3_face);
                        ESP_LOGI(TAG, "下边");
                        break;
                    case 4:
                        mp3_face.mp3_path = "/spiffs/look_up.mp3";
                        broker.publish("mp3_player", &mp3_face);
                        ESP_LOGI(TAG, "上边");
                        break;

                    default:
                        return false;

                        break;
                    }
                    ESP_LOGI(TAG, "note");
                }
            }
            if (packet.cmd_id == 0x00)
            {
                if (packet.data[0] == 0x013 && packet.data[1] == 0x00)
                {
                    ESP_LOGI(TAG, "录入成功 mid: 0x%X result:0x%X", packet.data[0], packet.data[1]);
                    // Serial.println(packet.data[4], BIN);
                    if (i == 4)
                    {
                        (templateCount) = packet.data[2];
                        (templateCount) <<= 8;
                        (templateCount) |= packet.data[3];
                        delay(1000);

                        ESP_LOGI(TAG, "******完全录入成功-id:%d", (templateCount));
                        *res = templateCount;
                    }
                }
                else
                {
                    ESP_LOGI(TAG, "录入失败 mid: 0x%X result:0x%X", packet.data[0], packet.data[1]);
                    enroll_res = false;
                }
            }
        }
        else
        {
            ESP_LOGI(TAG, "上一次失败");
        }
    }
    // if (enroll_res)

    return enroll_res;
}
void face_enroll_dec(void *param)
{
    if (xSemaphoreTake(xSema_FACE, portMAX_DELAY) == pdTRUE)
    {

        // modem_TTS("指纹采集");
        CMD_IOT_CL_FG *info = (CMD_IOT_CL_FG *)(param);
        uint16_t face_enroll_id = 0;

        if (enroll_face(&face_enroll_id))
        {
            doc_replay.clear();
            // DynamicJsonDocument doc_save(1024);
            doc_replay["id"] = face_enroll_id;
            doc_replay["driver_name"] = info->driver_name;
            doc_replay["driver_num"] = info->driver_num;
            String json2Str;
            serializeJson(doc_replay, json2Str);
            // doc_save.clear();
            doc_replay["command"] = "add_face";
            if (fc_cfg_local.putString(String(face_enroll_id).c_str(), json2Str))
            {
                mp3_face.mp3_path = "/spiffs/enroll_ok.mp3";
                ESP_LOGI(TAG, "完全成功-id:%d", face_enroll_id);
                doc_replay["code"] = 200;
            }
            else
            {

                mp3_face.mp3_path = "/spiffs/fail.mp3";
                doc_replay["code"] = 404;
            }
            ble_face.reply.clear();
            serializeJson(doc_replay, ble_face.reply);
            // ble_face.reply = json2Str;
            delay(1000);
        }
        else
        {
            mp3_face.mp3_path = "/spiffs/fail.mp3";
        }
        // broker.publish("mp3_player", &mp3_face);
        mp3_player_dec_d(&mp3_face);
        delay(2000);
        broker.publish("ble_reply", &ble_face);
        xSemaphoreGive(xSema_FACE);
        vTaskDelete(NULL);
    }
}
bool face_enroll_r(uint16_t *face_enroll_id)
{
    if (xSemaphoreTake(xSema_FACE, 2000) == pdTRUE)
    {
        mp3_face.mp3_path = "/spiffs/look_cam.mp3";
        broker.publish("mp3_player", &mp3_face);
        delay(2000);
        // uint16_t face_enroll_id = 0;
        if (enroll_face(face_enroll_id))
        {
            delay(2000);
            mp3_face.mp3_path = "/spiffs/enroll_ok.mp3";
            broker.publish("mp3_player", &mp3_face);
            delay(2000);
            xSemaphoreGive(xSema_FACE);

            // return face_enroll_id;
            return true;
        }
        mp3_face.mp3_path = "/spiffs/fail.mp3";
        broker.publish("mp3_player", &mp3_face);
        delay(2000);
        xSemaphoreGive(xSema_FACE);
        return false;
    }
    mp3_face.mp3_path = "/spiffs/fail.mp3";
    broker.publish("mp3_player", &mp3_face);
    delay(2000);
    return false;
}
void face_enroll(const String &topic, void *param)
{
    CMD_IOT_CL_FG *data = (CMD_IOT_CL_FG *)param;
    xTaskCreatePinnedToCore(&face_enroll_dec,  // task
                            "face_enroll_dec", // task name
                            1024 * 10,         // stack size
                            param,             // parameters
                            5,                 // priority
                            NULL,              // returned task handle
                            1                  // pinned core
    );
}
uint8_t face_empty_d()
{
    return face.empty();
}
uint8_t face_delete_one(uint16_t index)
{
    return face.deleteone(index);
}
void face_empty(const String &topic, void *param)
{
    if (xSemaphoreTake(xSema_FACE, portMAX_DELAY) == pdTRUE)
    {

        // DynamicJsonDocument doc_save(1024);
        doc_replay.clear();
        doc_replay["comand"] = "empty_face";
        if (face.empty())
        {
            mp3_face.mp3_path = "/spiffs/fail.mp3";
            doc_replay["code"] = 404;
        }
        else
        {
            mp3_face.mp3_path = "/spiffs/empty_ok.mp3";
            fc_cfg_local.clear();
            doc_replay["code"] = 200;
        }
        ble_face.reply.clear();
        serializeJson(doc_replay, ble_face.reply);
        broker.publish("mp3_player", &mp3_face);
        broker.publish("ble_reply", &ble_face);
        xSemaphoreGive(xSema_FACE);
    }
}
void face_getUsrlist(const String &topic, void *param)
{
    // if (xSemaphoreTake(xSema_FACE, portMAX_DELAY) == pdTRUE)
    {
        int pn, ps;
        CMD_FACE_LIST *data = (CMD_FACE_LIST *)param;
        pn = data->page_num;
        ps = data->page_size;
        ESP_LOGI(TAG, "pn:%d ps:%d", pn, ps);
        // printAllNVSKeys("finger_cfg");
        doc_replay.clear();
        String replay_str;
        doc_replay["code"] = 200;
        doc_replay["cmd"] = "getUser";
        doc_replay["pN"] = pn;

        JsonArray list = doc_replay.createNestedArray("list");
        // list.add("1#张三");
        // list.add("2#李四");
        int now_conut = 0;
        // return;
        const char *namespace_name = "face_cfg";
        nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
        // while (res == ESP_OK)
        while (it != NULL)
        {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
            ESP_LOGE(TAG, "key '%s', type '%d' \n", info.key, info.type);
            if (info.type == NVS_TYPE_STR)
            {
                // Serial.println(all_prt.getString(info.key));
                now_conut++;
                if (now_conut > (pn - 1) * ps && now_conut < (pn)*ps + 1)
                {
                    String str_temp = fc_cfg_local.getString(info.key);
                    StaticJsonDocument<256> doc_name;

                    DeserializationError error = deserializeJson(doc_name, (str_temp));
                    if (!error)
                    {
                        // list.add(String(info.key) + "#" + (fc_cfg_local.getString(info.key)));
                        list.add(String(info.key) + "#" + doc_name["driver_name"].as<String>());
                    }
                    ESP_LOGI(TAG, "index: %d 列表：%s", now_conut, (String(fc_cfg_local.getInt(info.key)) + "#" + String(info.key)).c_str());
                }
            }
            if (now_conut > (pn)*ps + 1)
            {
                break;
            }
            it = nvs_entry_next(it);

        }
        nvs_release_iterator(it);
        ble_face.reply.clear();
        serializeJson(doc_replay, ble_face.reply);
        broker.publish("ble_reply", &ble_face);
        // xSemaphoreGive(xSema_FACE);
    }
}
static int face_wait = 0;
extern void close_ble();
// extern uint32_t on_stamp;
void face_getID(void *parm)
{
    while (true)
    {
        if (faceId_control && xSemaphoreTake(xSema_FACE, 1000) == pdTRUE)
        {
            if (face_wait++ > 10)
            {
                ESP_LOGI(TAG, "face_wait %d", face_wait);
                mp3_face.mp3_path = "/spiffs/fail.mp3";
                broker.publish("mp3_player", &mp3_face);
                delay(3000);
                digitalWrite(FC_PW_PIN, LOW);
                close_ble();
                xSemaphoreGive(xSema_FACE);
                vTaskDelete(NULL);
            }
            ESP_LOGI(TAG, "xSema_FACE ok");
            uint16_t facesta = 1;
            uint16_t id = face.getID(&facesta);
            if (id)
            {

                blackboard.face_auth_id = id;
                // DateTime now = rtc.now();
                // uint32_t now_stamp = now.unixtime();
                // on_stamp=esp32Time.getEpoch();
                blackboard.car_sta.on_timestamp = esp32Time.getEpoch();
                if (0)
                {
                    // uint32_t now_stamp = esp32Time.getEpoch();
                    // if (work_rec.putUInt(String(now_stamp).c_str(), id) == 0)
                    // {
                    //     ESP_LOGI(TAG, "work_rec.putUInt fail");
                    // }
                    // else
                    // {
                    //     ESP_LOGI(TAG, "work_rec.putUInt:%s", String(now_stamp).c_str());
                    // }
                    // ESP_LOGI(TAG, "work_rec.putUInt ok---%lu", work_rec.getUInt(String(now_stamp).c_str()));
                }
                ESP_LOGI(TAG, "face.getID:%s", String(id).c_str());
                pinMode(10, OUTPUT);
                digitalWrite(10, HIGH);
                digitalWrite(FC_PW_PIN, LOW);
                mp3_face.mp3_path = "/spiffs/auth.mp3";
                // broker.publish("mp3_player", &mp3_face);
                mp3_player_dec_d(&mp3_face);
                delay(3 * 1000);
                close_ble();
                xSemaphoreGive(xSema_FACE);
                vTaskDelete(NULL);
            }
            else if (facesta == 0)
            {
                ESP_LOGI(TAG, "人脸未注册 %s", String(facesta).c_str());

                mp3_face.mp3_path = "/spiffs/fail.mp3";
                // broker.publish("mp3_player", &mp3_face);
                mp3_player_dec_d(&mp3_face);
            }
            xSemaphoreGive(xSema_FACE);
        }
        else
        {
            ESP_LOGI(TAG, "xSema_FACE fail");
        }
        delay(2000);
    }
}
void facdID_ctl(bool onoff)
{
    // 关闭人脸识别
    faceId_control = onoff;
}
void task_face(void *parm)
{
    // pinMode(FC_PW_PIN, OUTPUT);
    // digitalWrite(FC_PW_PIN, HIGH);
    delay(500);

    // 反序
    Serial.begin(115200, 134217756U, 21, 20, false, 20000UL, (uint8_t)112U);
    // 正序
    // Serial.begin(115200, 134217756U, 20, 21, false, 20000UL, (uint8_t)112U);
    delay(500);

    while (0)
    {
        uint8_t data[2] = {0x00, 0x02};
        Face_Packet packet(0x12, 0x02, data);
        // writeStructuredPacket(packet);
        face.getStructuredPacket(&packet, 1000);
        if (packet.cmd_id == 0x01)
        {
            ESP_LOGE(TAG, "face POWER OK");

            break;
        }
        else
        {
            ESP_LOGE(TAG, "face POWER fail");
        }
    }

    // if (0)
    if (0)
    {
        while (face.getFaceSta())
        {
            /* code */
            ESP_LOGE(TAG, "face init");

            digitalWrite(2, LOW);
            delay(1000);

            digitalWrite(2, HIGH);
            delay(1000);
        }
    }

    if (face.getFaceSta())
    {

        ESP_LOGE(TAG, "face error");
        delay(3 * 1000);
        mp3_face.mp3_path = "/spiffs/fail.mp3";
        broker.publish("mp3_player", &mp3_face);
        vTaskDelete(NULL);
    }
    else
    {
        esp_log_level_set(TAG, ESP_LOG_VERBOSE);
        ESP_LOGI(TAG, "face OK");
        broker.subscribe("face_enroll", face_enroll);
        broker.subscribe("face_empty", face_empty);
        broker.subscribe("get_Facelist", face_getUsrlist);
        xSema_FACE = xSemaphoreCreateBinary();
        xSemaphoreGive(xSema_FACE);
        xTaskCreatePinnedToCore(face_getID, "face_getID", 4096 * 1, NULL, 5, NULL, 0);
    }

    vTaskDelete(NULL);
}