#include "Arduino.h"
#include <WiFi.h>

#define TAG "app_ota"
bool bReboot = false;
extern "C"
{
    extern void app_ota(void);
}
#ifdef ARD_V2
#include "HttpsOTAUpdate.h"
static HttpsOTAStatus_t otastatus;

void HttpEvent(HttpEvent_t *event)
{
    switch (event->event_id)
    {
    case HTTP_EVENT_ERROR:
        Serial.println("Http Event Error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        Serial.println("Http Event On Connected");
        break;
    case HTTP_EVENT_HEADER_SENT:
        Serial.println("Http Event Header Sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        break;
    case HTTP_EVENT_ON_FINISH:
        Serial.println("Http Event On Finish");
        break;
    case HTTP_EVENT_DISCONNECTED:
        Serial.println("Http Event Disconnected");
        break;
    }
}
// #define  USE_ARD_OTA 1
#ifdef USE_ARD_OTA
#include <WiFiClient.h>
#include <Update.h>
#include <ArduinoHttpClient.h>
void splitURL(String url, String &ip, int &port, String &path)
{
    int doubleSlashIndex = url.indexOf("//");
    int colonIndex = url.indexOf(":", doubleSlashIndex + 2);
    int slashIndex = url.indexOf("/", colonIndex);

    ip = url.substring(doubleSlashIndex + 2, colonIndex);
    port = url.substring(colonIndex + 1, slashIndex).toInt();
    path = url.substring(slashIndex);
}

void printPercent(uint32_t readLength, uint32_t contentLength)
{
    if (contentLength != (uint32_t)-1)
    {
        // Serial.print("\r ");
        // Serial.print((100.0 * readLength) / contentLength);
        // // screen.SetTextValue(DS_MAIN_PAGE, 13, "OTA:" + String((100.0 * readLength) / contentLength) + "%");
        // Serial.print('%');
        ESP_LOGD(TAG, "printPercent %f", (100.0 * readLength) / contentLength);
    }
    else
    {
        // Serial.println(readLength);
    }
}

void ota_ser()
{
    // String ota_url = "http://www.on1on.cn:8888/down/UFRkCJIGMU89.bin";
    String ota_url = "http://www.on1on.cn:8101/dw/finger_c3";
    // String ota_url = "http://47.100.161.9:81/v1/FileController/getOtaFile?filePath=C%3A%5Cly-saas%5Cuploads%5Cbin%5Cota%5Cf430146.bin";
    String server;
    String resource;
    int port2;

    splitURL(ota_url, server, port2, resource);

    ESP_LOGI(TAG, "url:%s res:%s port:%d", server.c_str(), resource.c_str(), port2);

    WiFiClient wifi;
    HttpClient http(wifi, server, port2);
    if (WiFi.status() == WL_CONNECTED)
    {

        ESP_LOGE(TAG, "使用WIFI");
    }
    else
    {
        return;
    }
    // TinyGsmClient client3(modem, 3);

    uint32_t timeElapsed = millis();

    ESP_LOGD(TAG, "Waiting for response header");

    const uint32_t clientReadTimeout = 5000 * 10;
    uint32_t clientReadStartTime = millis();
    String headerBuffer;
    bool finishedHeader = false;
    uint32_t contentLength = 0;
    ESP_LOGD(TAG, "Connecting to %s", server.c_str());
    if (!http.connect(server.c_str(), port2))
    {
        ESP_LOGD(TAG, "连接失败");
        delay(10000);
        return;
    }
    ESP_LOGE(TAG, " success");
    http.print(String("GET ") + resource + " HTTP/1.0\r\n");
    http.print(String("Host: ") + server + "\r\n");
    http.print("Connection: close\r\n\r\n");
    while (!finishedHeader)
    {
        int nlPos;
        delay(10);

        if (http.available())
        {
            delay(10);

            clientReadStartTime = millis();
            while (http.available())
            {
                delay(5);
                char c = http.read();
                headerBuffer += c;
                if (headerBuffer.indexOf(F("\r\n")) >= 0)
                    break;
            }
        }
        else
        {
            if (millis() - clientReadStartTime > clientReadTimeout)
            {
                ESP_LOGE(TAG, ">>> Client Timeout !");
                break;
            }
        }

        nlPos = headerBuffer.indexOf(F("\r\n"));

        if (nlPos > 0)
        {
            headerBuffer.toLowerCase();
            if (headerBuffer.startsWith(F("content-length:")))
            {
                contentLength = headerBuffer.substring(headerBuffer.indexOf(':') + 1).toInt();
            }
            headerBuffer.remove(0, nlPos + 2);
        }
        else if (nlPos == 0)
        {
            finishedHeader = true;
        }
    }

    ESP_LOGE(TAG, "Content-length: %d\n", contentLength);
    if (Update.begin(contentLength))
    {
        ESP_LOGE(TAG, "Update begin ok");
    }
    else
    {
        ESP_LOGE(TAG, "Update begin not ok");

        while (true)
        {
            delay(1000);
        }
    }

    // The two cases which are not managed properly are as follows:
    // 1. The client doesn't provide data quickly enough to keep up with this
    // loop.
    // 2. If the client data is segmented in the middle of the 'Content-Length: '
    // header,
    //    then that header may be missed/damaged.
    //

    uint32_t readLength = 0;

    if (finishedHeader)
    {
        ESP_LOGE(TAG, "Reading response data");
        clientReadStartTime = millis();

        printPercent(readLength, contentLength);
        while (readLength < contentLength && http.connected() &&
               millis() - clientReadStartTime < clientReadTimeout)
        {
            while (http.available())
            {
                uint8_t c = http.read();
                Update.write(&c, 1);
                readLength++;
                if (readLength % (contentLength / 1104) == 0)
                {
                    printPercent(readLength, contentLength);
                }
                clientReadStartTime = millis();
                // printPercent(readLength, contentLength);
            }
        }
    }

    timeElapsed = millis() - timeElapsed;
    // Serial.println();

    if (Update.end())
    {
        ESP_LOGE(TAG, "OTA done!");
        if (Update.isFinished())
        {
            ESP_LOGI(TAG, "Update successfully completed. Rebooting.");
            bReboot = true;
        }
        else
        {
            ESP_LOGE(TAG, "Update not finished? Something went wrong!");
        }
    }
    else
    {
        ESP_LOGD(TAG, "Error Occurred. Error #: %s", String(Update.getError()).c_str());
    }

    http.stop();
    // Serial.println(F("Server disconnected"));

    // // modem.gprsDisconnect();
    // Serial.println(F("GPRS disconnected"));

    // float duration = float(timeElapsed) / 1000;

    // Serial.println();
    // Serial.print("Content-Length: ");
    // Serial.println(contentLength);
    // Serial.print("Actually read:  ");
    // Serial.println(readLength);
    // Serial.print("Duration:       ");
    // Serial.print(duration);
    // Serial.println("s");

    if (bReboot == true)
        ESP.restart();

    // Do nothing forevermore
    while (true)
    {
        delay(1000);
    }
    vTaskDelete(NULL);
}
#endif
static const char *server_certificate = "-----BEGIN CERTIFICATE-----\n"
                                        "MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\n"
                                        "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
                                        "DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\n"
                                        "SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\n"
                                        "GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
                                        "AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\n"
                                        "q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\n"
                                        "SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\n"
                                        "Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\n"
                                        "a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\n"
                                        "/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\n"
                                        "AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\n"
                                        "CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\n"
                                        "bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\n"
                                        "c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\n"
                                        "VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\n"
                                        "ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\n"
                                        "MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\n"
                                        "Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\n"
                                        "AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\n"
                                        "uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\n"
                                        "wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\n"
                                        "X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\n"
                                        "PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\n"
                                        "KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\n"
                                        "-----END CERTIFICATE-----";
void test_ota()
{
    HttpsOTA.begin("https://www.on1on.cn:8101/dw/finger_c3");
    while (true)
    {
        otastatus = HttpsOTA.status();
        if (otastatus == HTTPS_OTA_SUCCESS)
        {
            Serial.println("Firmware written successfully. To reboot device, call API ESP.restart() or PUSH restart button on device");
        }
        else if (otastatus == HTTPS_OTA_FAIL)
        {
            Serial.println("Firmware Upgrade Fail");
        }
        delay(1000);
    }
}
#endif
void task_ota(void *parm)
{
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    // WiFi.begin("myssid", "12345678");
    // delay(5 * 1000);

    if (WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGE(TAG, "使用WIFI");

        // ota_ser();
        app_ota();
    }
    else
    {
        ESP_LOGE(TAG, "关闭Wifi");
        WiFi.disconnect(true); // 断开并关闭 WiFi
        WiFi.mode(WIFI_OFF);   // 将 WiFi 模式设置为关闭
    }
    vTaskDelete(NULL);
}