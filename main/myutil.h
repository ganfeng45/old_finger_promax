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
std::string arduinoStringToStdString(const String &arduinoString);