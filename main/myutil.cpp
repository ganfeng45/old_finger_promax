#include "myutil.h"
#include "blackboard.h"
#include "blackboard_c.h"
#include "Arduino.h"
#include "tasks.h"
// #define TAG __FILE__
#define TAG __func__
MD5Builder md5;

String md5str(String str, int len)
{
  /* 6CD3556DEBDA54BCA6 */
  md5.begin();
  md5.add(str);
  md5.calculate();
  String fullMD5 = md5.toString();
  return fullMD5.substring(fullMD5.length() - len);
}

void sys_deafuat()
{
  dev_cfg.begin("dev_cfg", false);
  work_rec.begin("work_rec", false);
  fg_cfg_local.begin("finger_cfg", false);
  finger_temp.begin("finger_tmp", false);
  rfid_cfg_local.begin("rfid_cfg", false);
  rfid_temp.begin("rfid_tmp", false);
  sys_SB_LIMIT_TIME = dev_cfg.getInt("safe_belt_tm", 2);
  sys_SB_LOCK_TIME = dev_cfg.getInt("SB_LOCK", 655360);
  sys_SB_TYPE = dev_cfg.getInt("SB_TYPE", 0);
  sys_card_verify = dev_cfg.getInt("card_verify", 0);
  dev_cfg.getInt("fg_num", 100);

  // work_rec.freeEntries();
  // work_rec.freeEntries();

  // work_rec.clear();
}
bool rc522_init()
{
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  // mfrc522.PCD_Reset();
  return mfrc522.PCD_Init();
}

void car_onoff(uint8_t onoff)
{
  pinMode(CAR_CON, OUTPUT);
  int j_on = 1;
  int j_off = 0;
  // if (getJsonValue<int>(doc, "NC", 1))
  if (dev_cfg.getInt("NC", 0))
  {
    j_on = 0;
    j_off = 1;
  }
  ESP_LOGE(TAG, "onoff:%d  NC: %d  LOCK_ACTION:%d", onoff, dev_cfg.getInt("NC", 1), dev_cfg.getInt("LOCK_ACTION", 1));
  if (dev_cfg.getInt("LOCK_ACTION", 0) == 1)
  {
    char *handle_arg = "/spiffs/lock.mp3";
    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
    delay(2000);
    digitalWrite(audio_ctrl, LOW);
  }
  if (onoff && (dev_cfg.getInt("LOCK_ACTION", 0)) != 1)
  {
    // blackboard.car_sta.deviceStatus=DEV_AUTH;
    finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_BLUE, 0);
    ESP_LOGE(TAG, "Activate the relay: %d", j_on);
    digitalWrite(CAR_CON, j_on);
    char *handle_arg = "/spiffs/unlock.mp3";
    esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
    delay(2000);
    digitalWrite(audio_ctrl, LOW);
    digitalWrite(CAR_CON, j_off);
  }
  else
  {
    ESP_LOGE(TAG, "Deactivate the relay: %d", j_off);
    digitalWrite(CAR_CON, j_off);
  }
}
int dev_init()
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  xSemaRFID = xSemaphoreCreateBinary();
  xSemaphoreGive(xSemaRFID);
  Serial.println("init 123");
  Wire.begin(6, 7);
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  sys_deafuat();

  return 1;
}

// void printHex(int num, int precision)
// {
//   char tmp[16];
//   char format[128];

//   sprintf(format, "%%.%dX", precision);

//   sprintf(tmp, format, num);
//   memcpy(nuidPICC,tmp,16);
//   Serial.print(tmp);
// }
void printHex(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
    uuid += String(buffer[i], HEX);
    // Serial.println(uuid);
  }
}
void printDec(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
bool getFinger = false;
uint8_t getFingerprintEnroll()
{
  int p = -1;
  uint8_t count = 4; /* 录入次数 只要改这一个参数 */
  int num = 0;
  uint8_t id = finger.ReadIndexTable();
  finger.auto_enroll(id, count, 0);
  uint8_t data[32];
  char *handle_arg = "/spiffs/press.mp3";
  esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
  delay(1200);
  digitalWrite(audio_ctrl, LOW);
  unsigned long Enroll_time = millis();
  getFinger = true;
  while (1)
  {
    delay(1 * 100);
    finger.auto_enroll_replay(data);
    if (!data[0])
    {
      // if( (millis() - Enroll_time > 60000))
      if (num > 10)
      {
        // Serial.printf("millis is");
        // Serial.println(millis());
        // Serial.printf("Enroll_time is");
        // Serial.println(Enroll_time);
        //  Serial.println(num);
        num = 0;
        return 0;
      }
      if (data[2] == 0xf2)
      {
        return id;
      }

      if (data[1] != 0x01 && data[1] < count + 1 && (!(digitalRead(FG_TOUCH))))
      {
        handle_arg = "/spiffs/press.mp3";
        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        delay(1200);
        digitalWrite(audio_ctrl, LOW);
      }
      else
      {
        num++;
        while (digitalRead(FG_TOUCH))
        {
          handle_arg = "/spiffs/remove.mp3";
          esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
          delay(1200);
          digitalWrite(audio_ctrl, LOW);
        }
      }
    }
    else
    {
      return 0;
    }
  }

  return 0;
}
int splitStringToInt(const String &input, char delimiter, int numbers[])
{
  int numParts = 0; // Initialize the number of parts

  if (input.length() > 0)
  { // Check if inputString is not empty
    int start = 0;
    int end = input.indexOf(delimiter);

    while (end != -1 && numParts < 300)
    {
      String part = input.substring(start, end);
      numbers[numParts++] = part.toInt();
      start = end + 1;
      end = input.indexOf(delimiter, start);
    }

    // Add the last part after the last delimiter
    if (numParts < 300)
    {
      String lastPart = input.substring(start);
      numbers[numParts++] = lastPart.toInt();
    }
  }

  return numParts;
}

void printAllNVSKeys(const char *namespace_name)
{
  nvs_iterator_t it = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY);
  Preferences all_prt;
  all_prt.begin(namespace_name, true);
  ESP_LOGI(TAG, "++++++++++++++++NVS NAME:%s+++++++++++++", namespace_name);
  if (it == NULL)
  {
    ESP_LOGI(TAG, "%s:no KEY_VALUE", namespace_name);
  }
  while (it != NULL)
  {
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);
    it = nvs_entry_next(it);
    printf("key '%s', type '%d'\n", info.key, info.type);
    if (info.type == NVS_TYPE_STR)
    {
      Serial.println(all_prt.getString(info.key));
    }
    else if (info.type == NVS_TYPE_I32)
    {
      /* code */
      Serial.println(all_prt.getInt(info.key));
    }

    // Serial.println(preferences.getString("16845659906461"));
    // Serial.println(preferences.getString(info.key));
    // preferences.getString(info.key);
  };
  all_prt.end();
  ESP_LOGI(TAG, "+++++++++++++++NVS NAME:%s END+++++++++++++++++", namespace_name);
}

std::string arduinoStringToStdString(const String &arduinoString)
{
  char buffer[arduinoString.length() + 1];
  arduinoString.toCharArray(buffer, arduinoString.length() + 1);
  return std::string(buffer);
}
String stdStringToArduinoString(const std::string &stdString)
{
  return String(stdString.c_str());
}

/* void listFiles(const char *dirname)
{
  Serial.println("Listing files in SPIFFS:");

  File root = SPIFFS.open(dirname);

  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }

  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  Serial.println("File listing completed!");
} */

void DEV_I2C_ScanBus(void)
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address); // 从指定的地址开始向I2C从设备进行传输
    error = Wire.endTransmission();  // 停止与从机的数据传输
    /*
    * error返回结果：
    * 0: 成功
   1: 数据量超过传送缓存容纳限制
   2: 传送地址时收到 NACK
   3: 传送数据时收到 NACK
   4: 其它错误
    */
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);
}