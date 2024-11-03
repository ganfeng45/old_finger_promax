#include "Arduino.h"
#include "I2C_BM8563.h"

// RTC BM8563 I2C port

// I2C pin definition for M5Stick & M5Stick Plus & M5Stack Core2
#define BM8563_I2C_SDA 4
#define BM8563_I2C_SCL 5

// I2C pin definition for M5Stack TimerCam
// #define BM8563_I2C_SDA 12
// #define BM8563_I2C_SCL 14

I2C_BM8563 rtc_8563(I2C_BM8563_DEFAULT_ADDRESS, Wire);



void setup_rtc()
{
    // Init Serial
    // Serial.begin(115200);
    delay(50);

    // Init I2C
    Wire.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);

    // Init RTC
    rtc_8563.begin();
}

void loop_rtc()
{
    I2C_BM8563_DateTypeDef dateStruct;
    I2C_BM8563_TimeTypeDef timeStruct;

    // Get RTC
    rtc_8563.getDate(&dateStruct);
    rtc_8563.getTime(&timeStruct);

    // Print RTC
    // Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
    //               dateStruct.year,
    //               dateStruct.month,
    //               dateStruct.date,
    //               timeStruct.hours,
    //               timeStruct.minutes,
    //               timeStruct.seconds);
    ESP_LOGI("TAG", "%04d/%02d/%02d %02d:%02d:%02d",
             dateStruct.year,
             dateStruct.month,
             dateStruct.date,
             timeStruct.hours,
             timeStruct.minutes,
             timeStruct.seconds);

    // Wait
    delay(1000);
}
void task_rtc(void *parm)
{
    setup_rtc();
    while (true)
    {
        loop_rtc();
        delay(100);
    }
}
