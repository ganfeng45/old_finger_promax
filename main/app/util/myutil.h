#pragma once
#include "blackboard.h"
int dev_init();
// void printHex(int num, int precision);
String bufferToHex(byte *buffer, byte bufferSize);
void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);
uint8_t getFingerprintEnroll();
void printAllNVSKeys(const char *namespace_name);
int splitStringToInt(const String &input, char delimiter, int numbers[]);
void car_onoff(uint8_t onoff);
void listFiles(const char *dirname);
void sys_deafuat();
void DEV_I2C_ScanBus(void);
bool rc522_init();
int rc522_read();
bool finger_init();

String md5str(String str, int len);

size_t countNonZeroElements(const uint8_t *array, size_t size);
String stdStringToArduinoString(const std::string &stdString);
std::string arduinoStringToStdString(const String &arduinoString);

/* 特殊 */
 int play_mp3_dec(String filename);
