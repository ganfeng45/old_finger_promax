
#pragma once
/* FINGER */
#define FG_RX (GPIO_NUM_0)
#define FG_TX (GPIO_NUM_1)
// #define serial_finger
#define FG_TOUCH (GPIO_NUM_4)
#define FG_PW (GPIO_NUM_10)


/*IIC ID_Care*/
// #define IIC_Data (GPIO_NUM_0)
// #define IIC_SCL (GPIO_NUM_1)
#define RC_RST_PIN (GPIO_NUM_19)
#define RC_ARD 0X28
#define RC_DRIVER_NAME 16
#define RC_DRIVER_NUM 17
#define RC_DRIVER_ID 12
#define RC_CM_NUM 18
/***********audio*************/
#define audio_ctrl (GPIO_NUM_2)
/* power_control */
#define SENSOR_PWR 10
#define PIN_SB (GPIO_NUM_2)
/* jidinqi */
#define CAR_CON (GPIO_NUM_5)
#define ALARM_CON 48
/* config */
#define SYS_CFG_PATH "/config.json"
#define FG_CFG_PATH "/finger_config.json"

/* error bit */
#define ERR_BIT_RC522 1  
#define ERR_BIT_FG 2  
#define ERR_BIT_ACC 3  
#define ERR_BIT_CAT 4
#define ERR_BIT_SC 5
#define ERR_BIT_WF 6

#define ERR_BIT_SB 7  
#define ERR_BIT_SD 8  
#define ERR_BIT_ADC 9 
#define ERR_BIT_LOC 10
#define ERR_BIT_FENCE 11

#define PIN_ESP_STA 45

typedef enum {
    DEV_OFF = 0,
    DEV_ON =  1,
    DEV_AUTH = 2,
    DEV_TWO =3,
} DEV_sta;




