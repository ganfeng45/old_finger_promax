#include"blackboard.h"

Broker broker;
/* nvs */
Preferences rfid_cfg_local;
Preferences fg_cfg_local;
Preferences fc_cfg_local;
Preferences dev_cfg;
Preferences work_rec;
Blackboard blackboard;
FM24CLXX memory(0x56); 
ESP32Time esp32Time;

