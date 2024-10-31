#include "tasks.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp32-hal-gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include <math.h>
#include "audio_player.h"
#include "esp_spiffs.h"
#include "esp_event_loop.h"
#include "blackboard_c.h"
#include "DEV_PIN.h"


// #include"blackboard.h"

#define SAMPLE_RATE (36000)
#define I2S_NUM (0)
#define WAVE_FREQ_HZ (100)
#define PI (3.14159265)
#define I2S_BCK_IO (GPIO_NUM_NC)
#define I2S_WS_IO (GPIO_NUM_NC)
#define I2S_DO_IO (GPIO_NUM_3)
#define I2S_DI_IO (GPIO_NUM_NC)
#define CONST_PI (3.1416f)
#define EXAMPLE_WAVE_AMPLITUDE (1000.0) // 1~32767
#define EXAMPLE_TONE_LAST_TIME_MS 500
#define EXAMPLE_PDM_TX_FREQ_HZ 44100 // I2S PDM TX frequency

#define EXAMPLE_BYTE_NUM_EVERY_TONE (EXAMPLE_TONE_LAST_TIME_MS * EXAMPLE_PDM_TX_FREQ_HZ / 1000)
#define EXAMPLE_SINE_WAVE_LEN(tone) (uint32_t)((EXAMPLE_PDM_TX_FREQ_HZ / (float)tone) + 0.5) // The sample point number per sine wave to generate the tone
#define I2S_CONFIG_DEFAULT()                               \
    {                                                      \
        .mode = I2S_MODE_PDM | I2S_MODE_TX,                \
        .sample_rate = 44100,                              \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,      \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,      \
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,          \
        .dma_buf_count = 6,                                \
        .dma_buf_len = 160,                                \
        .use_apll = false,                                 \
        .tx_desc_auto_clear = true,                        \
        .fixed_mclk = 0,                                   \
        .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,        \
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,          \
    }

#define SAMPLE_PER_CYCLE (SAMPLE_RATE / WAVE_FREQ_HZ)

static const char *TAG = "i2s_example";
bool audio_player_sta = false;
/* The frequency of tones: do, re, mi, fa, so, la, si, in Hz. */
static const uint32_t tone[3][7] = {{262, 294, 330, 349, 392, 440, 494},         // bass
                                    {523, 587, 659, 698, 784, 880, 988},         // alto
                                    {1046, 1175, 1318, 1397, 1568, 1760, 1976}}; // treble
/* Numbered musical notation of 'twinkle twinkle little star' */
static const uint8_t song[28] = {1, 1, 5, 5, 6, 6, 5,
                                 4, 4, 3, 3, 2, 2, 1,
                                 5, 5, 4, 4, 3, 3, 2,
                                 5, 5, 4, 4, 3, 3, 2};
/* Rhythm of 'twinkle twinkle little star', it's repeated in four sections */
static const uint8_t rhythm[7] = {1, 1, 1, 1, 1, 1, 2};

static const char *tone_name[3] = {"bass", "alto", "treble"};
static void audio_player_callback(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event)
    {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE: /**< Player is idle, not playing audio */
        ESP_LOGI(TAG, "IDLE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        ESP_LOGI(TAG, "NEXT");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        ESP_LOGI(TAG, "PLAYING");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        ESP_LOGI(TAG, "PAUSE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
        ESP_LOGI(TAG, "SHUTDOWN");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE:
        ESP_LOGI(TAG, "UNKNOWN FILE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN:
        ESP_LOGI(TAG, "UNKNOWN");
        break;
    }
}
static void audio_player_callback2(audio_player_cb_ctx_t *ctx)
{
    ESP_LOGI(TAG, "audio_player_callback");
}
void spiffs_app(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 10,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}
static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    ESP_LOGI(TAG, "mute setting %d", setting);
    return ESP_OK;
}
int spk_init()
{
    if (audio_player_sta)
    {
        audio_player_state_t player_sta = audio_player_get_state();
        switch (player_sta)
        {
        case AUDIO_PLAYER_STATE_IDLE:
            return 0;
            break;
        case AUDIO_PLAYER_STATE_PLAYING:
            audio_player_delete();
            return 1;
            break;

        default:
            break;
        }
    }

    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO // Not used
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);

    audio_player_config_t config = {.port = I2S_NUM,
                                    .mute_fn = audio_mute_function,
                                    .priority = 5};
    esp_err_t ret = audio_player_new(config);
    // FILE *fp = fopen("/spiffs/factory.mp3", "r");
    ret = audio_player_callback_register(audio_player_callback, NULL);

    return 0;
}
void audio_event_handle(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    // Assuming event_data is a null-terminated string
    const char *filename = (const char *)event_data;

    ESP_LOGE("EVENT_HANDLE", "msg: %s", filename);
    // digitalWrite(audio_ctrl,HIGH);
    if (audio_player_sta)
    {
        audio_player_delete();

        audio_player_config_t config = {.port = I2S_NUM,
                                        .mute_fn = audio_mute_function,
                                        .priority = 5};

        audio_player_new(config);
        audio_player_callback_register(audio_player_callback, NULL);
        audio_player_sta = true;
        // spk_init();

        FILE *fp = fopen(filename, "r");

        if (fp == NULL)
        {
            ESP_LOGE("EVENT_HANDLE", "Failed to open file: %s", filename);
            // Handle error, return, or do something appropriate
            return;
        }

        audio_player_play(fp);
        // spk_delet();
    }
    else
    {
        i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();

        i2s_pin_config_t pin_config = {
            .mck_io_num = I2S_PIN_NO_CHANGE,
            .bck_io_num = I2S_BCK_IO,
            .ws_io_num = I2S_WS_IO,
            .data_out_num = I2S_DO_IO,
            .data_in_num = I2S_DI_IO // Not used
        };
        i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
        i2s_set_pin(I2S_NUM, &pin_config);
        i2s_zero_dma_buffer(I2S_NUM);
        audio_player_config_t config = {.port = I2S_NUM,
                                        .mute_fn = audio_mute_function,
                                        .priority = 5};

        audio_player_new(config);
        audio_player_callback_register(audio_player_callback, NULL);
        audio_player_sta = true;
        // spk_init();

        FILE *fp = fopen(filename, "r");

        if (fp == NULL)
        {
            ESP_LOGE("EVENT_HANDLE", "Failed to open file: %s", filename);
            // Handle error, return, or do something appropriate
            return;
        }

        audio_player_play(fp);
        // spk_delet();
    }
    // vTaskDelay(1700);
    // digitalWrite(audio_ctrl,LOW);
    // vTaskDelay(5000);
    // fclose(fp);
}

void consumer_event_handle(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
}

void litter_star()
{
    int test_bits = 16;
    // while (1) {
    // setup_triangle_sine_waves(test_bits);
    //     vTaskDelay(5000/portTICK_RATE_MS);
    //     test_bits += 8;
    //     if(test_bits > 32)
    //         test_bits = 16;

    // }
    int16_t *w_buf = (int16_t *)calloc(1, 2048);
    uint8_t tone_select = 0; // To selecting the tone level
    size_t w_bytes = 0;
    uint8_t cnt = 0; // The current index of the song

    // while (0)
    while (true)
    {
        int tone_point = EXAMPLE_SINE_WAVE_LEN(tone[tone_select][song[cnt] - 1]);
        /* Generate the tone buffer */
        for (int i = 0; i < tone_point; i++)
        {
            w_buf[i] = (int16_t)((sin(2 * (float)i * CONST_PI / tone_point)) * EXAMPLE_WAVE_AMPLITUDE);
        }
        for (int tot_bytes = 0; tot_bytes < EXAMPLE_BYTE_NUM_EVERY_TONE * rhythm[cnt % 7]; tot_bytes += w_bytes)
        {
            /* Play the tone */
            if (i2s_write(I2S_NUM, w_buf, tone_point * sizeof(int16_t), &w_bytes, 1000) != ESP_OK)
            {
                printf("Write Task: i2s write failed\n");
            }
        }
        cnt++;
        /* If finished playing, switch the tone level */
        if (cnt == sizeof(song))
        {
            cnt = 0;
            tone_select++;
            tone_select %= 3;
            printf("Playing %s `twinkle twinkle little star`\n", tone_name[tone_select]);
        }
        /* Gap between the tones */
        vTaskDelay(15);
    }
}
int spk_delet()
{
    ESP_LOGE(TAG, "%d", audio_player_sta);

    if (audio_player_sta)
    {
        while (audio_player_get_state() != AUDIO_PLAYER_STATE_IDLE)
        {
            vTaskDelay(100);
        }
        audio_player_delete();
        i2s_driver_uninstall(I2S_NUM);
        audio_player_sta = false;
        ESP_LOGE(TAG, "关闭iic ok");
    }

    return 0;
}

void task_audio_entry(void *params)
{
    // esp_event_loop_handle_t *bk_mqtt_handler = (esp_event_loop_handle_t *)params;

    spiffs_app();
    char *handle_arg = "我是个大胃王,爱吃辣";

    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO // Not used
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
// litter_star();
    esp_event_handler_register_with(bk_mqtt_handler, "audio", 1, audio_event_handle, handle_arg);
    if (1)
    {

        audio_player_config_t config = {.port = I2S_NUM,
                                        .mute_fn = audio_mute_function,
                                        .priority =1};
        esp_err_t ret = audio_player_new(config);
        vTaskDelay(1000);
        // pinMode(audio_ctrl,OUTPUT);
        // digitalWrite(audio_ctrl,HIGH);
        ret = audio_player_callback_register(audio_player_callback, NULL);
        FILE *fp = fopen("/spiffs/coin.mp3","r");
        if (fp == NULL) {
    // 文件打开失败，处理错误
    perror("Error opening file");
}
        audio_player_play(fp);
        vTaskDelay(1000);
       digitalWrite(audio_ctrl,LOW);
        audio_player_sta = true;
        while (false)
        {

            vTaskDelay(5000);
            audio_player_delete();
            digitalWrite(audio_ctrl,HIGH);
            ret = audio_player_new(config);
            // FILE *fp = fopen("/spiffs/fangIC.mp3", "r");
            ret = audio_player_callback_register(audio_player_callback, NULL);
            
            FILE *fp = fopen("/spiffs/unlock.mp3", "r");
            audio_player_play(fp);
            vTaskDelay(1500);
           digitalWrite(audio_ctrl,LOW);
        }
    }

    // vTaskDelete(NULL);
    int count_d = 0;
    while (true)
    {
        if (audio_player_sta)
        {

            if (audio_player_get_state() == AUDIO_PLAYER_STATE_IDLE)
            {
                count_d++;
            }
            else
            {
                count_d = 0;
            }
            if (count_d > 5)
            {
                // i2s_driver_uninstall(I2S_NUM);
                // spk_delet();
                ESP_LOGE(TAG, "关闭iic");
            }
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
