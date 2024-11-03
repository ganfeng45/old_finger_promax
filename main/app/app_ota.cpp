#include "Arduino.h"
#include <WiFi.h>

void task_ota(void *parm)
{
    WiFi.begin("ssid", "12345678");
    vTaskDelete(NULL);
}