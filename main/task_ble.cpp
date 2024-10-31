#include "tasks.h"
#include "blackboard.h"
#include "blackboard_c.h"
#include "DEV_PIN.h"
#include "myutil.h"
#include <stdio.h>
#include "SPIFFS.h"
// #include <Update.h>
// #include "mbedtls/md5.h" // MD5库

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "blackboard.h"
#include "esp_bt_device.h"

#define TAG "app_test"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
// static CMD_MP3_PLAYER mp3_ble_test;
SemaphoreHandle_t xSema_BLE = NULL;
String rxValue;
#define SERVICE_UUID "0000FFE0-0000-1000-8000-00805F9B34FB" // UART service UUID
#define CHARACTERISTIC_UUID_RX "0000FFE1-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        ESP_LOGD(TAG, "onConnect");

        // facdID_ctl(false);
        // deviceConnected = true;
        // mp3_ble_test.mp3_path = "/spiffs/connect.mp3";
        // broker.publish("mp3_player", &mp3_ble_test);
        // char *handle_arg = "/spiffs/link.mp3";
        // esp_event_post_to(bk_mqtt_handler, "audio", 1, handle_arg, strlen(handle_arg) + 1, portMAX_DELAY);
        delay(1000);
    };

    void onDisconnect(BLEServer *pServer)
    {
        ESP_LOGD(TAG, "onDisconnect");

        // facdID_ctl(true);
        // deviceConnected = false;
        // mp3_ble_test.mp3_path = "/spiffs/disconnect.mp3";
        // broker.publish("mp3_player", &mp3_ble_test);
    }
};
void parseDataCmd(String hexValue)
{

}
void task_cmd(void *pvParameters)
{
    // ESP_LOGD(TAG, "Received Value: %s", hexValue.c_str());
    if (xSemaphoreTake(xSema_BLE, 0) == pdTRUE)
    {
        ESP_LOGD(TAG, "task_cmd");
        String hexValue;
        for (size_t i = 0; i < rxValue.length(); i++)
        {
            char buf[5];
            sprintf(buf, " %02X", (unsigned char)rxValue[i]);
            hexValue += String(buf);
        }

        parseDataCmd(rxValue);
        xSemaphoreGive(xSema_BLE);
    }
    else
    {
        ESP_LOGW(TAG, "parseDataCmd task busy");
    }
    vTaskDelete(NULL);
}
class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        rxValue = stdStringToArduinoString(pCharacteristic->getValue());
        xTaskCreatePinnedToCore(&task_cmd,  // task
                                "task_cmd", // task name
                                4096,       // stack size
                                NULL,       // parameters
                                10,         // priority
                                NULL,       // returned task handle
                                0           // pinned core
        );
    }
};

void ble_task_setup()
{
    xSema_BLE = xSemaphoreCreateBinary();
    xSemaphoreGive(xSema_BLE);
    String dev_name = "demo";
    if (dev_cfg.getString("DEVICE_NAME", "001") == "001")
    {
        uint64_t chipId = ESP.getEfuseMac();
        dev_name = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
        dev_name = dev_name.substring(0, 6);
    }
    else
    {
        dev_name = dev_cfg.getString("DEVICE_NAME", "001");
        if (dev_name.length() > 16)
        {
            // uint64_t chipId = ESP.getEfuseMac();
            // dev_name = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
            dev_name = dev_name.substring(10);
            ESP_LOGE(TAG, "MACstr:%s", dev_name.c_str());
        }
    }
    ESP_LOGE(TAG, "ble_name:%s", dev_name.c_str());

    BLEDevice::init(("LY_LOCK_" + dev_name).c_str());
    // BLEDevice::init("LY_LOCK_001");
    // const uint8_t *bleAddr = esp_bt_dev_get_address(); // 查询蓝牙的mac地址，务必要在蓝牙初始化后方可调用！
    // // sprintf(MACstr, "%02x:%02x:%02x:%02x:%02x:%02x\n", bleAddr[0], bleAddr[1], bleAddr[2], bleAddr[3], bleAddr[4], bleAddr[5]);
    // dev_cfg.putString("MACstr", MACstr);
    // ESP_LOGE(TAG, "MACstr:%s", MACstr);
    // 状态服务
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    // pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    // pTxCharacteristic->addDescriptor(new BLE2902());

    pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    pRxCharacteristic->addDescriptor(new BLE2902());

    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisingData;
    String shortuid = "FFE0";
    advertisingData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advertisingData.setPartialServices(BLEUUID(arduinoStringToStdString(shortuid)));
    // BLEAddress addr = BLEDevice::getAddress();
    String bleAddress = stdStringToArduinoString(BLEDevice::getAddress().toString());
    ESP_LOGD(TAG, "bleAddress:%s", bleAddress.c_str());
    esp_bd_addr_t *addr = BLEDevice::getAddress().getNative();
    char cdata[2];
    cdata[0] = 9;
    cdata[1] = 0xFF; // 0x02

    char mac_data[8];
    mac_data[0] = 0x58;
    mac_data[1] = 0x44;

    bleAddress.replace(":", "");

    for (int i = 0; i < 6; i++)
    {
        String byteString = bleAddress.substring(i * 2, i * 2 + 2);
        mac_data[2 + i] = strtol(byteString.c_str(), NULL, 16);
    }

    String(mac_data, 8);
    advertisingData.addData(arduinoStringToStdString(String(cdata, 2) + String(mac_data, 8)));
    String adv_data = stdStringToArduinoString(advertisingData.getPayload());
    ESP_LOGE(TAG, "adv_data:%s", adv_data.c_str());

    pAdvertising->setAdvertisementData(advertisingData);
    pAdvertising->start();

    Serial.println("Waiting for a client connection to notify...");
}
void ble_task_loop()
{

    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

void speed_ble_entry(void *params)
{
    delay(1000 * 2);

    ESP_LOGE(TAG, "speed_ble_entry");

    ble_task_setup();
    while (1)
    {
        // rfid_cfg_local.getString("21199412300221", "doc2str_iic");
        ble_task_loop();
        delay(400);
    }
}
