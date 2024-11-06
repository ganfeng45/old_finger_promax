#include "blackboard.h"
// #include "blackboard_c.h"
#include "Arduino.h"
#include "MD5Builder.h"

// #include "tasks.h"
// #define TAG __FILE__
#define TAG __func__

String md5str(String str, int len)
{
  /* 6CD3556DEBDA54BCA6 */
  MD5Builder md5;

  md5.begin();
  md5.add(str);
  md5.calculate();
  String fullMD5 = md5.toString();
  return fullMD5.substring(fullMD5.length() - len);
}

size_t countNonZeroElements(const uint8_t *array, size_t size)
{
  size_t count = 0;
  for (size_t i = size; i > 0; i--)
  {
    if (array[i] != 0)
    {
      return i + 1;
      break;
    }
  }
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

// bool getFinger = false;
// uint8_t getFingerprintEnroll()
// {
//   int p = -1;
//   uint8_t count = 4; /* 录入次数 只要改这一个参数 */
//   int num = 0;
//   uint8_t id = finger.ReadIndexTable();
//   finger.auto_enroll(id, count, 0);
//   uint8_t data[32];
//   char *handle_arg = "/spiffs/press.mp3";
//   esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
//   delay(1200);
//   digitalWrite(audio_ctrl, LOW);
//   unsigned long Enroll_time = millis();
//   getFinger = true;
//   while (1)
//   {
//     delay(1 * 100);
//     finger.auto_enroll_replay(data);
//     if (!data[0])
//     {
//       // if( (millis() - Enroll_time > 60000))
//       if (num > 10)
//       {
//         // Serial.printf("millis is");
//         // Serial.println(millis());
//         // Serial.printf("Enroll_time is");
//         // Serial.println(Enroll_time);
//         //  Serial.println(num);
//         num = 0;
//         return 0;
//       }
//       if (data[2] == 0xf2)
//       {
//         return id;
//       }

//       if (data[1] != 0x01 && data[1] < count + 1 && (!(digitalRead(FG_TOUCH))))
//       {
//         handle_arg = "/spiffs/press.mp3";
//         esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
//         delay(1200);
//         digitalWrite(audio_ctrl, LOW);
//       }
//       else
//       {
//         num++;
//         while (digitalRead(FG_TOUCH))
//         {
//           handle_arg = "/spiffs/remove.mp3";
//           esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
//           delay(1200);
//           digitalWrite(audio_ctrl, LOW);
//         }
//       }
//     }
//     else
//     {
//       return 0;
//     }
//   }

//   return 0;
// }
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

// std::string arduinoStringToStdString(const String &arduinoString)
// {
//   char buffer[arduinoString.length() + 1];
//   arduinoString.toCharArray(buffer, arduinoString.length() + 1);
//   return std::string(buffer);
// }

std::string arduinoStringToStdString(const String &arduinoString) {
  return std::string(arduinoString.c_str());
}

// String stdStringToArduinoString(const std::string &stdString)
// {
//   return String(stdString.c_str());
//   // return String(stdString);
// }
String stdStringToArduinoString(const std::string &stdString)
{
  String result;
  for (char c : stdString)
  {
    result += c; // 直接将每个字符添加到 result 中，不做额外转换
  }
  return result;
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