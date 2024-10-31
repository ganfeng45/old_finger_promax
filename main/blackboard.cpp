
#include "blackboard.h"

Blackboard blackboard;
SemaphoreHandle_t xSemaFINGER = NULL; // 创建信号量Handler
SemaphoreHandle_t xSemaRFID = NULL; // 创建信号量Handler

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
/* rc522 */
byte nuidPICC[4] = {0};
MFRC522::MIFARE_Key key;
MFRC522 mfrc522(RC_ARD, RC_RST_PIN); // Create MFRC522 instance.
String uuid="";
DynamicJsonDocument doc_heart(1024);

int isr_count = 0;
void blackboard_init(void) {}
/* wifi */
// WiFiMulti wifiMulti;
/* nvs */
Preferences rfid_cfg_local;
Preferences rfid_temp;
Preferences fg_cfg_local;
Preferences finger_temp;


Preferences dev_cfg;
Preferences work_rec;
RTC_PCF8563 rtc;
// RTC_Millis rtc;



/* default */
volatile float sys_POWER_LIMIT;
volatile int sys_POWER_LIMIT_TIME;
volatile int sys_SPD_CALI = 0;
volatile int sys_SPD_SP;
volatile int sys_SPD_LIMIT_PRE;
volatile int sys_SPD_LIMIT;
volatile int sys_SPEED_LIMIT_AFT;
volatile int sys_SPD_LIMIT_TIME;
volatile int sys_SB_LIMIT_TIME;
volatile int sys_SB_LOCK_TIME;
volatile int sys_SB_TYPE;
volatile int sys_card_verify;

String infobar_str;

/* md5 */
// MD5Builder md5;


/* event loop */
// esp_event_loop_handle_t bk_mqtt_handler;
// esp_event_base_t bk_BASE = "BLACKBOARD";


