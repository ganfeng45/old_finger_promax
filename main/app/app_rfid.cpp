#include"util/myutil.h"
#include "blackboard.h"
#include <MFRC522_I2C.h>
#include <Wire.h>
/* rc522 */
#define TAG "app_rfid"
/* rc522 */
SemaphoreHandle_t xSemaRFID = NULL; // 创建信号量Handler
TaskHandle_t rfid_acce_task = NULL;
#define RC_SDA_PIN 13
#define RC_CLK_PIN 14
// #define RC_SDA_PIN 9
// #define RC_CLK_PIN 10
#define RC_RST_PIN (8)
#define RC_ARD 0X28

#define RC_DRIVER_NAME 16
#define RC_DRIVER_NUM 17
#define RC_CM_NUM 18

byte nuidPICC[4] = {0};
MFRC522::MIFARE_Key key;
MFRC522 mfrc522(RC_ARD, RC_RST_PIN); // Create MFRC522 instance.
StaticJsonDocument<128> doc_iic;

extern void modem_TTS(String str);
// {
//     play_mp3_dec(str);
// }

void printHex(int num, int precision)
{
    char tmp[16];
    char format[128];

    sprintf(format, "%%.%dX", precision);

    sprintf(tmp, format, num);
    Serial.print(tmp);
}
void printDec(byte *buffer, byte bufferSize)
{
    for (byte i = 0; i < bufferSize; i++)
    {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], DEC);
    }
}
void rfid_rst()
{

}

bool rc522_init()
{

    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }
    rfid_rst();
    delay(1000);

    return mfrc522.PCD_Init();
}
// String md5str(String str, int len)
// {
//     /* 6CD3556DEBDA54BCA6 */
//     md5.begin();
//     md5.add(str);
//     md5.calculate();
//     String fullMD5 = md5.toString();
//     return fullMD5.substring(fullMD5.length() - len);
// }
int rc522_read3()
{
    if (xSemaphoreTake(xSemaRFID, portMAX_DELAY) == pdTRUE)
    {
        if (!mfrc522.PICC_IsNewCardPresent())
        {
            xSemaphoreGive(xSemaRFID);
            return -1;
        }

        if (!mfrc522.PICC_ReadCardSerial())
        {
            xSemaphoreGive(xSemaRFID);
            return -1;
        }
        MFRC522::PICC_Type piccType = (MFRC522::PICC_Type)mfrc522.PICC_GetType(mfrc522.uid.sak);
        Serial.println(mfrc522.PICC_GetTypeName(piccType));

        if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
            piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
            piccType != MFRC522::PICC_TYPE_MIFARE_4K)
        {
            ESP_LOGW(TAG, "Your tag is not of type MIFARE Classic.");
            xSemaphoreGive(xSemaRFID);
            return -1;
        }

        if (1)
        {
            ESP_LOGI(TAG, "A new card has been detected.");

            for (byte i = 0; i < 4; i++)
            {
                nuidPICC[i] = mfrc522.uid.uidByte[i];
            }

            ESP_LOGI(TAG, "The NUID tag is:");
            ESP_LOGI(TAG, "In hex: ");
            ESP_LOGI(TAG, "In dec: ");
            printDec(mfrc522.uid.uidByte, mfrc522.uid.size);
            ESP_LOGI(TAG, "");

            if (1)
            {
                int block = RC_DRIVER_NUM;
                byte status;

                status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK)
                {
                    // ESP_LOGE(TAG, "PCD_Authenticate() failed: %s", mfrc522.GetStatusCodeName(status));
                    Serial.println(mfrc522.GetStatusCodeName(status));
                }
                else
                {
                    ESP_LOGI(TAG, "PCD_Authenticate() AAAA ok");
                }
                status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK)
                {
                    // ESP_LOGE(TAG, "PCD_Authenticate() failed: %s", mfrc522.GetStatusCodeName(status));
                    Serial.println(mfrc522.GetStatusCodeName(status));

                    rc522_init();
                    xSemaphoreGive(xSemaRFID);
                    return -1;
                }
                else
                {
                    ESP_LOGI(TAG, "PCD_Authenticate() BBBB ok");
                }

                byte buffer1[18];
                byte buffer2[18];
                unsigned char len233 = sizeof(buffer1);
                status = mfrc522.MIFARE_Read(block, buffer1, &len233);
                if (status != MFRC522::STATUS_OK)
                {
                    // ESP_LOGE(TAG, "Reading failed: %s", mfrc522.GetStatusCodeName(status));
                    Serial.println(mfrc522.GetStatusCodeName(status));

                    rc522_init();
                    xSemaphoreGive(xSemaRFID);
                    return -1;
                }
                else
                {
                    ESP_LOGI(TAG, "认证成功--%s", buffer1);
                    buffer1[15] = '\0';
                    String myString = String((char *)buffer1);
                    ESP_LOGI(TAG, "认证成功2--%s", myString.c_str());
                    ESP_LOGI(TAG, "MD5 Hash: ");
                    for (int i = 0; i < 18; i++)
                    {
                        ESP_LOGI(TAG, "%02X", buffer1[i]);
                    }
                    ESP_LOGI(TAG, "");

                    if (rfid_cfg_local.isKey(myString.c_str()))
                    {
                        StaticJsonDocument<128> doc_rfid;
                        String str2json = rfid_cfg_local.getString(myString.c_str(), "{");
                        ESP_LOGI(TAG, "认证成功233--%s", str2json.c_str());

                        DeserializationError error = deserializeJson(doc_rfid, str2json);
                        if (error)
                        {
                            ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
                        }
                        else
                        {
                            const char *driver_name = doc_rfid["name"];
                            int authorize_power = doc_rfid["authorize_power"];
                            const char *driver_num = doc_rfid["driver_num"];
                            xSemaphoreGive(xSemaRFID);
                            return 1;
                        }
                    }
                    else
                    {
                        memset(buffer1, 0, sizeof(buffer1));
                        status = mfrc522.MIFARE_Read(RC_CM_NUM, buffer1, &len233);
                        if (status != MFRC522::STATUS_OK)
                        {
                            // ESP_LOGE(TAG, "Reading failed: %s", mfrc522.GetStatusCodeName(status);
                            Serial.println(mfrc522.GetStatusCodeName(status));

                            rc522_init();
                            xSemaphoreGive(xSemaRFID);
                            return -1;
                        }
                        else
                        {
                            String cmp_id = dev_cfg.getString("company_num", "cjc1024");
                            String ttr = md5str(cmp_id, 15) + "T";
                            ESP_LOGI(TAG, "管理员2 %s 长度 %d", ttr.c_str(), ttr.length());
                            buffer1[16] = '\0';
                            String myString = String((char *)buffer1);
                            ESP_LOGI(TAG, "认证成功cmp23--%s 长度：%d", myString.c_str(), myString.length());
                            if (myString == ttr)
                            {
                                ESP_LOGI(TAG, "管理员刷卡开机");
                                xSemaphoreGive(xSemaRFID);
                                return 1;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ESP_LOGI(TAG, "Card read previously.");
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        xSemaphoreGive(xSemaRFID);
    }
    return -1;
}

int rc522_read()
{
    if (xSemaphoreTake(xSemaRFID, 2000) == pdTRUE)
    {

        if (!mfrc522.PICC_IsNewCardPresent())
        {
            // rc522_init();
            xSemaphoreGive(xSemaRFID);
            return -1;
        }

        // Verify if the NUID has been readed
        if (!mfrc522.PICC_ReadCardSerial())
        {
            // rc522_init();

            xSemaphoreGive(xSemaRFID);
            return -1;
        }
        MFRC522::PICC_Type piccType = (MFRC522::PICC_Type)mfrc522.PICC_GetType(mfrc522.uid.sak);
        Serial.print(F("PICC type: "));
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
            for (byte i = 0; i < 4; i++)
            {
                nuidPICC[i] = mfrc522.uid.uidByte[i];
            }

            {
                int block = RC_DRIVER_NUM;
                byte status;

                status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK)
                {
                    ESP_LOGW(TAG, "PCD_Auth A failed: ");
                    Serial.println(mfrc522.GetStatusCodeName(status));
                    return -1;
                }
                else
                {
                    ESP_LOGD(TAG, "PCD_Auth A ok: ");
                }
                status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK)
                {
                    ESP_LOGW(TAG, "PCD_Auth B failed: ");

                    Serial.println(mfrc522.GetStatusCodeName(status));
                    rc522_init();
                    xSemaphoreGive(xSemaRFID);

                    return -1;
                }
                else
                {
                    ESP_LOGD(TAG, "PCD_Auth B ok: ");
                }

                /* 开始读取 */
                byte buffer_driver_num[18];
                byte buffer_cmp[18];
                unsigned char len233 = sizeof(buffer_driver_num);
                status = mfrc522.MIFARE_Read(RC_DRIVER_NUM, buffer_driver_num, &len233);
                if (status != MFRC522::STATUS_OK)
                {
                    ESP_LOGW(TAG, "Reading failed: ");
                    Serial.println(mfrc522.GetStatusCodeName(status));
                    rc522_init();
                    xSemaphoreGive(xSemaRFID);
                    return -1;
                }
                else
                {

                    // ESP_LOGD(TAG, "认证成功--%s", buffer1);
                    buffer_driver_num[15] = '\0';
                    String myString = String((char *)buffer_driver_num);
                    ESP_LOGI(TAG, "buffer1:%s", myString.c_str());
                    if (rfid_cfg_local.isKey(myString.c_str()))
                    {

                        {

                            StaticJsonDocument<128> doc_rfid;
                            String str2json = rfid_cfg_local.getString(myString.c_str(), "{");
                            ESP_LOGI(TAG, "认证成功233--%s", str2json.c_str());

                            DeserializationError error = deserializeJson(doc_rfid, str2json);
                            if (error)
                            {
                                Serial.print("deserializeJson() failed: ");
                                Serial.println(error.c_str());
                            }
                            else
                            {

                                static CMD_AUTH data_auth;
                                data_auth.auth_sc = AUTH_RFID;
                                data_auth.id = doc_rfid["driver_num"].as<String>();
                                data_auth.name = doc_rfid["name"].as<String>();
                                data_auth.ps = str2json;
                                broker.publish("auth", &data_auth);

                                xSemaphoreGive(xSemaRFID);
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        memset(buffer_driver_num, 0, sizeof(buffer_driver_num));
                        status = mfrc522.MIFARE_Read(RC_CM_NUM, buffer_driver_num, &len233);
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
                            String cmp_id = dev_cfg.getString("company_num", "cjc1024");
                            String ttr = md5str(cmp_id, 15) + "T";
                            ESP_LOGE(TAG, "company_num: %s 长度 %d", ttr.c_str(), ttr.length());
                            buffer_driver_num[16] = '\0';
                            String myString = String((char *)buffer_driver_num);
                            ESP_LOGE(TAG, "认证成功cmp23--%s 长度：%d", myString.c_str(), myString.length());
                            if (myString == ttr)
                            {
                                ESP_LOGE(TAG, "管理员刷卡开机");
                                xSemaphoreGive(xSemaRFID);
                                return 1;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            Serial.println(F("Card read previously."));
        }

        // Halt PICC
        mfrc522.PICC_HaltA();

        // Stop encryption on PCD
        mfrc522.PCD_StopCrypto1();
        xSemaphoreGive(xSemaRFID);
    }
    else
    {
        ESP_LOGE(TAG, "获取句柄的h");
    }
    return -1;
}

void rc522_dec(void *params)
{
    while (1)
    {
        // if ( == 1)
        // {
        // }
        delay(1000);
        rc522_read();
        if (blackboard.car_sta.deviceStatus == DEV_AUTH)
        {
            ESP_LOGD(TAG, "rfid 授权自动关闭");
            vTaskDelete(NULL);
        }
    }
}

void rfid_write_dec(void *param)
{
    CMD_WRITE_RFID *info = (CMD_WRITE_RFID *)(param);
    bool witre_flag = true;
    int count_timeout = 0;

    if (xSemaphoreTake(xSemaRFID, portMAX_DELAY) == pdTRUE)
    {
        modem_TTS("开始制作钥匙卡");

        rc522_init();
        delay(1000);
        while (!mfrc522.PICC_IsNewCardPresent())
        {
            modem_TTS(F("请放上卡片"));
            //blackboard.lcd_info.info_bar = "请放上卡片";
            if (count_timeout++ > 20)
            {
                delay(1000);
                modem_TTS(F("制卡超时"));
                xSemaphoreGive(xSemaRFID);
                vTaskDelete(NULL);
            }
            delay(1000);
        }
        if (!mfrc522.PICC_ReadCardSerial())
        {
            rc522_init();
            // xSemaphoreGive(xSemaRFID);
        }
        for (byte i = 0; i < 4; i++)
        {
            nuidPICC[i] = mfrc522.uid.uidByte[i];
        }
        byte status;

        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, RC_DRIVER_NAME, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK)
        {
            ESP_LOGW(TAG, "PCD_Auth A failed: ");
            Serial.println(mfrc522.GetStatusCodeName(status));
            witre_flag = false;
        }

        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, RC_DRIVER_NAME, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK)
        {
            ESP_LOGW(TAG, "PCD_Auth B failed: ");
            Serial.println(mfrc522.GetStatusCodeName(status));
            witre_flag = false;
        }

        uint8_t buffer[16];
        memset(buffer, 0, sizeof(buffer));
        // strncpy((char *)buffer, "2022101762", 16);
        // strncpy((char *)buffer, getJsonValue(*pDoc, "driver_name", "twind"), 16);
        strncpy((char *)buffer, info->driver_name.c_str(), 16);
        status = mfrc522.MIFARE_Write(RC_DRIVER_NAME, buffer, 16);
        if (status != MFRC522::STATUS_OK)
        {
            ESP_LOGD(TAG, "driver_name Write failed");
            witre_flag = false;
        }
        if (1)
        {
            String key_driver_num = md5str(info->driver_num, 15);
            ESP_LOGD(TAG, "driver: %s key_driver_num txt:%s length：%d", info->driver_num.c_str(), key_driver_num.c_str(), key_driver_num.length());
            doc_iic.clear();
            doc_iic["authorize_power"] = info->authorize_power;
            doc_iic["name"] = info->driver_name;
            doc_iic["driver_num"] = info->driver_num;
            String doc2str_iic;
            serializeJson(doc_iic, doc2str_iic);
            rfid_cfg_local.putString(key_driver_num.c_str(), doc2str_iic);

            strncpy((char *)buffer, key_driver_num.c_str(), 15);
            status = mfrc522.MIFARE_Write(RC_DRIVER_NUM, buffer, 16);
            //  MFRC522::StatusCode status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
            if (status == MFRC522::STATUS_OK)
            {
                // screen.SetTextValue(DS_MAIN_PAGE, INFO_BAR, "NUM:" + getJsonValue<String>(*pDoc, "driver_num", "(error)"));
                //blackboard.lcd_info.info_bar = "NUM:" + info->driver_num;
                ESP_LOGD(TAG, "driver_num Write success");
            }
            else
            {
                witre_flag = false;
                ESP_LOGD(TAG, "driver_num Write failed");
            }
        }
        if (1)
        {
            String company_num_str = md5str(info->company_num, 15);
            ESP_LOGD(TAG, "cmp：%s company_num_str txt:%s len：%d", info->company_num.c_str(), company_num_str.c_str(), company_num_str.length());

            strncpy((char *)buffer, company_num_str.c_str(), 15);
            if (info->authorize_power == 1)
            {
                ESP_LOGD(TAG, "General authorized officer");
                buffer[15] = 70;
            }
            else if (info->authorize_power == 2)
            {
                ESP_LOGD(TAG, "Management authorized personnel");
                buffer[15] = 84;
            }
            status = mfrc522.MIFARE_Write(RC_CM_NUM, buffer, 16);
            //  MFRC522::StatusCode status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
            if (status == MFRC522::STATUS_OK)
            {
                // screen.SetTextValue(DS_MAIN_PAGE, INFO_BAR, "CMP:" + getJsonValue<String>(*pDoc, "company_num", "(error)"));
                //blackboard.lcd_info.info_bar = "CMP:" + info->company_num;
                ESP_LOGD(TAG, "company_num Write success");
            }
            else
            {
                witre_flag = false;
                ESP_LOGD(TAG, "company_num Write failed");
            }
        }
        delay(1000);
        if (witre_flag)
        {
            modem_TTS("制卡成功");
            //blackboard.lcd_info.info_bar = "制卡成功";
        }
        else
        {

            modem_TTS("制卡失败");
            //blackboard.lcd_info.info_bar = "制卡成功";
        }
        delay(2000);
        rc522_init();
        xSemaphoreGive(xSemaRFID);
    }

    vTaskDelete(NULL);
}

void rfid_write(const String &topic, void *param)
{
    xTaskCreatePinnedToCore(&rfid_write_dec,  // task
                            "rfid_write_dec", // task name
                            1024 * 10,        // stack size
                            param,            // parameters
                            5,                // priority
                            NULL,             // returned task handle
                            1                 // pinned core
    );
}
void auth_rfid(const String &topic, void *param)
{
    CMD_AUTH *data = (CMD_AUTH *)param;
}




void task_rfid(void *parm)
{
    // esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    ESP_LOGI(TAG, "task_rfid start");
    delay(1 * 1000);
    // vTaskDelete(NULL);

    // // ~TwoWire(0);
    // Wire.end();
    // delay(1 * 1000);
    // Wire.begin(RC_SDA_PIN, RC_CLK_PIN); // Initialize I2C 8/9
    if (rc522_init())
    {
        xSemaRFID = xSemaphoreCreateBinary(); // 创建二进制信号量
        xSemaphoreGive(xSemaRFID);
        // broker.subscribe("rfid_write", rfid_write);
        // broker.subscribe("update_ic_group", rfid_updata_group);
        // broker.subscribe("auth", auth_rfid);

        // broker.subscribe("ud_fingerg", finger_updata_g);
        xTaskCreatePinnedToCore(&rc522_dec,      // task
                                "rc522_dec",     // task name
                                1024*2,            // stack size
                                NULL,            // parameters
                                15,              // priority
                                &rfid_acce_task, // returned task handle
                                0                // pinned core
        );
    }
    else
    {
        ESP_LOGI(TAG, "rfid init fail");
    }

    vTaskDelete(NULL);
}