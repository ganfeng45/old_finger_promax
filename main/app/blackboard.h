#pragma once
#include "util/PubSubBroker.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include "cmd_table.h"
#include "RTClib.h"
#include "FM24CLXX.h"
#include <ESP32Time.h>
#define DEV_FG (1)
extern Broker broker;

/* nvs */
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <Preferences.h>
extern Preferences fg_cfg_local;
extern Preferences fc_cfg_local;
extern Preferences rfid_cfg_local;
extern Preferences dev_cfg;
extern Preferences work_rec;

 typedef struct 
{
  char dev_name[50];
  char dev_sc[50];
  char dev_pc[50];
} MQTT_SC;
typedef struct 
{
  DEV_sta deviceStatus = DEV_ON;
  esp_sleep_wakeup_cause_t wakeup_reason;
  uint32_t on_timestamp;
  long off_timestamp;
  uint8_t finger_sta;
  long move_duration_second;
  long carry_duration_second;
  long free_duration_second;
  uint8_t rfid_sta;
  float distance;
} CarStaData;
typedef struct 
{
  float sys_POWER_LIMIT;
  int sys_POWER_LIMIT_TIME;
  int sys_SPD_CALI;
  int sys_SPD_SP;
  int sys_SPD_LIMIT_PRE;
  int sys_SPD_LIMIT;
  int sys_SPEED_LIMIT_AFT;
  int sys_SPD_LIMIT_TIME;
  int sys_SB_LIMIT_TIME;
  int sys_SB_LOCK_TIME;
  int sys_SB_TYPE;
  int sys_WHEEL_DIAMETER;
  int sys_SPD_ORIGIN;
  int gps_hdop_v;
  int sys_BleWeight;
  String cmp_name;
  String SYS_BLE_MAC;

} Sys_parm;

typedef struct 
{
  MQTT_SC mqtt_sc;
  Sys_parm sys_parm;
  CarStaData car_sta;
  uint32_t err_code = 0;
  uint16_t face_auth_id=0;


} Blackboard;
extern Blackboard blackboard;
extern RTC_PCF8563 rtc;
extern FM24CLXX memory;
extern RTC_PCF8563 rtc;
extern ESP32Time esp32Time;


extern size_t countNonZeroElements(const uint8_t *array, size_t size);

