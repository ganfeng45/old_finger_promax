#include "blackboard.h"
#include "util/myutil.h"
#include "Adafruit_Fingerprint.h"
#define TAG "finger"
#define PIN_FG_TOUCH (GPIO_NUM_2)
#define finger_serial Serial
bool fingerId_control = true;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&finger_serial);
SemaphoreHandle_t xSemaFINGER = NULL;
StaticJsonDocument<2048> json_fg_upd;
StaticJsonDocument<2048> doc_modelcode;
static CMD_MP3_PLAYER mp3_fg;

String http_get(const char *url);
CMD_UD_FINGERG fg_info;
TaskHandle_t finger_acce_task = NULL;
CMD_AUTH data_auth;
int TemplateCount = 0;

void modem_TTS(String str)
{
    play_mp3_dec(str);
}

void fingerID_ctl(bool onoff)
{
    fingerId_control = onoff;
}
bool finger_init()
{
    pinMode(PIN_FG_TOUCH, INPUT);
    // ESP_LOGD(TAG, "finger_init");

    // Serial.begin(57600, 134217756U, 20, 21, false, 20000UL, (uint8_t)112U);
    // Serial.begin(57600, 134217756U, 20, 21, false, 20000UL, (uint8_t)112U);
    Serial.flush();

    delay(100);

    if (finger.verifyPassword())
    {
        ESP_LOGI(TAG, "Found fingerprint sensor!");
        // return true;
    }
    else
    {
        ESP_LOGE(TAG, "Did not find fingerprint sensor :(");
        return false;
    }

    ESP_LOGD(TAG, "Reading sensor parameters");
    finger.getParameters();
    ESP_LOGD(TAG, "Status: %02x Sys ID: %02x Capacity:%d used:%d", finger.status_reg, finger.system_id, finger.capacity, finger.getTemplateCount());

    return true;
}
int getFingerUsed()
{
    return finger.getTemplateCount();
}
int getFingerprintID()
{
    // if (xSemaphoreTake(xSemaFINGER, portMAX_DELAY) == pdTRUE)
    if (digitalRead(PIN_FG_TOUCH))
    {

        uint8_t p = finger.getImage();
        switch (p)
        {
        case FINGERPRINT_OK:
            ESP_LOGD(TAG, "Image taken");
            // screen.SetBuzzer(5);
            break;
        default:
            ESP_LOGD(TAG, "getFingerprintID Unknown error");
            // xSemaphoreGive(xSemaFINGER);
            return -1;
        }

        // OK success!

        p = finger.image2Tz();
        switch (p)
        {
        case FINGERPRINT_OK:
            ESP_LOGD(TAG, "Image converted");
            break;
        default:
            ESP_LOGD(TAG, "Unknown error");
            // xSemaphoreGive(xSemaFINGER);
            return -1;
        }

        // OK converted!
        p = finger.fingerSearch();
        if (p == FINGERPRINT_OK)
        {
            ESP_LOGD(TAG, "Found a print match!");
        }
        else if (p == FINGERPRINT_PACKETRECIEVEERR)
        {
            ESP_LOGD(TAG, "Communication error");
            // xSemaphoreGive(xSemaFINGER);
            return -1;
        }
        else if (p == FINGERPRINT_NOTFOUND)
        {
            ESP_LOGD(TAG, "Did not find a match");
            // xSemaphoreGive(xSemaFINGER);
            return -1;
        }
        else
        {
            ESP_LOGD(TAG, "Unknown error");
            // xSemaphoreGive(xSemaFINGER);
            return -1;
        }

        // found a match!
        ESP_LOGD(TAG, "Found ID #%d with confidence of %d", finger.fingerID, finger.confidence);
        // xSemaphoreGive(xSemaFINGER);
        return finger.fingerID;
    }
    else
    {
    }
    // xSemaphoreGive(xSemaFINGER);
    return -1;
}

uint8_t getFingerprintEnroll()
{

    int p = -1;
    uint8_t id = finger.ReadIndexTable();
    ESP_LOGD(TAG, "Waiting for valid finger to enroll as #%d", id);
    unsigned long Enroll_time = millis();
    int simple_enroll = dev_cfg.getInt("simple_enroll", 0);
    // int time_enroll = simple_enroll ? 1000 * 5 : 1000 * 30;
    while (p != FINGERPRINT_OK)
    {
        if (millis() - Enroll_time > 1000 * 5)
        {
            return 0;
        }
        p = finger.getImage();
        switch (p)
        {
        case FINGERPRINT_OK:
            ESP_LOGD(TAG, "Image taken");
            break;
        case FINGERPRINT_NOFINGER:
            // ////blackboard.lcd_info.infopage_info = "请按住指纹";
            modem_TTS("/spiffs/press.mp3");
            ESP_LOGD(TAG, ".");
            break;
        case FINGERPRINT_PACKETRECIEVEERR:
            ESP_LOGD(TAG, "Communication error");
            break;
        case FINGERPRINT_IMAGEFAIL:
            ESP_LOGD(TAG, "Imaging error");
            break;
        default:
            ESP_LOGD(TAG, "Unknown error");
            break;
        }
    }

    // OK success!

    p = finger.image2Tz(1);
    switch (p)
    {
    case FINGERPRINT_OK:
        ESP_LOGD(TAG, "Image converted");
        break;
    default:
        ESP_LOGD(TAG, "Unknown error");
        return 0;
    }

    ESP_LOGD(TAG, "Remove finger");

    // delay(2000);
    p = 0;
    while (digitalRead(PIN_FG_TOUCH) && p != FINGERPRINT_NOFINGER)
    {
        p = finger.getImage();
        // ////blackboard.lcd_info.infopage_info = F("请移开手指");
        modem_TTS("/spiffs/remove.mp3");
    }

    ESP_LOGD(TAG, "ID %d", id);

    p = -1;
    ESP_LOGD(TAG, "Place same finger again");
    while (p != FINGERPRINT_OK)
    {
        p = finger.getImage();
        switch (p)
        {
        case FINGERPRINT_OK:
            ESP_LOGD(TAG, "Image taken");
            break;
        case FINGERPRINT_NOFINGER:
            ESP_LOGD(TAG, ".");
            modem_TTS("/spiffs/press.mp3");

            break;
        case FINGERPRINT_PACKETRECIEVEERR:
            ESP_LOGD(TAG, "Communication error");
            break;
        case FINGERPRINT_IMAGEFAIL:
            ESP_LOGD(TAG, "Imaging error");
            break;
        default:
            ESP_LOGD(TAG, "Unknown error");
            break;
        }
    }

    p = finger.image2Tz(2);
    switch (p)
    {
    case FINGERPRINT_OK:
        break;
    default:
        return 0;
        break;
    }

    // OK converted!

    ESP_LOGD(TAG, "Creating model %d", id);

    p = finger.createModel();
    if (p == FINGERPRINT_OK)
    {
        ESP_LOGD(TAG, "Prints matched!");
    }
    else
    {
        ESP_LOGD(TAG, "Unknown error");
        // FG_FLAG = false;
        return 0;
    }

    p = 0;
    while (digitalRead(PIN_FG_TOUCH) && p != FINGERPRINT_NOFINGER)
    {
        p = finger.getImage();
        modem_TTS("/spiffs/remove.mp3");
        // screen.SetTextValue(DS_INFO_PAGE, 1, F("请移开手指"));
        ////blackboard.lcd_info.infopage_info = F("请移开手指");
        delay(100);
    }

    int model_meage = 0, modle_ok = 0, modle_fail = 0;

    while (simple_enroll)
    {
        p = -1;
        ESP_LOGI(TAG, "+++Entering the detection phase+++");
        while (p != FINGERPRINT_OK)
        {
            p = finger.getImage();
            switch (p)
            {
            case FINGERPRINT_OK:
                ESP_LOGD(TAG, "Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                ESP_LOGD(TAG, ".");
                modem_TTS("/spiffs/press.mp3");
                ////blackboard.lcd_info.infopage_info = F("请再按一次相同指纹");
                delay(10);
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                ESP_LOGD(TAG, "Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                ESP_LOGD(TAG, "Imaging error");
                break;
            default:
                ESP_LOGD(TAG, "Unknown error");
                break;
            }
        }
        p = finger.image2Tz(2);
        switch (p)
        {
        case FINGERPRINT_OK:
            ESP_LOGD(TAG, "Image converted");
            break;
        default:
            ESP_LOGD(TAG, "Unknown error");
            break;
        }

        p = finger.PS_Match();
        if (p == FINGERPRINT_OK)
        {
            ESP_LOGI(TAG, "Match successful,confidence: %u", finger.confidence);
            double normalizedConfidence = (double)finger.confidence / 2.55;
            normalizedConfidence = normalizedConfidence > 100 ? 95 : normalizedConfidence;

            if (finger.confidence < 40)
            {
                ////blackboard.lcd_info.infopage_info = "指纹融合中....";
                ESP_LOGE(TAG, "Composite fingerprint.");
                finger.createModel();
            }
            else
            {
                modle_ok++;
                ////blackboard.lcd_info.infopage_info = "指纹验证通过:" + String(modle_ok) + "/2";
                ESP_LOGI(TAG, "No Composite fingerprint.");
            }

            delay(100);
        }
        else
        {
            ESP_LOGE(TAG, "Fingerprint mismatch.");
            ////blackboard.lcd_info.infopage_info = "指纹评分: 0/100 分";
            delay(100);
            modle_fail++;
        }
        p = 0;
        while (digitalRead(PIN_FG_TOUCH) && p != FINGERPRINT_NOFINGER)
        {
            p = finger.getImage();
            modem_TTS("/spiffs/remove.mp3");
            ////blackboard.lcd_info.infopage_info = F("请移开手指");

            delay(100);
        }
        if (model_meage > 3 || modle_ok > 1)
        {
            break;
        }
        if ((model_meage + modle_ok + modle_fail) > 6)
        {
            ESP_LOGE(TAG, "录入失败--");
            return 0;
        }

        /* code */
    }

    ESP_LOGD(TAG, "ID :%d", id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK)
    {
        ESP_LOGD(TAG, "Stored!");
    }
    else
    {
        ESP_LOGD(TAG, "Unknown error");
        // FG_FLAG = false;
        return 0;
    }
    // //FG_FLAG = false;
    return id;
}
String downloadFingerprintTemplate(uint16_t id)
{
    ESP_LOGD(TAG, "------------------------------------");
    ESP_LOGD(TAG, "Attempting to load # %d", id);
    uint8_t p = finger.loadModel(id);
    switch (p)
    {
    case FINGERPRINT_OK:
        ESP_LOGD(TAG, "Template %d loaded", id);
        break;
    case FINGERPRINT_PACKETRECIEVEERR:
        ESP_LOGD(TAG, "Communication error");
        return "";
    default:
        ESP_LOGD(TAG, "Unknown error ");
        return "";
    }

    // OK success!

    // one data packet is 267 bytes. in one data packet, 11 bytes are 'usesless' :D
    uint8_t bytesReceived[560]; // 2 data packets
    memset(bytesReceived, 0xff, 560);

    uint32_t starttime = millis();
    int i = 0;
    ESP_LOGD(TAG, "Attempting to get # %d", id);

    p = finger.getModel();
    switch (p)
    {
    case FINGERPRINT_OK:
        ESP_LOGD(TAG, "Transferring template  %d", id);
        break;
    default:
        ESP_LOGD(TAG, "Unknown error ");
        return "";
    }
    while (i < 556 && (millis() - starttime) < 20000)
    {
        // delay(10);
        if (Serial.available())
        {
            bytesReceived[i++] = Serial.read();
            // Serial.print(bytesReceived[i-1],HEX);
        }
    }

    ESP_LOGD(TAG, "%d bytes read. Decoding packet...", i);

    uint8_t fingerTemplate[512]; // the real template
    memset(fingerTemplate, 0xff, 512);

    // filtering only the data packets
    int uindx = 9, index = 0;
    memcpy(fingerTemplate + index, bytesReceived + uindx, 128); // first 256 bytes
    uindx += 128;                                               // skip data
    uindx += 2;                                                 // skip checksum
    uindx += 9;                                                 // skip next header
    index += 128;                                               // advance pointer
    memcpy(fingerTemplate + index, bytesReceived + uindx, 128); // second 256 bytes
    uindx += 128;                                               // skip data
    uindx += 2;                                                 // skip checksum
    uindx += 9;                                                 // skip next header
    index += 128;
    memcpy(fingerTemplate + index, bytesReceived + uindx, 128); // second 256 bytes
    uindx += 128;                                               // skip data
    uindx += 2;                                                 // skip checksum
    uindx += 9;                                                 // skip next header
    index += 128;
    memcpy(fingerTemplate + index, bytesReceived + uindx, 128); // second 256 bytes

    String finger_model = "";

    for (size_t i = 0; i < 512; ++i)
    {
        char hex[3];
        sprintf(hex, "%02X", fingerTemplate[i]);
        finger_model += hex;
    }

    return finger_model;
}

uint8_t *hexStringToBytes(const char *hexString, size_t *bytesLength)
{
    size_t length = strlen(hexString);
    *bytesLength = (length + 1) / 2;
    uint8_t *bytes = (uint8_t *)malloc(*bytesLength);
    for (size_t i = 0; i < *bytesLength; i++)
    {
        sscanf(hexString + i * 2, "%2hhx", &bytes[i]);
    }
    return bytes;
}

int uidToindex(uint64_t uid, uint64_t *uid2index, int len)
{
    for (int i = 0; i < 300; i++)
    {
        if (uid2index[i] == uid)
        {
            return i;
        }
    }
    return -1;
}
void auth_fg(const String &topic, void *param)
{
    CMD_AUTH *data = (CMD_AUTH *)param;
    if (data->auth_sc != AUTH_OTH)
    {
        ESP_LOGI(TAG, "授权来源是指纹");
        if (finger_acce_task != NULL)
        {
            if (xSemaphoreTake(xSemaFINGER, portMAX_DELAY) == pdTRUE)
            {
                // vTaskDelete(finger_acce_task);
                vTaskSuspend(finger_acce_task);
                xSemaphoreGive(xSemaFINGER);
                ESP_LOGI(TAG, "暂停指纹任务");
            }
        }
    }
    else if (data->auth_sc == AUTH_OTH)
    {
        ESP_LOGI(TAG, "重启指纹任务");
        finger_init();
        // data_fg_cnt.finger_count = finger.getTemplateCount();
        // broker.publish("up_authnum", &data_fg_cnt);

        vTaskResume(finger_acce_task);
    }
}

void finger_collect_dec(void *param)
{
    modem_TTS("/spiffs/enroll.mp3");
    uint8_t id = 0;
    CMD_ENROLL *info = (CMD_ENROLL *)(param);
    if (xSemaphoreTake(xSemaFINGER, 2000) == pdTRUE)
    {
        // blackboard.lcd_info.screen_index = DS_INFO_PAGE;
        // blackboard.lcd_info.infopage_title = info->driver_name;

        id = getFingerprintEnroll();
        ESP_LOGE(TAG, "Fingerprint module id:%d", id);
        if (id)
        {
            ////blackboard.lcd_info.infopage_info = "请放开手指";
            delay(1000);
            modem_TTS("/spiffs/enroll_ok.mp3");
            /* 获取特征值 */
            info->reply = downloadFingerprintTemplate(id);
            info->index = id;

            // DynamicJsonDocument doc_up(2048);
            // doc_up["finger_code"] = model;
            // doc_up["driver_num"] = info->driver_name;
            // // doc_up["finger_uid"] = rtc.getEpoch() + String(random(1000, 10000));
            // String jsonStr;
            // serializeJson(doc_up, jsonStr);
            // // CMD_MQTT_EVENT finger_enroll={.str=jsonStr ,.topic="upload_finger"};
            // static CMD_MQTT_EVENT data_fg_up;
            // data_fg_up.topic = "upload_finger";
            // data_fg_up.msg = jsonStr;
            // broker.publish("MQTT_EVENT", &data_fg_up);
            // blackboard.lcd_info.screen_index = DS_MAIN_PAGE;
        }
        else
        {
            modem_TTS("请换个手指再试一次");
            ////blackboard.lcd_info.infopage_info = F("请换个手指再试一次");
            delay(1000);
            // blackboard.lcd_info.screen_index = DS_MAIN_PAGE;
        }
        info->index = id;

        xSemaphoreGive(xSemaFINGER);
        // vTaskDelete(NULL);
    }
    else
    {

        modem_TTS("指纹采集异常");
        ESP_LOGE(TAG, "finger_collect_dec超时了");
        // vTaskDelete(NULL);
    }
    info->index = id;
}
void finger_collect(const String &topic, void *param)
{
    xTaskCreate(&finger_collect_dec,  // task
                "finger_collect_dec", // task name
                1024 * 5,             // stack size
                param,                // parameters
                18,                   // priority
                NULL                  // returned task handle
    );
}
// SemaphoreHandle_t xSemaFINGER = NULL; // 创建信号量Handler
void finger_auth(void *parm)
{
    while (true)
    {
        if (fingerId_control && xSemaphoreTake(xSemaFINGER, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {

            int id = getFingerprintID();

            if (id == -1)
            {
                delay(1500);
            }
            else
            {

                blackboard.face_auth_id = id;
                blackboard.car_sta.on_timestamp = esp32Time.getEpoch();
                ESP_LOGI(TAG, "finger.getID:%s", String(id).c_str());
                pinMode(10, OUTPUT);
                digitalWrite(10, HIGH);
                modem_TTS("/spiffs/unlock.mp3");
                finger.LEDcontrol(FINGERPRINT_LED_GRADUAL_ON, 100, FINGERPRINT_LED_BLUE, 0);

                // finger.LEDcontrol(false);
                // broker.publish("mp3_player", &mp3_face);
                // mp3_player_dec_d(&mp3_face);
                delay(3 * 1000);
                // close_ble();
                xSemaphoreGive(xSemaFINGER);
                vTaskDelete(NULL);
                // if (dev_cfg.getInt("safe_belt_lock", 0))
                // {
                //     static CMD_SB_LINE data_sb = {.sb_line_sta = -1, .sb_sc = -1};
                //     broker.publish("get_sb_sta", &data_sb);
                //     ESP_LOGE(TAG, "Fingerprint module id:%d", data_sb.sb_line_sta);

                //     modem_TTS("请系好安全带再开锁");
                //     // blackboard.lcd_info.info_bar = "请系好安全带再开锁";
                //     if (blackboard.sys_parm.sys_SB_TYPE == data_sb.sb_line_sta)
                //         continue;
                // }
                // if (fg_cfg_local.isKey(String(res).c_str()))
                // {
                //     // blackboard.lcd_info.info_bar = blackboard.sys_parm.cmp_name;
                //     DynamicJsonDocument json_finger_item(512);
                //     ESP_LOGI(TAG, "finger dect sucess index:%d", res);

                //     delay(1000);
                // }
                // xSemaphoreGive(xSemaFINGER);
            }
            xSemaphoreGive(xSemaFINGER);
            delay(100);
        }
        else
        {
            delay(1000);
        }
        if (blackboard.car_sta.deviceStatus == DEV_AUTH)
        {
            ESP_LOGD(TAG, "fnger 授权自动关闭");
            // finger.LEDcontrol()
            finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 255, FINGERPRINT_LED_BLUE, 1);
            vTaskDelete(NULL);
        }
        delay(100);
    }
}
void up_authnum(const String &topic, void *param)
{
    CMD_FG_CNT *data_fg_cnt = (CMD_FG_CNT *)param;
    data_fg_cnt->finger_count = TemplateCount;
    ESP_LOGD(TAG, "指纹数量上报:%d", data_fg_cnt->finger_count);
}
int fg_delete_one(uint16_t id)
{
    if (id == 0xffff)
    {
        return finger.emptyDatabase();
    }
    return finger.deleteModel(id);
}
void task_finger(void *parm)
{
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    ESP_LOGD(TAG, "task_finger");
    int fg_conut = 0;
    while (fg_conut < 5)
    {
        if (finger_init())
        {
            break;
        }
        fg_conut++;
    }

    if (finger_init())
    {

        TemplateCount = finger.getTemplateCount();
        // blackboard.err_code |= (1 << ERR_BIT_FG);
        xSemaFINGER = xSemaphoreCreateBinary();
        xSemaphoreGive(xSemaFINGER);
        // broker.subscribe("hal_sensor_cl", finger_collect);
        // broker.subscribe("auth", auth_fg);
        // broker.subscribe("up_authnum", up_authnum);
        xTaskCreatePinnedToCore(&finger_auth,      // task
                                "finger_auth",     // task name
                                1024 * 5,          // stack size
                                NULL,              // parameters
                                15,                 // priority
                                &finger_acce_task, // returned task handle
                                0                  // pinned core
        );
    }
    else
    {
        modem_TTS("/spiffs/fail.mp3");
        ESP_LOGE(TAG, "ERR finger");
    }

    delay(1000);
    vTaskDelete(NULL);
}