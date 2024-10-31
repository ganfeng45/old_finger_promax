#include <stdio.h>
#include "Arduino.h"
#include "myutil.h"
#include "tasks.h"
#include "blackboard_c.h"
#include"WiFi.h"
#define TAG __func__
// char *handle_arg = "/spiffs/enrool.mp3";

// RTC_PCF8563 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    initArduino();
    WiFi.begin("11","12345678");
    Serial.begin(115200);
    // ESP_LOGE(__func__, "Current system time: "); // (String) returns time with specified format
    ESP_LOGI(TAG, "233  Compile time: %s %s", __DATE__, __TIME__);
    // while (true)
    // {
    //     delay(1000);
    // }
    dev_init();

    
    // DEV_I2C_ScanBus();
    // pinMode(4,OUTPUT);
    // pinMode(5,OUTPUT);
    // int count=0;
    // while (true)
    // {
    //     digitalWrite(4,count++%2);
    //     digitalWrite(5,count%2);
    //     delay(1000);
    // }
    

    ESP_LOGE("MAIN", "创建 Event Loop");
    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "night_market_task",
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 1024 * 5,
        .task_core_id = tskNO_AFFINITY,
    };
    esp_event_loop_create(&loop_args, &bk_mqtt_handler);

    xTaskCreatePinnedToCore(&task_access_entry,                  // task
                            ACCESS_TASk_NAME,                    // task name
                            4096,                                // stack size
                            NULL,                                // parameters
                            15,                                   // priority
                            &blackboard.system.accessTaskHandle, // returned task handle
                            1                                    // pinned core
    );
    xTaskCreatePinnedToCore(&speed_ble_entry, // task
                            "speed_entry",    // task name
                            4096,             // stack size
                            NULL,             // parameters
                            10,               // priority
                            NULL,             // returned task handle
                            0                 // pinned core
    );
    xTaskCreatePinnedToCore(&task_audio_entry,   // task
                            "speed_audio_entry", // task name
                            4096,                // stack size
                            NULL,                // parameters
                            18,                  // priority
                            NULL,                // returned task handle
                            0                    // pinned core
    );
    xTaskCreatePinnedToCore(&task_sb_entry,  // task
                            "task_sb_entry", // task name
                            4096,            // stack size
                            NULL,            // parameters
                            10,              // priority
                            NULL,            // returned task handle
                            0                // pinned core
    );
    // xTaskCreatePinnedToCore(&task_led_entry,  // task
    //                         "task_led_entry", // task name
    //                         4096,            // stack size
    //                         NULL,            // parameters
    //                         10,              // priority
    //                         NULL,            // returned task handle
    //                         0                // pinned core
    // );
    delay(2000);
    if (dev_cfg.getInt("LOCK_ACTION", 0) == 2)
    {
        car_onoff(DEV_ON);
        blackboard.car_sta.deviceStatus = DEV_AUTH;
        char *handle_arg = "/spiffs/free.mp3";
        esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        delay(1500);
            digitalWrite(audio_ctrl,LOW);
    //     if (blackboard.system.fingerTaskHandle != nullptr)
    //     {
    //         vTaskSuspend(blackboard.system.fingerTaskHandle); // 挂起任务
    //     }
    //     delay(1000);
    //     if (blackboard.system.rc522TaskHandle != nullptr)
    //     {
    //         vTaskSuspend(blackboard.system.rc522TaskHandle);
    //     }
    // xSemaphoreGive(xSemaRFID);
    // xSemaphoreGive(xSemaFINGER);
    // Wire.begin(6, 7);
    // rc522_init();
    // mfrc522.PICC_HaltA();
    // mfrc522.PCD_StopCrypto1();
    }

    int a = 0;

    while (false)
    {
          
        delay(1 * 1000);
        ESP.restart();
        // char *handle_arg = "/spiffs/enrool.mp3";

        // esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
    }
}

