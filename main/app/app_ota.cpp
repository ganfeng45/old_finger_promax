#include "Arduino.h"
#include <WiFi.h>

void task_ota(void *parm)
{
    WiFi.begin("ssid", "12345678");
    delay(5*1000);
    vTaskDelete(NULL);
}