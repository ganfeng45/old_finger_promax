#include "tasks.h"
#include "blackboard.h"
#include "blackboard_c.h"
#include "DEV_PIN.h"
#include "myutil.h"
#include<stdio.h>
#include "SPIFFS.h"
#include <Update.h>
#include "mbedtls/md5.h" // MD5库
// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>
// #include <BLE2902.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "blackboard.h"
// #define SPIFFS LittleFS
StaticJsonDocument<1024> doc_ble;
StaticJsonDocument<1024> doc_replay;
StaticJsonDocument<512> doc_iic;
char MACstr[20];
#define version "V2.1.0"
#define TAG __func__
// #define Version  V1.0.0

struct RFID_Info
{
    String driver_name;
    String phone;
    String id;
    int number;
    bool macIc;
};
RFID_Info rfid_info;
/*
特征值
0-1：版本号
2：wifi
3:4g
4:gps
6:授权状态
7：保留
8-16：速度
行驶里程
 */
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
SemaphoreHandle_t xSemaCMD = NULL; // 创建信号量Handler

uint32_t value = 0;
uint16_t speed = 0; // 3700 mV
BLEScan *pBLEScan;
uint8_t payLoad[32];
uint32_t rec_key[2048];
int log_count = 100;

int con_count = 0;
String ble_mac;
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// MovingAverageFilter filter_spd_ble(3);
int wheel_diameter = 1;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define SERVICE_UUID_SSP "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        con_count++;
        Serial.println("onConnect");
        deviceConnected = true;
        BLEDevice::startAdvertising();
        finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, 0x03,0);        //蓝牙连接成功指纹灯变为黄色
        char *handle_arg = "/spiffs/link.mp3";
        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        delay(2000);
        digitalWrite(audio_ctrl,LOW);
    };

    void onDisconnect(BLEServer *pServer)
    {
        con_count--;
        Serial.println("onDisconnect");
        deviceConnected = false;
        BLEDevice::startAdvertising();
    }
};
int test_timer = 0;

void get_number(String driverID)
{
    rfid_info.number=1;
    String mystring = driverID.substring(12);
    const char *namespace_name = "rfid_cfg";
    nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
    Preferences all_prt;
    all_prt.begin(namespace_name, true);
    // work_rec.clear();
    // ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
    if (it == NULL)
    {
        ESP_LOGI(TAG, "%s:no KEY_VALUE", namespace_name);
    }
    int index = 0;
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        rfid_info.number++;
        uint32_t convertedKey = strtoul(info.key, NULL, 10);
        rec_key[index++] = convertedKey;
        if(mystring==info.key)
        {
            rfid_info.number=0;
            ESP_LOGI(TAG, "%s: KEY_VALUE", info.key);
            char *handle_arg = "/spiffs/repeatIC.mp3";
            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(1500);
            digitalWrite(audio_ctrl,LOW);
            break;
        }

    }
}

bool verify_fgnum(String driverid)
{
    JsonArray list = doc_replay.createNestedArray("list");
    // list.add("1#张三");
    // list.add("2#李四");
    int now_conut = 0;
    bool verify_fgnum=true;
    // return;
    const char *namespace_name = "finger_cfg";
    nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
    Preferences all_prt;
    all_prt.begin(namespace_name, true);
    // ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
    if (it == NULL)
    {
        ESP_LOGI(TAG, "%s:no KEY_VALUE", namespace_name);
    }
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        Serial.printf("key '%s', type '%d'\n", info.key, info.type);
        if (info.type == NVS_TYPE_STR)
        {
            Serial.println(all_prt.getString(info.key));
            // rfid_cfg_local.remove(info.key);
            if((all_prt.getString(info.key)).indexOf(driverid)!=0)
            {
                now_conut++;
            }
            if(dev_cfg.getInt("fg_num",100)<=now_conut)
               {
                verify_fgnum=false;
                return verify_fgnum;
                // break;
               }
        }

    }
     Serial.println(verify_fgnum);
    return verify_fgnum;
}
void enroll_save(String driverName, String driverID, String driverphone)
{
    if (xSemaphoreTake(xSemaFINGER, portMAX_DELAY) == pdTRUE)
    {

        vTaskSuspend(blackboard.system.fingerTaskHandle);            //挂起指纹刷卡
        // uint8_t finger_id=0;
        ESP_LOGE(TAG, "获取到指纹句柄");
        String replay_str;
        // int time_out=0;
        char *handle_arg = "/spiffs/enrool.mp3";
          esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
          delay(2000);
          digitalWrite(audio_ctrl,LOW);
        // while(!(digitalRead(FG_TOUCH)))
        // {
        //     char *handle_arg = "/spiffs/press.mp3";;
        //   esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        //   delay(2000);
        //   if(time_out++>10) break;
        // }
         uint8_t finger_id = getFingerprintEnroll();
        if (finger_id)
        {
            // if(finger_id<(log_count+1))
            String Finger_Data = driverName + "#" + driverID + "#" + driverphone;
            fg_cfg_local.putString(String(finger_id).c_str(), Finger_Data.c_str());
            doc_replay.clear();
            doc_replay["code"] = 200;
            doc_replay["cmd"] = "addFingerUser";
            doc_replay["name"] = driverName;
            doc_replay["index"] = finger_id;
            doc_replay["id"] = driverID;
            doc_replay["phone"] = driverphone;
            char *handle_arg = "/spiffs/over.mp3";
            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(1200);
            digitalWrite(audio_ctrl,LOW);
            // serializeJson(doc_replay, replay_str);
            // ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());

            // pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
            // pTxCharacteristic->notify();

            // {
            //     "code" : 200, "cmd" : "addUser", "name" : "张三", "index" : 1
            // }
        }
        else 
        {
            // if(finger_id>log_count)  finger.deleteModel(finger_id);
            doc_replay.clear();
            doc_replay["code"] = 404;
            doc_replay["cmd"] = "addFingerUser";
            doc_replay["msg"] = "录入失败";
            if (!finger_init())
               { 
                   ESP_LOGE(TAG, "Failed to finger_init :(");
                    vTaskDelete(NULL);
              }
            else
              {
                      blackboard.err_code |= (1 << ERR_BIT_FG);
                     }
            //  delay(500);
            char *handle_arg = "/spiffs/noOK.mp3";
            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(1200);
            digitalWrite(audio_ctrl,LOW);
        }
        serializeJson(doc_replay, replay_str);
        pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
        pTxCharacteristic->notify();
        vTaskResume(blackboard.system.fingerTaskHandle);
        delay(5000);
        xSemaphoreGive(xSemaFINGER);
    }
}

void get_usrlist(int pn, int ps)
{
    // printAllNVSKeys("finger_cfg");
    doc_replay.clear();
    String replay_str;
    doc_replay["code"] = 200;
    doc_replay["cmd"] = "getFingerUser";
    doc_replay["pN"] = pn;

    JsonArray list = doc_replay.createNestedArray("list");
    // list.add("1#张三");
    // list.add("2#李四");
    int now_conut = 0;
    // return;
    const char *namespace_name = "finger_cfg";
    nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
    Preferences all_prt;
    all_prt.begin(namespace_name, true);
    // ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
    if (it == NULL)
    {
        ESP_LOGI(TAG, "%s:no KEY_VALUE", namespace_name);
    }
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        Serial.printf("key '%s', type '%d'\n", info.key, info.type);
        if (info.type == NVS_TYPE_STR)
        {
            Serial.println(all_prt.getString(info.key));
            // fg_cfg_local.remove(info.key);
            now_conut++;
            if (now_conut > (pn - 1) * ps && now_conut < (pn)*ps + 1)
            {
                list.add(String(info.key) + "#" + (all_prt.getString(info.key)));
                ESP_LOGI(TAG, "index: %d 列表：%s", now_conut, (String(all_prt.getInt(info.key)) + "#" + String(info.key)).c_str());
            }
        }
        if (now_conut > (pn)*ps + 1)
        {
            break;
        }
    }
    serializeJson(doc_replay, replay_str);
    ESP_LOGI(TAG, "回复消息：%s", replay_str.c_str());

    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
    pTxCharacteristic->notify();
}
void get_rfidlist(int pn, int ps)
{
    // printAllNVSKeys("finger_cfg");
    doc_replay.clear();
    String replay_str;
    doc_replay["code"] = 200;
    doc_replay["cmd"] = "getICUser";
    doc_replay["pN"] = pn;

    JsonArray list = doc_replay.createNestedArray("list");
    // list.add("1#张三");
    // list.add("2#李四");
    int now_conut = 0;
    // return;
    const char *namespace_name = "rfid_cfg";
    nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
    Preferences all_prt;
    all_prt.begin(namespace_name, true);
    // ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
    if (it == NULL)
    {
        ESP_LOGI(TAG, "%s:no KEY_VALUE", namespace_name);
    }
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        Serial.printf("key '%s', type '%d'\n", info.key, info.type);
        if (info.type == NVS_TYPE_STR)
        {
            Serial.println(all_prt.getString(info.key));
            // rfid_cfg_local.remove(info.key);
            now_conut++;
            if (now_conut > (pn - 1) * ps && now_conut < (pn)*ps + 1)
            {
                int num = now_conut + (pn - 1) * ps;

                list.add(String(now_conut) + "#" + (all_prt.getString(info.key)));
                ESP_LOGI(TAG, "index: %d 列表：%s", now_conut, (String(now_conut) + "#" + String(info.key)).c_str());
            }
        }
        if ((now_conut > (pn)*ps + 1) || now_conut >= log_count)
        {
            all_prt.remove(info.key);
            break;
        }
    }
    serializeJson(doc_replay, replay_str);
    ESP_LOGI(TAG, "回复消息：%s", replay_str.c_str());

    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
    pTxCharacteristic->notify();
}
void get_worklist(int pn, int ps)
{
    // printAllNVSKeys("finger_cfg");
    doc_replay.clear();
    // String rec_key[10];
    String replay_str;
    doc_replay["code"] = 200;
    doc_replay["cmd"] = "getWork";
    doc_replay["pN"] = pn;

    JsonArray list = doc_replay.createNestedArray("list");
    // list.add("1#张三");
    // list.add("2#李四");
    int now_conut = 0;
    // return;
    const char *namespace_name = "work_rec";
    nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
    Preferences all_prt;
    all_prt.begin(namespace_name, true);
    // work_rec.clear();
    // ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
    if (it == NULL)
    {
        ESP_LOGI(TAG, "%s:no KEY_VALUE", namespace_name);
    }
    int index = 0;
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        uint32_t convertedKey = strtoul(info.key, NULL, 10);
        // printf("key '%s', type '%d'  转为数据：%u \n", info.key, info.type, convertedKey);
        rec_key[index++] = convertedKey;

        /* if (0)
        {
            if (info.type == NVS_TYPE_STR)
            {
                // Serial.println(all_prt.getString(info.key));
                now_conut++;
                if (now_conut > (pn - 1) * ps && now_conut < (pn)*ps + 1)
                {
                    list.add(String(info.key) + "#" + (all_prt.getString(info.key)));
                    ESP_LOGI(TAG, "index: %d 列表：%s", now_conut, (String(all_prt.getInt(info.key)) + "#" + String(info.key)).c_str());
                }
            }
            if (now_conut > (pn)*ps + 1)
            {
                break;
            }
        }
        else
        {
            // if (info.type == NVS_TYPE_STR)
            // {
            //     ESP_LOGI(TAG, "index: %s ", info.key);

            //     rec_key[index++] = uint32_t(info.key);
            // }
        } */
    }
    // int work__send = 0;
    // int work_list_count = index;
    int start_index = index - (pn - 1) * ps;
    for (int i = 1; i < ps + 1; i++)
    {
        if (start_index - i < 0)
        {
            break;
        }
        String dname = work_rec.getString(String(rec_key[start_index - i]).c_str(), "ERR");
        ESP_LOGI(TAG, "rec_key[%d]: %u ", start_index - i, rec_key[start_index - i]);
        if (dname != "ERR")
        {

            list.add(String(rec_key[start_index - i]) + "#" + dname);
        }

        // work_list_count
    }
    // for(int i=0;i<1024;i++)
    //     {
    //         ESP_LOGI(TAG, "rec_key[%d]: %u ", index, rec_key[i]);
    //     }
    if (index > log_count)
    {
        for (int i = 0; i < index - log_count; i++)
        {
            work_rec.remove(String(rec_key[i]).c_str());
        }
    }

    // while (index--)
    // {

    //     ESP_LOGI(TAG, "rec_key[%d]: %u ", index, rec_key[index]);
    // }
    ESP_LOGI(TAG, "回复消息：%s", replay_str.c_str());
  if(deviceConnected)
  {
        serializeJson(doc_replay, replay_str);

    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
    pTxCharacteristic->notify();
  }
}
void delete_save(String list_str)
{
    if (xSemaphoreTake(xSemaFINGER, portMAX_DELAY) == pdTRUE)
    {
        ESP_LOGE(TAG, "获取到指纹句柄");
        // {"code":200,"cmd":"deleteUser","index":"1,2,3"}
        doc_replay.clear();
        String delete_index = "";
        String replay_str;
        doc_replay["code"] = 200;
        doc_replay["cmd"] = "deleteFingerUser";
        // doc_replay["index"] = "";
        int numbers[300];
        int count = splitStringToInt(list_str, ',', numbers);
        for (int i = 0; i < count; i++)
        {

            if (numbers[i])
            {

                if (!finger.deleteModel(numbers[i]) && fg_cfg_local.getString(String(numbers[i]).c_str(), "0") != "0")
                {
                    // ESP_LOGI(TAG, "删旧指纹 id:%d 姓名:%s", numbers[i], fg_cfg_local.getString( String(numbers[i]).c_str()));
                    fg_cfg_local.remove(String(numbers[i]).c_str());
                    delete_index += String(numbers[i]) + ",";
                }
            }

            // ESP_LOGE(TAG, "删旧指纹%d: %s", numbers[i], finger_temp.getString(String(numbers[i]).c_str(), "0").c_str());
        }
        if (!delete_index.isEmpty())
        {
            delete_index = delete_index.substring(0, delete_index.length() - 1);
            doc_replay["index"] = delete_index;
        }
        // doc_replay["index"] = delete_index;
        serializeJson(doc_replay, replay_str);

        ESP_LOGI(TAG, "回复消息：%s", replay_str.c_str());

        pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
        pTxCharacteristic->notify();

        finger_temp.clear();
        xSemaphoreGive(xSemaFINGER);
    }
}

void RFdelete_save(String list_str)
{
    if (xSemaphoreTake(xSemaRFID, portMAX_DELAY) == pdTRUE)
    {
        ESP_LOGE(TAG, "获取钥匙卡句柄");
        // {"code":200,"cmd":"deleteUser","index":"1,2,3"}
        doc_replay.clear();
        String delete_index = "";
        String replay_str;
        doc_replay["code"] = 200;
        doc_replay["cmd"] = "deleteICUser";
        int numbers[300];
        int count = splitStringToInt(list_str, ',', numbers);
        // Stringsplit(list_str,',',count);
        int now_conut = 0;
        // return;
        const char *namespace_name = "rfid_cfg";
        nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
        Preferences all_prt;
        all_prt.begin(namespace_name, true);
        // ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
        if (it == NULL)
        {
            Serial.printf("%s:no KEY_VALUE", namespace_name);
        }
        while (it != NULL)
        {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);
            it = nvs_entry_next(it);
            Serial.printf("key '%s', type '%d'\n", info.key, info.type);
            if (info.type == NVS_TYPE_STR)
            {
                Serial.println(all_prt.getString(info.key));
                // rfid_cfg_local.remove(info.key);
                now_conut++;
                for (int i = 0; i < count; i++)
                {
                    if (now_conut == numbers[i])
                    {

                        // if (rfid_cfg_local.getString(String(numbers[i]).c_str(), "0") != "0")
                        {
                            ESP_LOGI(TAG, "删旧指纹 id:%d 姓名:%s", numbers[i], String(info.key).c_str());
                            rfid_cfg_local.remove(info.key);
                            delete_index += String(numbers[i]) + ",";
                        }
                    }
                }
            }
        }
        if (!delete_index.isEmpty())
        {
            delete_index = delete_index.substring(0, delete_index.length() - 1);
            doc_replay["index"] = delete_index;
        }
        // doc_replay["index"] = delete_index;
        serializeJson(doc_replay, replay_str);

        ESP_LOGI(TAG, "回复消息：%s", replay_str.c_str());

        pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
        pTxCharacteristic->notify();

        finger_temp.clear();
        xSemaphoreGive(xSemaRFID);
    }
}
void empty_all()
{
    if (xSemaphoreTake(xSemaFINGER, portMAX_DELAY) == pdTRUE)
    {
        ESP_LOGE(TAG, "获取到指纹句柄");
        doc_replay.clear();
        ESP_LOGE(TAG, "empty finger.emptyDatabase res:%d", finger.emptyDatabase());
        if (finger.getTemplateCount() == 0)
        {
            fg_cfg_local.clear();
            rfid_cfg_local.clear(); // 清卡
            doc_replay["code"] = 200;
        }
        else
        {
            doc_replay["code"] = 404;
            doc_replay["msg"]  ="清空失败";
        }
        // {"code":200,"cmd":"clearUser"}/*  */
        doc_replay["cmd"] = "clearUser";
        String replay_str;
        serializeJson(doc_replay, replay_str);
        ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());

        pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
        pTxCharacteristic->notify();

        xSemaphoreGive(xSemaFINGER);
    }
}
void get_Version(void)
{
    doc_replay.clear();
    doc_replay["cmd"] = "getVersion";
    doc_replay["code"] = 200;
    doc_replay["version"] = version;
    String replay_str;
    serializeJson(doc_replay, replay_str);
    ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());

    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
    pTxCharacteristic->notify();
}
void IC_save(String driverName, String driverID, String driverphone)
{
    uuid="";
    // char *handle_arg = "/spiffs/fangIC.mp3";
    //              esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
    if (xSemaphoreTake(xSemaRFID, portMAX_DELAY) == pdTRUE)
    {
        // char *handle_arg = "/spiffs/noIC.mp3";
        // esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        // DynamicJsonDocument *pDoc = (DynamicJsonDocument *)params;
        // serializeJsonPretty((*pDoc), Serial);
        bool witre_flag = true;
        String replay_str;
        if (1)
        {
            rc522_init();
            char *handle_arg = "/spiffs/fangIC.mp3";
            while (!mfrc522.PICC_IsNewCardPresent())
            {
                // 语言播报请放上卡片
                esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                delay(1500);
                digitalWrite(audio_ctrl,LOW);
                delay(100);
            }
            // Verify if the NUID has been readed
            if (!mfrc522.PICC_ReadCardSerial())
            {
                rc522_init();
                xSemaphoreGive(xSemaRFID);
            }
            Serial.println(F("A new card has been detected."));

            // Store NUID into nuidPICC array
            for (byte i = 0; i < 4; i++)
            {
                nuidPICC[i] = mfrc522.uid.uidByte[i];
            }

            Serial.println(F("The NUID tag is:"));
            Serial.print(F("In hex: "));
            //  Serial.println(mfrc522.uid.uidByte);
            // printHex((reinterpret_cast<int>(mfrc522.uid.uidByte)), mfrc522.uid.size);
            printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println();
            Serial.print(F("In dec: "));
            printDec(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println();
            Serial.print(F("PICC type: "));
            MFRC522::PICC_Type piccType = (MFRC522::PICC_Type)mfrc522.PICC_GetType(mfrc522.uid.sak);
            Serial.println(mfrc522.PICC_GetTypeName(piccType));

            byte status;
            // Authenticate using key A.
            // avvio l'autentificazione A
            // Serial.println("Authenticating using key A...");
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, RC_DRIVER_NAME, &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK)
            {
                Serial.print("PCD_Authenticate() failed: ");
                Serial.println(mfrc522.GetStatusCodeName(status));
                rc522_init();
                xSemaphoreGive(xSemaRFID);
            }
            else
            {
                Serial.print("PCD_Authenticate() AAAA ok: ");
            }
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, RC_DRIVER_NAME, &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK)
            {
                Serial.print("PCD_Authenticate() failed: ");
                Serial.println(mfrc522.GetStatusCodeName(status));
                rc522_init();
                xSemaphoreGive(xSemaRFID);

                // return;
            }
            else
            {
                Serial.print("PCD_Authenticate() BBBB ok: ");
            }

            uint8_t buffer[20];
            memset(buffer, 0, sizeof(buffer));
            strncpy((char *)buffer, driverName.c_str(), 16);

            status = mfrc522.MIFARE_Write(RC_DRIVER_NAME, buffer, 16);
            //  MFRC522::StatusCode status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
            if (status == MFRC522::STATUS_OK)
            {
                Serial.println(" driver_name Write success");
                Serial.println(driverName.length());
                Serial.println(driverID.c_str());
                Serial.println(driverphone.c_str());
            }
            else
            {
                Serial.println(" driver_name Write failed");
                witre_flag = false;
            }
            handle_arg= "/spiffs/di.mp3";

            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            if (driverName != MACstr) // 非设备卡
            {
                if (1)
                {
                    // String key_driver_id = md5str(driverID,18);
                    String key_driver_id = driverID;
                    ESP_LOGI(TAG, "driver: %s key_driver_num txt:%s length：%d", rfid_info.id.c_str(), key_driver_id.c_str(), key_driver_id.length());

                    strncpy((char *)buffer, key_driver_id.c_str(), 16);
                    status = mfrc522.MIFARE_Write(RC_DRIVER_NUM, buffer, 16);
                    if (status == MFRC522::STATUS_OK)
                    {
                        Serial.println("driver_num1 Write success:%s");
                    }
                    else
                    {
                        witre_flag = false;
                        Serial.println("driver_num1 Write failed");
                    }

                    String last_14_digits = key_driver_id.substring(key_driver_id.length() - 2);
                    ESP_LOGI(TAG, "key_driver_num 长度：%d", key_driver_id.length());
                    ESP_LOGI(TAG, "key_driver_num 截取后：%s", last_14_digits.c_str());
                    rfid_info.id = last_14_digits;
                }
                if (1)
                {
                    // String phone_num_str = md5str(driverphone, 16);
                    String phone_num_str = rfid_info.id + driverphone; // 将剩余ID与电话号码存在一个地方。
                    ESP_LOGI(TAG, "cmp：%s company_num_str txt:%s len：%d", rfid_info.id.c_str(), phone_num_str.c_str(), phone_num_str.length());
                    // memset(buffer, 0, 20);
                    strncpy((char *)buffer, phone_num_str.c_str(), 16);
                    status = mfrc522.MIFARE_Write(RC_CM_NUM, buffer, 16);
                    //  MFRC522::StatusCode status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
                    if (status == MFRC522::STATUS_OK)
                    {
                        //   screen.SetTextValue(DS_MAIN_PAGE, INFO_BAR, "CMP:" + getJsonValue<String>(*pDoc, "company_num", "(error)"));
                        Serial.println("phone_num Write success");
                    }
                    else
                    {
                        witre_flag = false;
                        Serial.println("phone_num Write failed");
                    }
                }
                // String company_num_str = getJsonValue<String>(*pDoc, "company_num", "0000");
                if (witre_flag) // 非设备卡成功回复
                {

                    //  Serial.println("Number of entries in 'my-app' namespace: " + String(entryCount));

                    String doc2str_iic;
                    // serializeJson(doc_iic, doc2str_iic);
                    doc2str_iic = driverName + "#" + driverID + "#" + driverphone+"#"+uuid;
                    // rfid_cfg_local.begin("NRFC522");
                    String mystring = driverID.substring(12);
                    rfid_cfg_local.putString(mystring.c_str(), doc2str_iic);
                    if (rfid_cfg_local.isKey(mystring.c_str()))
                    {
                        Serial.println(mystring.c_str());
                    }
                    Serial.println(doc2str_iic);
                    Serial.println(mystring.c_str());   
                    doc_replay.clear();
                    doc_replay["code"] = 200;
                    doc_replay["cmd"] = "writeUser";
                    doc_replay["name"] = driverName.c_str();
                    doc_replay["index"] = rfid_info.number;
                    doc_replay["id"] = driverID.c_str();
                    doc_replay["phone"] = driverphone.c_str();
                    doc_replay["card"] = uuid.c_str();
                    handle_arg = "/spiffs/writerIC.mp3";    
                    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                    delay(1200);
                    digitalWrite(audio_ctrl,LOW);
                }
                else
                {
                    doc_replay.clear();
                    doc_replay["code"] = 404;
                    doc_replay["cmd"] = "writeUser";
                    doc_replay["msg"] = "写卡失败";
                    handle_arg = "/spiffs/noIC.mp3";
                    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                    delay(1200);
                    digitalWrite(audio_ctrl,LOW);
                }
            }
            else if (witre_flag)
            {
                doc_replay.clear();
                // rfid_cfg_local.putString("writeTerminal", driverName.c_str());
                doc_replay["code"] = 200;
                doc_replay["cmd"] = "writeTerminal";
                handle_arg = "/spiffs/writerIC.mp3";
                esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                delay(1200);
                digitalWrite(audio_ctrl,LOW);
            }
            else
            {
                doc_replay.clear();
                doc_replay["code"] = 404;
                doc_replay["cmd"] = "writeTerminal";
                doc_replay["msg"] = "写卡失败";
                handle_arg = "/spiffs/noIC.mp3";
                esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                delay(1200);
                digitalWrite(audio_ctrl,LOW);
                // 播报写卡失败
            }
        }


        serializeJson(doc_replay, replay_str);
        ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());

        pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
        pTxCharacteristic->notify();
        xSemaphoreGive(xSemaRFID);
        rc522_init();
        delay(2000);
        // vTaskDelete(NULL);
    }
}

int authorization_writer(String phone)
{

    // 语言播报请放上卡片
    if (xSemaphoreTake(xSemaRFID, portMAX_DELAY) == pdTRUE)
    {
        uuid="";
        Serial.print(F("rc522_read......"));
        while (!mfrc522.PICC_IsNewCardPresent())
        {
             char *handle_arg = "/spiffs/fangIC.mp3";
             esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(1500);
        digitalWrite(audio_ctrl,LOW);
            delay(100);
        }

        // Verify if the NUID has been readed
        if (!mfrc522.PICC_ReadCardSerial())
        {
            xSemaphoreGive(xSemaRFID);
            return -1;
        }
        Serial.print(F("PICC type: "));
        MFRC522::PICC_Type piccType = (MFRC522::PICC_Type)mfrc522.PICC_GetType(mfrc522.uid.sak);
        Serial.println(mfrc522.PICC_GetTypeName(piccType));
        /* dump */
        // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
        // Check is the PICC of Classic MIFARE type
        if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
            piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
            piccType != MFRC522::PICC_TYPE_MIFARE_4K)
        {
            Serial.println(F("Your tag is not of type MIFARE Classic."));
            xSemaphoreGive(xSemaRFID);
            return -1;
        }
        if (1)
        {
            Serial.println(F("A new card has been detected."));

            // Store NUID into nuidPICC array
            for (byte i = 0; i < 16; i++)
            {
                nuidPICC[i] = mfrc522.uid.uidByte[i];
            }

            Serial.println(F("The NUID tag is:"));
            Serial.print(F("In hex: "));
            printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println();
            Serial.print(F("In dec: "));
            printDec(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println();
            // write_RFID(10, "程俊聪程俊聪程俊聪");
            // 验证密码
            // #define qwert 1
            // #ifdef qwert
            char *handle_arg = "/spiffs/di.mp3";

            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(200);
            digitalWrite(audio_ctrl,LOW);
            if (1)
            {
                int block = RC_CM_NUM;
                byte status;
                String replay_str;

                // Serial.println("Authenticating using key A...");
                status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK)
                {
                    Serial.print("PCD_Authenticate() failed: ");
                    Serial.println(mfrc522.GetStatusCodeName(status));
                }
                else
                {
                    Serial.print("PCD_Authenticate() AAAA ok: ");
                }
                status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK)
                {
                    Serial.print("PCD_Authenticate() failed: ");
                    Serial.println(mfrc522.GetStatusCodeName(status));
                    rc522_init();
                    xSemaphoreGive(xSemaRFID);

                    return -1;
                }
                else
                {
                    Serial.print("PCD_Authenticate() BBBB ok: ");
                }

                /* 开始读取 */
                byte buffer1[50];
                unsigned char len233 = sizeof(buffer1);
                status = mfrc522.MIFARE_Read(RC_DRIVER_NAME, buffer1, &len233);
                if (status != MFRC522::STATUS_OK)
                {
                    Serial.print(F("Reading failed: "));
                    Serial.println(mfrc522.GetStatusCodeName(status));
                    rc522_init();
                    xSemaphoreGive(xSemaRFID);
                    return -1;
                }
                else
                {

                    ESP_LOGE(TAG, "认证成功--%s", buffer1);
                    buffer1[16] = '\0';
                    String davicername = String((char *)buffer1);
                    ESP_LOGE(TAG, "认证成功1--%s", davicername.c_str());
                    rfid_info.driver_name = davicername;
                    // memset(buffer1,0,sizeof(buffer1));
                    String cmp_id = MACstr;
                    rfid_info.macIc=0;
                    if(cmp_id.indexOf(rfid_info.driver_name)==0)
                    {
                        rc522_init();
                        rfid_info.number=0;
                        rfid_info.macIc=1;
                        xSemaphoreGive(xSemaRFID);
                        return 1;
                    }
                    status = mfrc522.MIFARE_Read(RC_DRIVER_NUM, buffer1, &len233);
                    if (status != MFRC522::STATUS_OK)
                    {
                        Serial.print(F("Reading failed: "));
                        Serial.println(mfrc522.GetStatusCodeName(status));
                        rc522_init();
                        xSemaphoreGive(xSemaRFID);
                        return -1;
                    }
                    else
                    {
                        buffer1[16] = '\0';
                        String davicerID = String((char *)buffer1);
                        ESP_LOGE(TAG, "认证成功2--%s", davicerID.c_str());
                        rfid_info.id = davicerID;
                        // memset(buffer1,0,sizeof(buffer1));
                        status = mfrc522.MIFARE_Read(RC_CM_NUM, buffer1, &len233);
                        if (status != MFRC522::STATUS_OK)
                        {
                            Serial.print(F("Reading failed: "));
                            Serial.println(mfrc522.GetStatusCodeName(status));
                            rc522_init();
                            xSemaphoreGive(xSemaRFID);
                            return -1;
                        }
                        else
                        {

                            buffer1[18] = '\0';
                            String driverphone = String((char *)buffer1);
                            ESP_LOGE(TAG, "认证成功3--%s", driverphone.c_str());
                            rfid_info.id = rfid_info.id + driverphone.substring(0, 2);
                            rfid_info.phone = driverphone.substring(2);
                            ESP_LOGE(TAG, "认证成功3--%s,==%s,==%s", rfid_info.id.c_str(), rfid_info.phone.c_str(), rfid_info.driver_name.c_str());
                            davicerID = rfid_info.id.substring(4);
                            get_number(rfid_info.id);
                            if(rfid_info.number!=0)
                            {
                              String ICuser = davicername + "#" + rfid_info.id + "#" + phone+"#"+uuid;
                              rfid_cfg_local.putString(String(davicerID).c_str(), ICuser.c_str());
                             doc_replay.clear();
                             doc_replay["code"] = 200;
                             doc_replay["cmd"] = "addICUser";
                             doc_replay["name"] = davicername.c_str();
                             doc_replay["index"] = rfid_info.number;
                             doc_replay["id"] = rfid_info.id.c_str();
                             doc_replay["phone"] = phone.c_str();
                             doc_replay["card"] =  uuid.c_str();
                             replay_str.clear();
                             serializeJson(doc_replay, replay_str);
                            char *handle_arg = "/spiffs/writerIC.mp3";
                            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                            ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());
                            delay(2000);
                            digitalWrite(audio_ctrl,LOW);
                            pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
                            pTxCharacteristic->notify();
                            }

                            rc522_init();
                            xSemaphoreGive(xSemaRFID);
                            xSemaphoreGive(xSemaRFID);
                            delay(2000);
                            return 1;
                        }
                    }
                }
            }
            // #endif
        }
        xSemaphoreGive(xSemaRFID);
    }
    return -1;
}
void authorization_save(String phone)
{
    if (authorization_writer(phone) != 1)
    {
        String replay_str;
        doc_replay.clear();
        doc_replay["code"] = 404;
        doc_replay["cmd"] = "addICUser";
        doc_replay["msg"] = "授权失败";
        replay_str.clear();
        serializeJson(doc_replay, replay_str);
        char *handle_arg = "/spiffs/noIC.mp3";
        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        delay(1200);
        digitalWrite(audio_ctrl,LOW);
        ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());
         
         rc522_init();
        xSemaphoreGive(xSemaRFID);
        xSemaphoreGive(xSemaRFID);
        pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
        pTxCharacteristic->notify();
    }
}

void face_save(String drivername, String driverid, String driverphone)
{
    String replay_str;
    pinMode(CAR_CON, OUTPUT);
    if((dev_cfg.getInt("LOCK_ACTION", 0) != 1)&& (digitalRead(CAR_CON) == 0))
    {
        String faceData = drivername + "#" + driverid + "#" + driverphone;
        DateTime now = rtc.now();
        work_rec.putString(String(now.unixtime()).c_str(), faceData+"#3");
        ESP_LOGE(TAG, "drivername : %s", faceData.c_str());
        car_onoff(1);
        doc_replay.clear();
        doc_replay["code"] = 200;
        doc_replay["cmd"] = "faceUser";
        // char  *handle_arg = "/spiffs/unlock.mp3";
        // esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        // if (blackboard.system.fingerTaskHandle != nullptr)
        // {
        //     vTaskSuspend(blackboard.system.fingerTaskHandle); // 挂起任务
        // }
        // delay(1000);
        // if (blackboard.system.rc522TaskHandle != nullptr)
        // {
        //     vTaskSuspend(blackboard.system.rc522TaskHandle);
        // }
    }
    else
    {
        doc_replay.clear();
        doc_replay["code"] = 404;
        doc_replay["msg"] = "已启动";
        doc_replay["cmd"] = "faceUser";
    }
    replay_str.clear();
    serializeJson(doc_replay, replay_str);

    ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());

    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
    pTxCharacteristic->notify();
    xSemaphoreGive(xSemaRFID);
    xSemaphoreGive(xSemaFINGER);
    rc522_init();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
void Restore(void)
{
    work_rec.clear();
    rfid_cfg_local.clear();
    finger.emptyDatabase();
     if (finger.getTemplateCount() == 0)
     {
       fg_cfg_local.clear();
     }

    dev_cfg.putInt("LOCK_ACTION", 0);
    dev_cfg.putInt("safe_belt_tm", 2);
    dev_cfg.putInt("SB_TYPE", 0);
    dev_cfg.putInt("SB_LOCK",655360); 
    dev_cfg.putInt("card_verify",0);
    dev_cfg.putString("DEVICE_NAME","001");
    dev_cfg.putString("verifyCode","0000");
    dev_cfg.putInt("fg_num",100);
}
char *finger_c3 = "/finger_c3.bin";

bool ota_flag=false;
void updateFirmware() {
  // 开始OTA更新
     File file = SPIFFS.open("/finger_c3.bin", FILE_READ);
   if (!file) {
    Serial.println("Failed to open firmware file");
    return;
  }
  size_t size = file.size();
    Serial.print("File size: ");
    Serial.println(size);

    if (!Update.begin(size)) // 开始固件升级，检查空可用间大小，如果正在升级或空间不够则返回false
    {
        Serial.println("升级不可用");
        return;
    }
    size_t written = Update.writeStream(file); // 将数据写入到OTA区域 // 使用writeStream方法的话前面Update.begin()必须填写要更新的固件大小
    Serial.println("写入" + String(written) + "字节到OTA区域");
    if (!Update.end()) // 完成数据写入，设置在系统重启后自动将OTA区域固件移动到Sketch区域
    {
        Serial.println("升级失败 Error #: " + String(Update.getError()));
        return;
    }

    file.close(); // 关闭.bin文件

    Serial.println("升级操作完成,模块将在5秒后重启以完成固件升级");
    delay(1000); 
    ESP.restart(); // 重启设备
}
bool verify_MD5(String vreifymd5)
{
    if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return false;
  }

  Serial.println("SPIFFS Mounted");

  // 检查文件是否存在
  if(!SPIFFS.exists("/finger_c3.bin")){
    Serial.println("File not found");
    return false;
  }

  // 打开文件
  File file = SPIFFS.open("/finger_c3.bin", FILE_READ);
  if(!file){
    Serial.println("Failed to open file");
    return false;
  }
  // 获取文件长度
  size_t fileSize = file.size();
  Serial.print("File size: ");
  Serial.println(fileSize);

  // 计算文件的MD5值
  MD5Builder md5;
  md5.begin();
  md5.addStream(file, file.size());
  md5.calculate();
   
  // 获取计算的MD5值
  String calculatedMD5 = md5.toString();
  Serial.println("Calculated MD5: " + calculatedMD5);
  Serial.println("vreifymd5 MD5: " + vreifymd5);
  // 与预期的MD5值进行比较
//   String expectedMD5 = md5; // 请替换为您期望的MD5值 
  if(calculatedMD5.equals(vreifymd5)) {
    Serial.println("MD5 verification passed!");
    // updateFirmware();
    return true;
  } else {
    Serial.println("MD5 verification failed!");
    return false;
  }
}
void appendDataToFile(uint8_t* buffer, size_t size) {
 size_t bytesWritten;
 if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }   
  File file = SPIFFS.open("/finger_c3.bin", FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(size==0)
  {
      SPIFFS.remove("/finger_c3.bin");
  }
  else 
  {
        bytesWritten = file.write(buffer, size);
         if(bytesWritten != size){
            Serial.println("Error writing to file");
  }
  }// 将接收到的数据写入文件
  file.close();
}
void prepare_ota(int status,String md5)
{
    if(0==status)
    {
        doc_replay.clear();
        doc_replay["code"] = 200;
        doc_replay["msg"] = "准备成功";
        doc_replay["cmd"] = "ota";
        doc_replay["status"] = 0;
        ota_flag=true;
        appendDataToFile((uint8_t*)"", 0);

    }
   else if(1==status)
    {
        doc_replay.clear();
        if(verify_MD5(md5))
        {
            doc_replay["code"] = 200;
            doc_replay["msg"] = "md5检验成功";
            // 执行OTA更新
            updateFirmware();
            // ble_ota();
        }
        else{
               doc_replay["code"] = 404;
               doc_replay["msg"] = "md5检验失败";
        }
        doc_replay["cmd"] = "ota";
        doc_replay["status"] = 1;
        ota_flag=false;
    }
    String replay_str;
    replay_str.clear();
    serializeJson(doc_replay, replay_str);

    ESP_LOGI(TAG, "返回消息：%s", replay_str.c_str());

    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
    pTxCharacteristic->notify();

    // else if(2==status)
    // {
       
    // }

}
void task_cmd(void *parm)
{
    // if (xSemaphoreTake(xSemaCMD, 0) == pdTRUE)
    {

        if (!doc_ble.containsKey("cmd"))
        {
            vTaskDelete(NULL);
            vTaskDelay(portMAX_DELAY);
        }
        else
        {
            serializeJsonPretty(doc_ble, Serial);
            String cmd = doc_ble["cmd"]; // "deleteUser"
            if (cmd == "deleteFingerUser")
            {
                /* {"cmd":"deleteUser","index":"0,5"} */
                ESP_LOGE(TAG, "删除指纹");
                String driverName = doc_ble["index"];
                delete_save(driverName);
            }
            else if (cmd == "addFingerUser")
            {
                /* {"cmd":"addUser","driverName":"我有葱"} */
                ESP_LOGE(TAG, "新增指纹");
                String driverName = doc_ble["name"];
                String driverid = doc_ble["id"];
                String driverphone = doc_ble["phone"];
                if((finger.getTemplateCount()<(log_count+1))&&(verify_fgnum(driverid)==true))
                {
                    enroll_save(driverName, driverid, driverphone);
                }
                else {
                    String replay_str;
                     doc_replay.clear();
                     doc_replay["code"] = 404;
                     doc_replay["cmd"] = "addFingerUser";
                     doc_replay["msg"] = "重复授权";
                     replay_str.clear();
                    serializeJson(doc_replay, replay_str);
                    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
                    pTxCharacteristic->notify();
                }
            }
            else if (cmd == "writeTerminal")
            {
                ESP_LOGE(TAG, "写设备卡");
                String driverName = MACstr;
                Serial.println(driverName.c_str());
                String driverid = "#";
                String driverphone = "#";
                IC_save(driverName, driverid, driverphone);
            }
            else if (cmd == "writeUser")
            {
                ESP_LOGE(TAG, "写设司机卡");
                String driverName = doc_ble["name"];
                String driverid = doc_ble["id"];
                String driverphone = doc_ble["phone"];
                get_number(driverid);
                ESP_LOGE(TAG, "写设司机卡%d", rfid_info.number);
                if ((rfid_info.number <= log_count) && (rfid_info.number != 0))
                {
                    IC_save(driverName, driverid, driverphone);
                }
                 if((rfid_info.number==0)||(rfid_info.number>log_count))
                    {
                      String replay_str;
                     doc_replay.clear();
                     doc_replay["code"] = 404;
                     doc_replay["cmd"] = "writeUser";
                     doc_replay["msg"] = "重复授权";
                     replay_str.clear();
                    serializeJson(doc_replay, replay_str);
                    pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
                    pTxCharacteristic->notify();
                    }
            }
            else if (cmd == "addICUser")
            {
                ESP_LOGE(TAG, "司机钥匙卡授权");
                String driverphone = doc_ble["phone"];
                get_number("1234");
                if ((rfid_info.number <= log_count) && (rfid_info.number != 0))
                {
                   authorization_save(driverphone);
                }
                    if((rfid_info.number==0)||(rfid_info.number>log_count))
                    {
                      String replay_str;
                     doc_replay.clear();
                     doc_replay["code"] = 404;
                     doc_replay["cmd"] = "addICUser";
                     if(rfid_info.macIc==1)
                      doc_replay["msg"] = "非人员卡";
                      else
                     doc_replay["msg"] = "重复授权";
                     replay_str.clear();
                     serializeJson(doc_replay, replay_str);
                     pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
                     pTxCharacteristic->notify();
                    }
                
            }
            else if (cmd == "deleteICUser")
            {
                ESP_LOGE(TAG, "删除卡授权");
                String driverName = doc_ble["index"];
                RFdelete_save(driverName);
            }
            else if (cmd == "getICUser")
            {
                ESP_LOGE(TAG, "钥匙卡授权列表");
                int pn = doc_ble["pN"];
                int ps = doc_ble["pS"];
                get_rfidlist(pn, ps);
                // RFdelete_save();
            }
            else if (cmd == "faceUser")
            {
                ESP_LOGE(TAG, "刷脸启动");
                String driverName = doc_ble["name"];
                String driverid = doc_ble["id"];
                String driverphone = doc_ble["phone"];
                face_save(driverName, driverid, driverphone);
                // RFdelete_save();
            }
            else if(cmd=="verifyCard")            
            {
                ESP_LOGE(TAG, "刷卡启动");
                String code = doc_ble["code"];
                String phoneid =dev_cfg.getString("phone_id","0");
                if (phoneid.indexOf(code)!=0&&code.length()>=4)
                {
                    dev_cfg.putString("phone_id","0");
                    DateTime now = rtc.now();
                    work_rec.putString(String(now.unixtime()).c_str(),dev_cfg.getString("Two-factor","{}"));
                    blackboard.car_sta.deviceStatus = DEV_AUTH;
                    car_onoff(1);
                    doc_replay.clear();
                    doc_replay["code"] = 200;
                    doc_replay["cmd"] = "verifyCard";
                }
                else{
                    doc_replay.clear();
                    doc_replay["code"] = 404;
                    doc_replay["cmd"] = "verifyCard";
                    doc_replay["msg"] = "验证失败";
                }

                String replay_str;
               replay_str.clear();
               serializeJson(doc_replay, replay_str);
               pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
               pTxCharacteristic->notify();
            }
            else if (cmd == "getWork")
            {
                /* {"cmd":"addUser","driverName":"我有葱"} */
                ESP_LOGE(TAG, "工作记录");
                int pn = doc_ble["pN"];
                int ps = doc_ble["pS"];
                get_worklist(pn, ps);
                // String replay_str = "{\"code\":200,\"cmd\":\"getWork\",\"pN\":1,\"list\":[\"1704872617#张三\",\"1704872617#李四\"]}";
                // pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
                // pTxCharacteristic->notify();
                // String driverName = doc_ble["driverName"];
                // enroll_save(driverName);
            }
            else if (cmd == "verifyCode")            //设置验证密码
            {
                ESP_LOGE(TAG,"验证密码 verifyCode is %s", dev_cfg.getString("verifyCode","0000").c_str());
               if(doc_ble["code"]==dev_cfg.getString("verifyCode","0000"))
               {
                  doc_replay.clear();
                  doc_replay["code"] = 200;
                  doc_replay["cmd"] = "verifyCode";
               }
               else 
               {
                doc_replay.clear();
                 doc_replay["code"] = 404;
                 doc_replay["cmd"] = "verifyCode";
                 doc_replay["msg"] = "密码错误";
               }
               String replay_str;
               replay_str.clear();
               serializeJson(doc_replay, replay_str);
               pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
               pTxCharacteristic->notify();
            }
            else if (cmd == "config")
            {
                /* {"cmd":"config","param":"LOCK_ACTION","type":"int","value":0} */

                const char *param = doc_ble["param"]; // "LOCK_ACTION"
                String type = doc_ble["type"];        // "int"
                 doc_replay.clear();
                 doc_replay["code"] = 200;
                 doc_replay["cmd"] = "config";
                 doc_replay["param"]=doc_ble["param"];
                ESP_LOGE(TAG, "高级设置");
                if (type == "int")
                {
                    if (String(param) == "rtc_time")
                    {
                        int value = doc_ble["value"]; // 0
                        ESP_LOGE(TAG, "设置时间 %d", value + 3600 * 8);

                        rtc.adjust(DateTime(value + 3600 * 8));
                        /* DateTime now = rtc.now();

                        Serial.print(now.year(), DEC);
                        Serial.print('/');
                        Serial.print(now.month(), DEC);
                        Serial.print('/');
                        Serial.print(now.day(), DEC);
                        Serial.print(' ');
                        Serial.print(now.hour(), DEC);
                        Serial.print(':');
                        Serial.print(now.minute(), DEC);
                        Serial.print(':');
                        Serial.print(now.second(), DEC);
                        Serial.println(); */
                    }
                    else
                    {

                        int value = doc_ble["value"]; // 0
                        dev_cfg.putInt(param, value);
                    }
                }
                else if (type == "string")
                {
                    String value = doc_ble["value"]; // 0
                    ESP_LOGE(TAG, "VALUE:%s", value.c_str());

                    dev_cfg.putString(param, value);
                }
                if ((String(param) == "LOCK_ACTION"))
                {
                    int value = doc_ble["value"]; // 0
                    if (value == 3)
                    {
                        car_onoff(DEV_AUTH);
                        delay(2000);
                    }
                }
                if((String(param) == "Restore"))
                {
                    int value=doc_ble["value"];
                    if(value==8888)
                    {
                        ESP_LOGE(TAG, "恢复出厂设置");
                        Restore();
                    }
                    else doc_replay["code"] = 404;
                }
                if((String(param) == "SET_CODE"))
                {
                    String verifyCode=doc_ble["value"];
                    dev_cfg.putString("verifyCode",verifyCode);
                    ESP_LOGE(TAG,"设置密码 verifyCode is %s", dev_cfg.getString("verifyCode","0000").c_str());
                }
                if((String(param) == "card_verify"))      
                {
                    ESP_LOGE(TAG, "设置刷卡人员校验");
                    int value=doc_ble["value"];
                    dev_cfg.putInt("card_verify",value);
                }
                if((String(param) == "finger_num")) 
                {
                   ESP_LOGE(TAG, "设置人员指纹数量");
                    int value=doc_ble["value"];
                    dev_cfg.putInt("fg_num",value);  
                }
                sys_deafuat();
                String replay_str;
                replay_str.clear();
               serializeJson(doc_replay, replay_str);
               pTxCharacteristic->setValue(arduinoStringToStdString(replay_str));
               pTxCharacteristic->notify();
            }
            else if (cmd == "getFingerUser")
            {
                /* {"pN":1,"pS":10,"cmd":"getUser"} */
                ESP_LOGE(TAG, "获取用户");
                int pn = doc_ble["pN"];
                int ps = doc_ble["pS"];
                get_usrlist(pn, ps);
            }
            else if (cmd == "clearUser")
            {
                /* {"cmd":"clearUser"}*/
                ESP_LOGE(TAG, "清空用户");
                empty_all();
            }
            else if (cmd == "getVersion") 
            {
               ESP_LOGE(TAG, "获取版本号");
               get_Version();
            }
            else if (cmd == "ota") 
            {
                ESP_LOGE(TAG, "OTA");
                int status= doc_ble["status"];
                String md5=doc_ble["md5"];
                prepare_ota(status,md5);

            }
            xSemaphoreGive(xSemaCMD);

            vTaskDelete(NULL);
            vTaskDelay(portMAX_DELAY);
        }
    }
    // else
    // {
    //     ESP_LOGE(TAG, "忙......");

    //     vTaskDelete(NULL);
    //     vTaskDelay(portMAX_DELAY);
    // }
}

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        DeserializationError error = deserializeJson(doc_ble, (rxValue));
        if (!error)
        {
            xTaskCreatePinnedToCore(&task_cmd,  // task
                                    "task_cmd", // task name
                                    4096,       // stack size
                                    NULL,       // parameters
                                    10,         // priority
                                    NULL,       // returned task handle
                                    0           // pinned core
            );
        }
        if((ota_flag==true)&&(doc_ble["cmd"]!="ota"))
        {
             ESP_LOGE(TAG, "onWrite:%d", pCharacteristic->getValue().length());
             appendDataToFile((uint8_t*)pCharacteristic->getValue().data(), pCharacteristic->getValue().length());
        }
    }
};
#include "esp_bt_device.h"
void ble_task_setup()
{
    //   Serial.begin(115200);

    // Create the BLE Device  
    //   get_worklist(1, 5);    //防止长时间不获取工作记录导致死机。不可以删   

     xSemaCMD = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaCMD);
    String dev_name = "demo";
    if (dev_cfg.getString("DEVICE_NAME", "001") == "001")
    {
        uint64_t chipId = ESP.getEfuseMac();
        dev_name = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
        dev_name = dev_name.substring(0,6);
        ESP_LOGE(TAG, "MACstr:%s", dev_name.c_str());
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
    const uint8_t *bleAddr = esp_bt_dev_get_address(); // 查询蓝牙的mac地址，务必要在蓝牙初始化后方可调用！
    sprintf(MACstr, "%02x:%02x:%02x:%02x:%02x:%02x\n", bleAddr[0], bleAddr[1], bleAddr[2], bleAddr[3], bleAddr[4], bleAddr[5]);
    dev_cfg.putString("MACstr",MACstr);
    ESP_LOGE(TAG, "MACstr:%s", MACstr);
    // 状态服务
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);
    /* ssp服务 */
    BLEService *pService_ssp = pServer->createService(SERVICE_UUID_SSP);
    pTxCharacteristic = pService_ssp->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
    pRxCharacteristic = pService_ssp->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);

    pCharacteristic->addDescriptor(new BLE2902());
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();
    pService_ssp->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    // pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}
void ble_task_loop()
{

    // notify changed value
    // ESP_LOGE(TAG, "当前连接数 %d", con_count);

    if (deviceConnected || con_count)
    {

        // std::string value2 = "104";
        // value = blackboard.car_speed_ble > blackboard.car_speed ? blackboard.car_speed_ble : blackboard.car_speed;
        // value = blackboard.car_speed;
        // pCharacteristic->setValue((uint8_t *)&value, 1);
        // pTxCharacteristic->setValue(arduinoStringToStdString("{\"index\":"+String(value++)+",\"driverName\":\"李四\"}"));
        if(blackboard.car_sta.deviceStatus==DEV_TWO)          //双重认证刷卡成功后回复
        {
            char value[128]; 
            sprintf(value,"{\"cmd\":\"deviceVerifyCard\",\"id\":\"%s\"}",(dev_cfg.getString("phone_id","0")).c_str());
            // pCharacteristic->setValue("value");
            pTxCharacteristic->setValue(arduinoStringToStdString(value));
            //    pTxCharacteristic->notify();
            // pCharacteristic->notify();
            pTxCharacteristic->notify();
            blackboard.car_sta.deviceStatus=DEV_OFF;
            Serial.println(value);
        }
        // Serial.println(blackboard.car_sta.deviceStatus);
        // pCharacteristic->setValue(value2);
        // pCharacteristic->notify();
        // pTxCharacteristic->notify();
        // ESP_LOGE(TAG, "发送蓝牙通知 %d", value);

        // value = (speed++) % 20;
        delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    else
    {
        // ESP_LOGE(TAG, "没有设备连接");
    }
}

void speed_ble_entry(void *params)
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
