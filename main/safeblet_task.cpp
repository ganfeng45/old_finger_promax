#include "tasks.h"
#include "blackboard.h"
#include "blackboard_c.h"
#include "DEV_PIN.h"
#include "myutil.h"

static const char *TAG = "Sb";
char *handle_arg = "/spiffs/on.mp3";

//
String ERR_BIT_SB_TIME = "0000";
typedef enum
{
    SB_OFF = 0,
    SB_ON = 1
} sb_sta_t;

void task_sb_entry(void *params)
{

    pinMode(PIN_SB, INPUT);
    ESP_LOGE(TAG, "task_sb_entry___xTaskNotifyWait"); // (String) returns time with specified format

    int count = 0;
    delay(3 * 1000);
    int sb_count = 0;
    int sys_warn_onoff = dev_cfg.getInt("warn_onoff", 1);
    sb_sta_t sys_sb_sta = SB_OFF;
    while (1)
    {
        // ESP_LOGE(__func__, "安全带--%d type: %d", digitalRead(PIN_SB), sys_SB_TYPE); // (String) returns time with specified format
        // int waterMark = uxTaskGetStackHighWaterMark(blackboard.system.sbTaskHandle);
        // ESP_LOGE(TAG, "Task  Used Memory: %d Bytes", waterMark);
        // ESP_LOGE(TAG, "Task Free Memory: %d Bytes", 4096 - waterMark);
        if (digitalRead(PIN_SB) != sys_SB_TYPE)
        {
            // ESP_LOGE(__func__, "安全带--SB_OFF %d ",blackboard.car_sta.deviceStatus); // (String) returns time with specified format
            sys_sb_sta = SB_OFF;
        }
        else
        {
            // ESP_LOGE(__func__, "安全带--SB_ON"); // (String) returns time with specified format

            sys_sb_sta = SB_ON;
        }
        // if (digitalRead(PIN_SB) && (blackboard.car_sta.deviceStatus == DEV_AUTH))
        if (sys_sb_sta && (blackboard.car_sta.deviceStatus == DEV_AUTH))
        {
            count++;
            ESP_LOGE(__func__, "安全带++ %d", count); // (String) returns time with specified format
        }
        else
        {
            count = 0;
        }

        /* 设置安全带 */
        // if (count > getJsonValue<int>(doc, "safe_belt_tm", 30) && getJsonValue<String>(doc_work, "driver_num", "0000") == "0000")
        if (count > sys_SB_LIMIT_TIME)
        {
            blackboard.err_code |= (1 << ERR_BIT_SB);
        }
        else
        {
            // digitalWrite(ALARM_CON,LOW);
            blackboard.err_code &= ~(1 << ERR_BIT_SB);
        }
        /* 低电压00 */
        // if (getJsonValue<String>(doc, "ERR_BIT_ADC", "0000") == "0000" && blackboard.err_code & (1 << ERR_BIT_ADC))
        // {
        //     sb_count=0;
        //     ESP_LOGE(__func__, "低电压"); // (String) returns time with specified format
        //     doc["ERR_BIT_ADC"] = ali_time();
        //     DynamicJsonDocument doc_estart(256);
        //     doc_estart["code"] = 1001;
        //     doc_estart["startTime"] = ali_time();
        //     doc_estart["additional_info"] = "low vvv";
        //     doc_estart["driver_num"] = getJsonValue<String>(doc_work, "driver_num", "007");
        //     String jsonStr;modem_TTS
        //     serializeJson(doc_estart, jsonStr);
        //     iot.sendEvent("event_code_start", jsonStr.c_str());
        // }

        // screen.SetTextValue(DS_MAIN_PAGE, 9, String((blackboard.isr_count) / doc["SpeedSensorPulses"].as<int>()));updata_infobar
        /* 报警灯检查 */
        // if (blackboard.err_code & (1 << ERR_BIT_SB) || blackboard.err_code & (1 << ERR_BIT_SD) || blackboard.err_code & (1 << ERR_BIT_ADC))
        if ((blackboard.err_code & (1 << ERR_BIT_SB) || blackboard.err_code & (1 << ERR_BIT_SD)) && (sys_warn_onoff))
        {
            ESP_LOGE(__func__, "报警灯"); // (String) returns time with specified format
            // digitalWrite(ALARM_CON, HIGH);
            if (blackboard.err_code & (1 << ERR_BIT_SB))
            {
                // modem_TTS("安全带报警");
                  handle_arg = "/spiffs/sb.mp3";
                    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                    delay(1200);
                    digitalWrite(audio_ctrl,LOW);
                    delay(2000);
            }
            
        }
        else
        {
            // digitalWrite(ALARM_CON, LOW);
        }
#ifdef CAT1
        /* 安全带 */
        if (ERR_BIT_SB_TIME == "0000" && blackboard.err_code & (1 << ERR_BIT_SB))
        {
            ESP_LOGE(__func__, "发送"); // (String) returns time with specified format
            // updata_infobar("安全带报警");

            ERR_BIT_SB_TIME = ali_time();
            DynamicJsonDocument doc_estart(256);
            doc_estart["code"] = 1108;
            doc_estart["startTime"] = ali_time();
            doc_estart["additional_info"] = "安全带报警";
            doc_estart["driver_num"] = work_rec.getString("driver_num", "0000");
            String jsonStr;
            serializeJson(doc_estart, jsonStr);
            iot.sendEvent("event_code_start", jsonStr.c_str());
        }
        if (ERR_BIT_SB_TIME != "0000" && !(blackboard.err_code & (1 << ERR_BIT_SB)))
        {
            // //updata_infobar("待机");
            // updata_infobar(dev_cfg.getString("cmp_name", "朗云科技"));

            DynamicJsonDocument doc_estart(256);
            doc_estart["startTime"] = ERR_BIT_SB_TIME;
            doc_estart["endTime"] = ali_time();
            doc_estart["code"] = 1108;
            doc_estart["additional_info"] = "注意不一定是此驾驶员违规";
            doc_estart["event_value"] = " ";
            doc_estart["driver_num"] = work_rec.getString("driver_num", "0000");
            String jsonStr;
            serializeJson(doc_estart, jsonStr);
            iot.sendEvent("event_code_stop", jsonStr.c_str());
            ERR_BIT_SB_TIME = "0000";
            // saveConfiguration(SYS_CFG_PATH, doc);
        }
#endif
        delay(1000);
    }
}


