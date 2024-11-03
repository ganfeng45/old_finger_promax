#include "blackboard.h"
#include "Arduino.h"
#define TAG "app_duaio"

extern "C"
{
    extern void task_audio_entry(void *params);
    extern int play_mp3(const char *filename);
}

int play_mp3_dec(String filename)
{
    digitalWrite(9,HIGH);
    return  play_mp3(filename.c_str());
}
void task_audio(void *parm)
{
    task_audio_entry(NULL);
    vTaskDelete(NULL);
}
