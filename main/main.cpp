#include "Arduino.h"

#include "blackboard.h"
#include "util/myutil.h"
#include <WiFi.h>

#define TAG "MAIN"
extern "C"
{
    extern void i2s_example_pdm_tx_task(void *args);
    extern void app_main_test(void);
}
extern void task_twai_entry(void *params);
extern void task_audio(void *parm);
extern void task_face(void *parm);
extern void task_ble(void *parm);
extern void app_test(void *parm);
extern void task_8563(void *parm);
extern void task_finger(void *parm);
extern void task_ota(void *parm);
extern void task_rfid(void *parm);

int print_all(String nms)
{
    int count = 0;
    //    = NULL;
    nvs_iterator_t it = nvs_entry_find("nvs", nms.c_str(), NVS_TYPE_ANY);
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
        ESP_LOGI(TAG, "key '%s', type '%d' ", info.key, info.type);
        it = nvs_entry_next(it);
        if (nms == "work_rec" && count > 255)
        {
            work_rec.remove(info.key);
        }
        count++;
        // it = nvs_release_iterator(it);
    }
    return count;
}

void sys_deafuat()
{

    Wire.begin(4, 5);

    if (1)
    {
        ESP_LOGI(TAG, "----开机初始化参数");
        // Initialize NVS
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);

        dev_cfg.begin("dev_cfg", false);
        work_rec.begin("work_rec", false);
        fg_cfg_local.begin("finger_cfg", false);
        fc_cfg_local.begin("face_cfg", false);
        rfid_cfg_local.begin("rfid_cfg", false);
        nvs_stats_t nvs_stats;
        err = nvs_get_stats(NULL, &nvs_stats);
        if (err)
        {
            ESP_LOGE(TAG, "Failed to get nvs statistics");
        }
        else
        {
            ESP_LOGI(TAG, "nvs_sta used_entry_count:%zu free_entries:%zu", nvs_stats.used_entries, nvs_stats.free_entries);
        }
#ifdef DEV_FG
        // ESP_LOGI(TAG, "fg_cfg_local.freeEntries():%zu", fg_cfg_local.freeEntries());
        // work_rec.clear();
        ESP_LOGI(TAG, " freeEntries():%zu 工作记录:%d 指纹记录:%d", fg_cfg_local.freeEntries(), print_all("work_rec"), print_all("finger_cfg"));
#elif
        // ESP_LOGI(TAG, "fg_cfg_local.freeEntries():%zu", fc_cfg_local.freeEntries());
        // work_rec.clear();
        ESP_LOGI(TAG, "freeEntries():%zu 工作记录:%d 人脸记录:%d", fc_cfg_local.freeEntries(), print_all("work_rec"), print_all("face_cfg"));

#endif

        // print_all("work_rec");
        // print_all("face_cfg");
    }
}
void setup()
{
    sys_deafuat();
    xTaskCreatePinnedToCore(task_twai_entry, "task_twai_entry", 1024 * 2, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_8563, "task_8563", 1024 * 3, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_audio, "task_audio", 1024 * 2, NULL, 19, NULL, 0);

#ifdef DEV_FG
    Serial.begin(57600, 134217756U, 20, 21, false, 20000UL, (uint8_t)112U);

    delay(2 * 1000);
    xTaskCreatePinnedToCore(task_rfid, "task_rfid", 1024 * 3, NULL, 19, NULL, 0);
    xTaskCreatePinnedToCore(task_finger, "task_finger", 1024 * 4, NULL, 5, NULL, 0);
#else
    Serial.begin(115200, 134217756U, 20, 21, false, 20000UL, (uint8_t)112U);

    pinMode(GPIO_NUM_2, OUTPUT);
    digitalWrite(GPIO_NUM_2, HIGH);
    delay(3 * 1000);
    xTaskCreatePinnedToCore(task_face, "task_face", 4096 * 1, NULL, 5, NULL, 0);
#endif

    WiFi.begin("myssid", "12345678");
    delay(5 * 1000);

    if (WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGE(TAG, "使用WIFI");
        play_mp3_dec("/spiffs/ota.mp3");

        xTaskCreatePinnedToCore(task_ota, "task_ota", 4096 * 1, NULL, 19, NULL, 0);
        // app_ota();
    }
    else
    {
        xTaskCreatePinnedToCore(task_ble, "task_ble", 4096 * 2, NULL, 5, NULL, 0);
    }
}

void loop()
{
}
extern "C" int app_main()
{
    setup();
    while (true)
    {
        delay(1000);
    }

    return 0;
}