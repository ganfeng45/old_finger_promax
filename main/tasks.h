
#ifndef TASKS_H_
#define TASKS_H_

#define BLE_TASK_NAME    "ble_task"
#define ALIYUN_TASK_NAME    "aliyun_task"
#define SCREEN_TASK_NAME    "screen_task"
#define GPS_TASK_NAME    "gps_task"
#define ACCESS_TASk_NAME "access_task"
#ifdef __cplusplus
extern "C" {
#endif
// void task_ble_entry(void* params);

void task_access_entry(void* params);
void task_aliyun_entry(void* params);
void task_screen_entry(void* params);
void task_gps_entry(void* params);
void speed_entry(void *params);
void task_adc_entry(void *params);
void task_sb_entry(void *params);
void speed_ble_entry(void *params);
void task_acce_entry(void *params);
void task_audio_entry(void *params);
void task_led_entry(void *params);
#ifdef __cplusplus
}
#endif

#endif
