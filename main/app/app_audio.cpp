#include "blackboard.h"
#include "util/myutil.h"
#include "Arduino.h"
#define TAG "app_duaio"

extern "C"
{
    extern void task_audio_entry(void *params);
    extern int play_mp3(const char *filename);
}

int play_mp3_dec(String filename)
{
    digitalWrite(9, HIGH);
    return play_mp3(filename.c_str());
}
void task_audio(void *parm)
{
    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);

    task_audio_entry(NULL);
    vTaskDelete(NULL);
}
