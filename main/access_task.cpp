#include "tasks.h"
#include "blackboard.h"
#include "blackboard_c.h"
#include "myutil.h"
#define TAG __func__
bool finger_init()
{

    // serial_finger.begin(57600);
    // serial_finger.begin(9600);
    pinMode(FG_PW, OUTPUT);
    digitalWrite(FG_PW, HIGH);
    delay(500);
    // Serial1.begin(57600,134217756U,FG_TX,FG_RX,false,20000UL,112U);
    Serial1.begin(57600, 134217756U, FG_RX, FG_TX, false, 20000UL, 112U);
    xSemaFINGER = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaFINGER);
    delay(10);
    if (finger.verifyPassword())
    {
        ESP_LOGI(TAG, "Found fingerprint sensor!");
        // return true;
    }
    else
    {
        // Serial.println("Did not find fingerprint sensor :(");
        ESP_LOGE(TAG, "Did not find fingerprint sensor :(");
        return false;
    }

    Serial.println(F("Reading sensor parameters"));
    finger.getParameters();
    Serial.print(F("Status: 0x"));
    Serial.println(finger.status_reg, HEX);
    Serial.print(F("Sys ID: 0x"));
    Serial.println(finger.system_id, HEX);
    Serial.print(F("Capacity: "));
    Serial.println(finger.capacity);
    Serial.print(F("Security level: "));
    Serial.println(finger.security_level);
    Serial.print(F("Device address: "));
    Serial.println(finger.device_addr, HEX);
    Serial.print(F("Packet len: "));
    Serial.println(finger.packet_len);
    Serial.print(F("Baud rate: "));
    Serial.println(finger.baud_rate);
    Serial.println("************fingerInit over*****************\n\n\n");

    return true;
}
int getFingerprintID()
{
    if (xSemaphoreTake(xSemaFINGER, portMAX_DELAY) == pdTRUE)
    {
        // Serial.println("getFingerprintID");

        uint8_t p = finger.getImage();
        switch (p)
        {
        case FINGERPRINT_OK:
            Serial.println("Image taken");
            // screen.SetBuzzer(5);
            break;
        default:
            Serial.println("getFingerprintID Unknown error");
            xSemaphoreGive(xSemaFINGER);
            return 0;
        }

        // OK success!

        p = finger.image2Tz();
        switch (p)
        {
        case FINGERPRINT_OK:
            Serial.println("Image converted");
            break;
        default:
            // Serial.println("Unknown error");
            xSemaphoreGive(xSemaFINGER);
            return -1;
        }

        // OK converted!
        p = finger.fingerSearch();
        if (p == FINGERPRINT_OK)
        {
            Serial.println("Found a print match!");
        }
        else if (p == FINGERPRINT_PACKETRECIEVEERR)
        {
            Serial.println("Communication error");
            xSemaphoreGive(xSemaFINGER);
            return -1;
        }
        else if (p == FINGERPRINT_NOTFOUND)
        {
            Serial.println("Did not find a match");
            xSemaphoreGive(xSemaFINGER);
            return -1;
        }
        else
        {
            Serial.println("Unknown error");
            xSemaphoreGive(xSemaFINGER);
            return -1;
        }
    
        // found a match!
        Serial.print("Found ID #");
        Serial.print(finger.fingerID);
        String id=String(finger.fingerID);
        Serial.print(" with confidence of ");
        Serial.println(finger.confidence);
        xSemaphoreGive(xSemaFINGER);
        if(fg_cfg_local.getString(id.c_str())!=NULL)
        return finger.fingerID;
        else {finger.deleteModel(finger.fingerID); return -1;}
    }
    xSemaphoreGive(xSemaFINGER);
    return -1;
}

String read_RFID(int block)
{
    String result = "";
    byte status;

    for (int i = 0; i < 3; i++)
    {
        uint8_t buffer[16];
        memset(buffer, 0, sizeof(buffer));
        // status = mfrc522.MIFARE_Read(block * 4 + i, buffer, NULL);
        // if (status == MFRC522::STATUS_OK)
        // {
        //     Serial.print("Read block ");
        //     Serial.print(block + i);
        //     Serial.print(": ");
        //     Serial.println((char *)buffer);
        //     result += (char *)buffer; // 拼接子字符串
        // }
        // else
        // {
        //     Serial.print("Read block ");
        //     Serial.print(block + i);
        //     Serial.println(" failed");
        //     return "";
        // }
    }
    Serial.println(result);

    return result;
}

int write_RFID(int block, String str)
{
    uint8_t buffer_all[48];
    uint8_t buffer[4][16];
    memset(buffer_all, 0, sizeof(buffer_all));
    strncpy((char *)buffer_all, str.c_str(), 48);
    Serial.print("buffer_all:");
    Serial.println(strlen((const char *)buffer_all));
    int buffer_len = strlen((const char *)buffer_all);
    int count = 0;
    byte status;
    while (buffer_len > 0) // 只要还有剩余的字符就继续循环
    {
        memset(buffer[count], 0, sizeof(buffer[count]));                            // 初始化buffer[count]
        int len = buffer_len > 16 ? 16 : buffer_len;                                // 计算要复制的字符数
        strncpy((char *)buffer[count], (const char *)buffer_all + count * 16, len); // 复制子字符串
        buffer_len -= len;                                                          // 更新剩余字符数
        count++;                                                                    // 更新子字符串的计数器
    }
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block*4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("PCD_Authenticate() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    else
    {
        Serial.println("PCD_Authenticate() AAAA ok: ");
    }
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block * 4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("PCD_Authenticate() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        xSemaphoreGive(xSemaRFID);

        return -1;
    }
    else
    {
        Serial.println("PCD_Authenticate() BBBB ok: ");
    }
    // 在串口中打印出各个子字符串的长度
    for (int i = 0; i < count; i++)
    {
        if (count > 2)
        {
            break;
        }
        Serial.print("buffer[");
        Serial.print(i);
        Serial.print("]: ");
        Serial.println(strlen((const char *)buffer[i]));

        // 写入RFID
        // mfrc522.MIFARE_Write

        status = mfrc522.MIFARE_Write(4 * block + i, buffer[i], 16);
        if (status == MFRC522::STATUS_OK)
        {
            Serial.print("Write block [");
            Serial.print(4 * block + i);
            Serial.println("] success");
        }
        else
        {
            Serial.print("Write block ");
            Serial.print(4 * block + i);
            Serial.println(" failed");
            return 0;
        }
    }
    read_RFID(block);
    return 1;
}

int rc522_read()
{
    if (xSemaphoreTake(xSemaRFID, portMAX_DELAY) == pdTRUE)
    {
        // Serial.print(F("rc522_read......"));
        if (!mfrc522.PICC_IsNewCardPresent())
        {
            xSemaphoreGive(xSemaRFID);
            return -1;
        }

        // Verify if the NUID has been readed
        if (!mfrc522.PICC_ReadCardSerial())
        {
            xSemaphoreGive(xSemaRFID);
            return -1;
        }
        // modem_TTS("哎呦你干嘛");
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

        // if (mfrc522.uid.uidByte[0] != nuidPICC[0] ||
        //     mfrc522.uid.uidByte[1] != nuidPICC[1] ||
        //     mfrc522.uid.uidByte[2] != nuidPICC[2] ||
        //     mfrc522.uid.uidByte[3] != nuidPICC[3])
        if (1)
        {
            Serial.println(F("A new card has been detected."));

            // Store NUID into nuidPICC array
            for (byte i = 0; i < 4; i++)
            {
                nuidPICC[i] = mfrc522.uid.uidByte[i];
            }

            Serial.println(F("The NUID tag is:"));
            Serial.print(F("In hex: "));
            //  printHex((reinterpret_cast<int>(mfrc522.uid.uidByte)), mfrc522.uid.size);
            printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println();
            Serial.print(F("In dec: "));
            printDec(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println();
            //  write_RFID(3, "测试");
            // 验证密码
            // #define qwert 1
            // #ifdef qwert
            // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
            // delay(1000);
            char *handle_arg = "/spiffs/di.mp3";

            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(300);
            digitalWrite(audio_ctrl,LOW);
            if (1)
            {
                int block = RC_CM_NUM;
                //  int block = 40;
                byte status;
                String driverid,drivername,driverphone;

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
                        byte buffer1[18];
                        unsigned char len233 = sizeof(buffer1);
                        memset(buffer1, 0, sizeof(buffer1));
                        status = mfrc522.MIFARE_Read(RC_DRIVER_NAME, buffer1, &len233);       //设备名
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
                            // rfid_cfg_local.begin("writeTerminal");;
                            String cmp_id;
                            cmp_id = dev_cfg.getString("MACstr",MACstr);
                            ESP_LOGE(TAG, "管理员2 %s 长度 %d", cmp_id.c_str(), cmp_id.length());
                            // while(1)
                            // {
                            //    cmp_id = MACstr;
                            //    ESP_LOGE(TAG, "管理员2 %s 长度 %d", cmp_id.c_str(), cmp_id.length());
                            //    if(cmp_id.length()!=0)
                            //    break;
                            //    delay(100);
                            // }
                             buffer1[16] = '\0';
                            String myString = String((char *)buffer1);
                            ESP_LOGE(TAG, "认证成功cmp23--%s 长度：%d", myString.c_str(), myString.length());
                            drivername=myString;
                            if (cmp_id.indexOf(myString)==0&&myString.length()>0)
                            {
                                ESP_LOGE(TAG, "管理员刷卡开机");
                                pinMode(PIN_SB, INPUT);

                                if (dev_cfg.getInt("safe_belt_lock", 0) && digitalRead(PIN_SB) == sys_SB_TYPE)
                                 {
                                   if (digitalRead(PIN_SB) == sys_SB_TYPE)
                                   {
                                     char *handle_arg = "/spiffs/bsb.mp3";

                                        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                                        delay(1500);
                                        delay(1200);
                                        digitalWrite(audio_ctrl,LOW);
                                        rc522_init();
                                        xSemaphoreGive(xSemaRFID);
                                        xSemaphoreGive(xSemaFINGER);
                                        return -1;
                                      }
                                     }
                                else 
                                {
                                    if(dev_cfg.getInt("LOCK_ACTION", 0) != 1)
                                    {
                                         if(dev_cfg.getInt("card_verify",0)==1)
                                        {
                                            dev_cfg.putString("Two-factor","无#000000000000000000#无#1");
                                            dev_cfg.putString("phone_id","000000000000000000");
                                            blackboard.car_sta.deviceStatus = DEV_TWO;
                                            xSemaphoreGive(xSemaRFID);
                                            xSemaphoreGive(xSemaFINGER);
                                            return -1;
                                        }
                                        else {
                                           DateTime now = rtc.now();
                                    // String writeTerminalwork='000000000000000000'+'#'+'无'+'#'+'无';
                                           work_rec.putString(String(now.unixtime()).c_str(),"无#000000000000000000#无#1");
                                        }
                                     blackboard.car_sta.deviceStatus = DEV_AUTH;
                                    }
                                //    car_onoff(DEV_ON);
                                }
                            //     char *handle_arg = "/spiffs/unlock.mp3";
                            //    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                                xSemaphoreGive(xSemaRFID);
                                xSemaphoreGive(xSemaFINGER);
                                return 1;
                            }

                        }
                // byte buffer2[18];
                status = mfrc522.MIFARE_Read(block, buffer1, &len233);
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
                    buffer1[16]='\0';
                    String myString1 = String((char *)buffer1).substring(0,2);
                    driverphone=String((char *)buffer1).substring(2);
                    // String myString = myString1.substring(2);
                    ESP_LOGE(TAG, "认证成功2--%s", myString1.c_str());
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
                    buffer1[16]='\0';
                    String myString=String((char *)buffer1)+myString1;
                    ESP_LOGE(TAG, "%s", myString.c_str());
                    dev_cfg.putString("phone_id",myString.c_str()); //身份证号
                    driverid = myString;
                    myString=myString.substring(12);
                    if (rfid_cfg_local.isKey(myString.c_str()))
                    {
                        pinMode(PIN_SB, INPUT);

                        if (dev_cfg.getInt("safe_belt_lock", 0) && digitalRead(PIN_SB) == sys_SB_TYPE)
                                 {
                                   if (digitalRead(PIN_SB) == sys_SB_TYPE)
                                   {
                                     char *handle_arg = "/spiffs/bsb.mp3";

                                        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                                         delay(1500);
                                         delay(1200);
                                         digitalWrite(audio_ctrl,LOW);
                                         rc522_init();
                                         xSemaphoreGive(xSemaRFID);
                                         xSemaphoreGive(xSemaFINGER);
                                         return -1;
                                      }
                                }
                        else 
                        {
                           String str2json =drivername+"#"+driverid+"#"+driverphone+"#1";
                           ESP_LOGE(TAG, "认证成功233--%s", str2json.c_str());

                             if(dev_cfg.getInt("card_verify",0)==1)
                             {
                                      dev_cfg.putString("Two-factor",str2json);
                                    //   dev_cfg.putString("phone_id",);
                                            blackboard.car_sta.deviceStatus = DEV_TWO;
                                            xSemaphoreGive(xSemaRFID);
                                            xSemaphoreGive(xSemaFINGER);
                                            mfrc522.PICC_HaltA();
                                            mfrc522.PCD_StopCrypto1();
                                            Serial.println(dev_cfg.getString("Two-factor","str2json"));
                                            return -1;
                             }
                             DateTime now = rtc.now();
                             work_rec.putString(String(now.unixtime()).c_str(),str2json);
                             blackboard.car_sta.deviceStatus = DEV_AUTH;
                            //  ESP_LOGE(TAG, "drivername : %s", str2json);
                            // car_onoff(DEV_ON);
                        }
                        // StaticJsonDocument<128> doc_rfid;
                            xSemaphoreGive(xSemaRFID);
                            xSemaphoreGive(xSemaFINGER);
                        //    char  *handle_arg = "/spiffs/unlock.mp3";
                        //     esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                            return 1;
                      
                    }
                    // else
                    // {
                    //     memset(buffer1, 0, sizeof(buffer1));
                    //     status = mfrc522.MIFARE_Read(RC_DRIVER_NAME, buffer1, &len233);       //设备名
                    //     if (status != MFRC522::STATUS_OK)
                    //     {
                    //         Serial.print(F("Reading failed: "));
                    //         Serial.println(mfrc522.GetStatusCodeName(status));
                    //         rc522_init();
                    //         xSemaphoreGive(xSemaRFID);
                    //         return -1;
                    //     }
                    //     else
                    //     {
                    //         // rfid_cfg_local.begin("writeTerminal");
                    //         String cmp_id = MACstr;
                    //         ESP_LOGE(TAG, "管理员2 %s 长度 %d", cmp_id.c_str(), cmp_id.length());
                    //          buffer1[16] = '\0';
                    //         String myString = String((char *)buffer1);
                    //         ESP_LOGE(TAG, "认证成功cmp23--%s 长度：%d", myString.c_str(), myString.length());
                    //         if (cmp_id.indexOf(myString)==0)
                    //         {
                    //             ESP_LOGE(TAG, "管理员刷卡开机");
                    //             pinMode(PIN_SB, INPUT);

                    //             if (dev_cfg.getInt("safe_belt_lock", 0) && digitalRead(PIN_SB) == sys_SB_TYPE)
                    //              {
                    //                if (digitalRead(PIN_SB) == sys_SB_TYPE)
                    //                {
                    //                  char *handle_arg = "/spiffs/bsb.mp3";

                    //                     esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                    //                     delay(1500);
                    //                     rc522_init();
                    //                     xSemaphoreGive(xSemaRFID);
                    //                     xSemaphoreGive(xSemaFINGER);
                    //                     return -1;
                    //                   }
                    //                  }
                    //             else 
                    //             {
                    //                 DateTime now = rtc.now();
                    //                 // String writeTerminalwork='000000000000000000'+'#'+'无'+'#'+'无';
                    //                 work_rec.putString(String(now.unixtime()).c_str(),"无#000000000000000000#无");
                             
                    //               car_onoff(DEV_ON);
                    //             }
                    //         //     char *handle_arg = "/spiffs/unlock.mp3";
                    //         //    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                    //             xSemaphoreGive(xSemaRFID);
                    //             xSemaphoreGive(xSemaFINGER);
                    //             return 1;
                    //         }

                    //     }
                    // }
                }
                }
                // Serial.println(buffer1);

                // String myname="";
                // sprintf(myname,buffer1)
            }
            // #endif
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
    return -1;
}


void finger_dec(void *parm)
{
    char *handle_arg = "/spiffs/on.mp3";
    int RFID_FG=0;          //change RFID/finger
    if (!finger_init())
    {
        ESP_LOGE(TAG, "Failed to finger_init :(");
        vTaskDelete(NULL);
    }
    else
    {
        blackboard.err_code |= (1 << ERR_BIT_FG);
    }
    while (1)
    {
        if ((digitalRead(CAR_CON) == 0)&&(digitalRead(FG_TOUCH)==1))
        {
            RFID_FG=0; 
            Serial.println("finger_dec");
            // //modem_TTS("识别中");

            int res = getFingerprintID();

            char *handle_arg = "/spiffs/di.mp3";

            esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
            delay(500);
            digitalWrite(audio_ctrl,LOW);
            // int res=0;
            if (res == -1) 
            {
                // //modem_TTS("认证失败");
                delay(1500);
            }
            else if (res)
            {
                // if (dev_cfg.getInt("safe_belt_lock", 0))

                pinMode(PIN_SB, INPUT);

                if (dev_cfg.getInt("safe_belt_lock", 0) && digitalRead(PIN_SB) == sys_SB_TYPE)
                {
                    if (digitalRead(PIN_SB) == sys_SB_TYPE)
                    {
                        handle_arg = "/spiffs/bsb.mp3";

                        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                        delay(1500);
                        delay(1200);
                        digitalWrite(audio_ctrl,LOW);
                        continue;
                    }
                }

                if (true)
                {

                    String drivername = fg_cfg_local.getString(String(res).c_str(), "null");
                    if( (drivername != "null")&&(dev_cfg.getInt("LOCK_ACTION", 0) != 1))
                    {
                        DateTime now = rtc.now();
                        // work_rec.putULong(drivername.c_str(), now.unixtime());
                        work_rec.putString(String(now.unixtime()).c_str(),drivername+"#2");
                        ESP_LOGE(TAG, "drivername : %s", drivername.c_str());
                        blackboard.car_sta.deviceStatus = DEV_AUTH;
                        delay(1000);
                    }
                    car_onoff(DEV_ON);
                    // handle_arg = "/spiffs/unlock.mp3";
                    // esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
                    delay(1500);
                    // //updata_infobar("待机");
                    // updata_infobar(dev_cfg.getString("cmp_name", "朗云科技"));

                    // //modem_TTS("认证成功");
                    // DynamicJsonDocument json_finger_item(256);

                    // ESP_LOGE(TAG, "finger dect sucess"); // (String) returns time with specified format
                    // String str2json = fg_cfg_local.getString(String(res).c_str(), "{}");
                    // ESP_LOGE(TAG, "str2json:%s", str2json.c_str());

                    // if (blackboard.system.rc522TaskHandle != nullptr)
                    // {
                    //     vTaskSuspend(blackboard.system.rc522TaskHandle);
                    //     // blackboard.system.rc522TaskHandle = NULL;
                    // }
                    xSemaphoreGive(xSemaRFID);
                    xSemaphoreGive(xSemaFINGER);
                    // screen.SetTextValue(DS_MAIN_PAGE, 16, "驾驶员:");
                    rc522_init();
                   mfrc522.PICC_HaltA();
                    mfrc522.PCD_StopCrypto1();

                    // screen.SetTextValue(DS_MAIN_PAGE, 11, getJsonValue<String>(json_finger_item, "name", "(错误)"));
                    // blackboard.system.fingerTaskHandle = NULL;
                    // vTaskSuspend(NULL);
                    // car_onoff(DEV_OFF);
                }
                else
                {
                    ESP_LOGE(TAG, "finger dect fail"); // (String) returns time with specified format
                }
            }
        }
     
        // printf("1024");
        delay(10);
    }
}

void rc522_dec(void *params)
{

    Wire.begin(6, 7);
    rc522_init();
    delay(2000);
        ESP_LOGI(TAG, "初始化————:)");
        pinMode(CAR_CON, OUTPUT);
    while (1)
    {
    //     if (!rtc.begin())      //时钟初始化
    //       {
    //            Serial.println("Couldn't find RTC");
    //            Serial.flush();
    //       }
    //   else  Serial.println("find RTC");

         if ((digitalRead(CAR_CON) == 0)&&(rc522_read() == 1))
         {
            // car_onoff(1);
             car_onoff(DEV_ON);
            // if (blackboard.system.fingerTaskHandle != nullptr)
            // {
            //     vTaskSuspend(blackboard.system.aliyunTaskHandle);
            // }
            xSemaphoreGive(xSemaRFID);
            xSemaphoreGive(xSemaFINGER);
            // blackboard.system.rc522TaskHandle=NULL;
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            // delay(1000);
            // vTaskSuspend(NULL);
            Serial.println("刷卡成功");
        }
        delay(1000);
    }
}
void task_access_entry(void *params)
{
    pinMode(FG_TOUCH, INPUT);
    // Serial.println(finger.getTemplateCount());

    // ESP_LOGI(TAG, "指纹初始化————:)");
    xTaskCreatePinnedToCore(&finger_dec,                         // task
                            "ACCESS_TASk_NAME",                  // task name
                            4096,                                // stack size
                            NULL,                                // parameters
                            15,                                  // priority
                            &blackboard.system.fingerTaskHandle, // returned task handle
                            1                                    // pinned core
    );
    // if (blackboard.err_code & (1 << ERR_BIT_RC522))
    {
        // ESP_LOGI(TAG, "rc522初始化————:)");
        xTaskCreatePinnedToCore(&rc522_dec,                         // task
                                "rc522TaskHandle",                  // task name
                                4096,                               // stack size
                                NULL,                               // parameters
                                15,                                 // priority
                                &blackboard.system.rc522TaskHandle, // returned task handle
                                1                                   // pinned core
        );
    }
    vTaskDelete(NULL);

    while (true)
    {
        delay(1000);
    }
}
