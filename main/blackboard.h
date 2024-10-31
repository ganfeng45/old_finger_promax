
#ifndef BLACKBOARD_H_
#define BLACKBOARD_H_

#include <stdio.h>
#include "DEV_PIN.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ArduinoJson.h"
#include "esp_event.h"
#include <Wire.h>
// #include "sqlite3.h"
// #include <FS.h>
// #include "SPIFFS.h"
#include "Adafruit_Fingerprint.h"

// #include "Update.h"
/* cjson */
#include "cJSON.h"
/* wifi */
// #include <WiFi.h>
// #include <WiFiMulti.h>
// extern WiFiMulti wifiMulti;
/* nvs */
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <Preferences.h>
extern Preferences fg_cfg_local;
extern Preferences finger_temp;
extern Preferences rfid_cfg_local;
extern Preferences rfid_temp;
extern Preferences dev_cfg;
extern Preferences work_rec;

/* md5 */
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_image_format.h"
// #include "md5.h"
#include "mbedtls/md5.h"
// #include "esp_ota_ops.h"
#include "MD5Builder.h"
#include "MFRC522_I2C.h"
/* hhh */


// extern sqlite3 *db_cfg;
/* other */
#include <math.h>
/* event loop */
#include "esp_event.h"

// extern esp_event_loop_handle_t bk_mqtt_handler;
// extern esp_event_base_t bk_BASE;
// extern SoftwareSerial serial_finger;
extern Adafruit_Fingerprint finger;
extern SemaphoreHandle_t xSemaFINGER; // 创建信号量Handler
extern SemaphoreHandle_t xSemaRFID;   // 创建信号量Handler

/* rc522 */
extern byte  nuidPICC[4];
extern MFRC522::MIFARE_Key key;
extern MFRC522 mfrc522;
extern char MACstr[20];
extern String uuid;


// extern WiFiClient wifi_client_mqtt;
// extern WiFiClient wifi_client_http;

/* rtc */
#include "RTClib.h"

extern RTC_PCF8563 rtc;
// extern RTC_Millis rtc;


/* speed */
// int isr_count;
/* config */
// extern String dev_sn;

extern DynamicJsonDocument doc;
extern DynamicJsonDocument doc_finger;
extern DynamicJsonDocument doc_work;
extern DynamicJsonDocument doc_heart;
extern QueueHandle_t xQueue_alarm;
extern QueueHandle_t xQueue_alarm;
/* defau */
extern volatile float sys_POWER_LIMIT;
extern volatile int sys_POWER_LIMIT_TIME;
extern volatile int sys_SPD_CALI;
extern volatile int sys_SPEED_LIMIT_AFT;
extern volatile int sys_SPD_SP;
extern volatile int sys_SPD_LIMIT;
extern volatile int sys_SPD_LIMIT_TIME;
extern volatile int sys_SB_LIMIT_TIME;
extern volatile int sys_SPD_LIMIT_PRE;
extern volatile int sys_SB_LOCK_TIME;
extern volatile int sys_SB_TYPE;
extern volatile int sys_card_verify;

typedef struct CarStaData
{
    DEV_sta deviceStatus;
    long on_timestamp;
    long off_timestamp;
    uint8_t finger_sta;
    uint8_t rfid_sta;
} CarStaData;

typedef struct SensorTaskData
{
    float temperature;
    float humidity;
    float batteryPercent;
    float pressure;
} SensorTaskData;

typedef struct SystemData
{
    uint8_t bootCount;
    TaskHandle_t fingerTaskHandle;
    TaskHandle_t rc522TaskHandle;
    TaskHandle_t aliyunTaskHandle;
    TaskHandle_t screenTaskHandle;
    TaskHandle_t gpsTaskHandle;
    TaskHandle_t sensorTaskHandle;
    TaskHandle_t adcTaskHandle;
    TaskHandle_t sbTaskHandle;
    TaskHandle_t spdTaskHandle;
    TaskHandle_t acceTaskHandle;
    TaskHandle_t accessTaskHandle;
} SystemData;
typedef struct fence
{
    double fence_Latitude;
    double fence_Longitude;
    int fence_Type = 0;
    int fence_Weight;
    int fence_Speed;
    int fence_Radius;

} Fence;
typedef struct Blackboard
{
    SystemData system;
    uint32_t err_code = 0;
    // SensorTaskData sensors;
    esp_event_loop_handle_t loop_handler;
    volatile int isr_count = 0;
    int up_pau = 0;
    uint8_t max_speed = 0;
    // uint8_t car_sta=DEV_ON;
    CarStaData car_sta;
    esp_sleep_wakeup_cause_t wakeup_reason;
    int acce_region = 0;
    int8_t car_speed = 0;
    int8_t car_speed_ble = 0;
    int8_t car_speed_line = 0;
    Fence fence_cfg;
    bool gps_net = true;
    // uint8_t max_speed = -1;
} Blackboard;

// extern Fence
extern Blackboard blackboard;

void blackboard_init(void);

String md5str(String str, int len);

#endif
