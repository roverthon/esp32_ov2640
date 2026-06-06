/**
 ****************************************************************************************************
 * @file        main.c
 * @author      RoverTyphon
 * @version     V1.0
 * @date        2026-06-05
 * @brief       摄像头控制
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "led.h"
#include "lcd.h"
#include "esp_log.h"
#include "esp_err.h"
#include "xl9555.h"
#include "FontDotMatrix32.h"
#include "wifi_config.h"
#include "lwip_demo.h"
#include "iic.h"
#include "esp_camera.h"
#include "esp_lcd_panel_ops.h"
//#include "esp_lcd_panel_io.h"
//#include "esp_camera_fb.h"
//#include "Mqtt_demo.h"
#include "lwip_demo.h"
#include "esp32_weather_time.h"
#include "camera.h"
#include "freertos/event_groups.h"

SemaphoreHandle_t display_1;  /* LCD显示互斥锁 */

#define LED_TASK_PRIO           10
#define LED_STK_SIZE            2048
TaskHandle_t LEDTask_Handler;
void led_task(void *pvParameters);

#define KEY_TASK_PRIO           11
#define KEY_STK_SIZE            4096        /* 增大按键任务栈 */
TaskHandle_t KEYTask_Handler;
void key_task(void *pvParameters);

/* ov2640任务声明 */
void ov2640_task(void *pvParameters);

/* 时间显示任务配置（独立，高优先级，每秒更新） */
#define TIME_TASK_PRIO          10          /* 提高优先级确保时间实时显示 */
#define TIME_TASK_STK_SIZE      4096
TaskHandle_t TimeTask_Handler;

/* 天气显示任务配置（独立，低优先级，120秒更新） */
#define WEATHER_TASK_PRIO       5
#define WEATHER_TASK_STK_SIZE   8192        /* 天气任务需要更大栈空间 */
TaskHandle_t WeatherTask_Handler;
/* ov2640任务配置 */
#define OV2640_TASK_PRIO       5
#define OV2640_TASK_STK_SIZE   8192        /* ov2640任务需要更大栈空间 */
TaskHandle_t OV2640Task_Handler;


static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
i2c_obj_t i2c0_master;
extern uint8_t g_led_control_flag;
extern esp_mqtt_client_handle_t g_mqtt_client;
camera_fb_t *fb = NULL;                    // 摄像头帧缓冲区


extern esp_lcd_panel_handle_t lcd_panel_handle; // 来自LCD驱动
     esp_err_t ret;

void app_main(void)
{
   
    
    display_1 = xSemaphoreCreateMutex();  /* 初始化LCD显示互斥锁 */
    
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();

    lcd_clear(BLACK);
    
    lcd_display_string(0, 0, YELLOW, BLACK, 24, "巴法云MQTT控制实验");
    lcd_display_string(0, 30, WHITE, BLACK, 16, "通过巴法云控制LED");
    lcd_display_string(0, 50, WHITE, BLACK, 16, "发送 on  - 点亮LED");
    lcd_display_string(0, 70, WHITE, BLACK, 16, "发送 off - 关闭LED");
    
    wifi_sta_init();

    taskENTER_CRITICAL(&my_spinlock);
        /* 创建独立的ov2640任务（120秒更新，优先级较低） */
    xTaskCreate((TaskFunction_t )ov2640_task,
                (const char *   )"ov2640_task",
                (uint16_t       )OV2640_TASK_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )OV2640_TASK_PRIO,
                (TaskHandle_t * )&OV2640Task_Handler);

    
    /* 创建按键任务 */
    xTaskCreate((TaskFunction_t )key_task,
                (const char *   )"key_task",
                (uint16_t       )KEY_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )KEY_TASK_PRIO,
                (TaskHandle_t * )&KEYTask_Handler);
    
    /* 创建独立的时间显示任务（每秒更新，优先级较高） */
    xTaskCreate((TaskFunction_t )display_time_task,
                (const char *   )"time_task",
                (uint16_t       )TIME_TASK_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )TIME_TASK_PRIO,
                (TaskHandle_t * )&TimeTask_Handler);
    
    /* 创建独立的天气显示任务（120秒更新，优先级较低） */
    xTaskCreate((TaskFunction_t )display_weather_task,
                (const char *   )"weather_task",
                (uint16_t       )WEATHER_TASK_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )WEATHER_TASK_PRIO,
                (TaskHandle_t * )&WeatherTask_Handler);

    taskEXIT_CRITICAL(&my_spinlock);
    
    /* 启动MQTT连接巴法云 */
    lwip_demo();
}

void key_task(void *pvParameters)
{
    pvParameters = pvParameters;
    uint8_t key;
  
    while (1)
    {
        key = xl9555_key_scan(0);

        if (KEY1_PRES == key)
        {
            vTaskDelay(20);
            if (KEY1_PRES == key)
            {
                if (g_mqtt_client != NULL) {
                    char *send_msg = "Button pressed on ESP32";
                    esp_mqtt_client_publish(g_mqtt_client, BEMFA_TOPIC, send_msg, 0, 0, 0);
                    xSemaphoreTake(display_1, portMAX_DELAY);
                    lcd_display_string(0, 270, WHITE, BLACK, 16, "已发送按键消息到巴法云");
                    xSemaphoreGive(display_1);
                    ESP_LOGI("KEY_TASK", "发送消息到巴法云: %s", send_msg);
                }
            }
        }

        if (KEY2_PRES == key)
        {
            vTaskDelay(20);
            if (KEY2_PRES == key)
            {
                if (g_mqtt_client != NULL) {
                    char *send_msg = "0";
                    esp_mqtt_client_publish(g_mqtt_client, BEMFA_TOPIC, send_msg, 0, 0, 0);
                    xSemaphoreTake(display_1, portMAX_DELAY);
                    lcd_display_string(0, 270, WHITE, BLACK, 16, "已发送按键消息到巴法云");
                    xSemaphoreGive(display_1);
                    ESP_LOGI("KEY_TASK", "发送消息到巴法云: %s", send_msg);
                }
            }
        }
        
        vTaskDelay(10);
    }
}

void ov2640_task(void *pvParameters)
{
    // 启动画面（不变）...
    lcd_show_string(30, 50, 200, 16, 16, "ESP32", RED); 
    lcd_show_string(30, 70, 200, 16, 16, "CAMERA TEST", RED); 
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED); 
    vTaskDelay(pdMS_TO_TICKS(1000));

    lcd_show_string(30, 50, 200, 16, 16, "CAMERA START", RED);
    // 初始化摄像头...
    esp_err_t ret = init_camera();
    if(ret != ESP_OK) {
        lcd_show_string(30, 80, 200, 16, 16, "CAMERA FAIL", RED);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    lcd_fill(0, 0, 239, 179, BLACK);   // 仅清除左边文字区域，不影响右下角摄像头
lcd_show_string(30, 50, 200, 16, 16, "CAMERA START", RED);
    const int disp_w = 80;
    const int disp_h = 60;
    const int cam_w = 320;
    const int cam_h = 240;
    const int down_ratio = 4;

    uint8_t *small_buf = heap_caps_malloc(disp_w * disp_h * 2, MALLOC_CAP_SPIRAM);
    if (small_buf == NULL) {
        while(1) vTaskDelay(1000);
    }

    // 显示位置：右下角
    const int x_start = 240;   // 320 - 80
    const int y_start = 180;   // 240 - 60

    while (1)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (fb->len == cam_w * cam_h * 2) {
            uint16_t *src = (uint16_t *)fb->buf;
            uint16_t *dst = (uint16_t *)small_buf;
            for (int y = 0; y < disp_h; y++) {
                int src_y = y * down_ratio;
                for (int x = 0; x < disp_w; x++) {
                    int src_x = x * down_ratio;
                    dst[y * disp_w + x] = src[src_y * cam_w + src_x];
                }
            }
        }

        xSemaphoreTake(display_1, portMAX_DELAY);
        lcd_display_picture(x_start, y_start, disp_w, disp_h, small_buf);
        xSemaphoreGive(display_1);

        esp_camera_fb_return(fb);
        vTaskDelay(100);
    }

    free(small_buf);
}