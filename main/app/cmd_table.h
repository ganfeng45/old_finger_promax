#include "Arduino.h"

enum AUTH_SC
{
    AUTH_OTH = 0,
    AUTH_FG,
    AUTH_RFID,
    AUTH_BLE,
    AUTH_NET
};

enum DEV_sta
{
    DEV_OFF = 0,
    DEV_ON = 1,
    DEV_AUTH = 2,
} ;

enum SB_SC
{
    SC_OTH = 0,
    SC_LINE,
    SC_BLE,
};
/* CMD */
typedef struct  
{
    String mp3_path;
} CMD_MP3_PLAYER;

typedef struct  
{
    String text;
    int index;
} CMD_TTS;
typedef struct  
{
    int finger_count;
} CMD_FG_CNT;
typedef struct 
{
    int auth_sc;
    String id = "1024";
    String name = "TWIND";
    String ps = "1024";
} CMD_AUTH;
typedef struct 
{
    String driver_name;
    String upload_url;
    String driver_num;
} CMD_IOT_CL_FG;

typedef struct 
{
    String topic;
    String msg;
} CMD_MQTT_EVENT;
typedef struct 
{
    String topic;
    String msg;
} CMD_MQTT_RAW_EVENT;

typedef struct 
{
    String url_item;
    String url_grp;
} CMD_UD_FINGERG;

typedef struct 
{
    String driver_name;
    String driver_num;
    String company_num;
    int authorize_power;
} CMD_WRITE_RFID;

 typedef  struct
{
    int sb_line_sta;
    int sb_sc = SC_LINE;
} CMD_SB_LINE;

typedef struct 
{
    String reply;
} CMD_BLE_REPLY;

typedef struct 
{
    int page_num;
    int page_size;
} CMD_FACE_LIST;

typedef struct 
{
    String reply;
    uint16_t index;
} CMD_ENROLL;