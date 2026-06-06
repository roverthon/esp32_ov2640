/**
 ****************************************************************************************************
 * @file        camera.c
 * @author      RoverTyphon
 * @version     V1.0
 * @date        2026-04-60
 * @brief       CAMERA驱动代码
 ****************************************************************************************************
 * @attention
 *
 *
 ****************************************************************************************************
 */
#include "camera.h"
#include "lcd.h"
#include "esp_camera.h"    
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // vTaskDelay 需要
/* 摄像头配置 */ 
extern SemaphoreHandle_t display_1;
static camera_config_t camera_config = {
    /* 引脚配置 */
    .pin_pwdn = CAM_PIN_PWDN, 
    .pin_reset = CAM_PIN_RESET, 
    .pin_xclk = CAM_PIN_XCLK, 
    .pin_sccb_sda = CAM_PIN_SIOD, 
    .pin_sccb_scl = CAM_PIN_SIOC, 
    .pin_d7 = CAM_PIN_D7, 
    .pin_d6 = CAM_PIN_D6, 
    .pin_d5 = CAM_PIN_D5, 
    .pin_d4 = CAM_PIN_D4, 
    .pin_d3 = CAM_PIN_D3, 
    .pin_d2 = CAM_PIN_D2, 
    .pin_d1 = CAM_PIN_D1, 
    .pin_d0 = CAM_PIN_D0, 
    .pin_vsync = CAM_PIN_VSYNC, 
    .pin_href = CAM_PIN_HREF, 
    .pin_pclk = CAM_PIN_PCLK, 
    /* XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental) */
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
//lcd_show_string(30, 60, 100, 32, 32, "camera init___________78", GREEN);
    .pixel_format = PIXFORMAT_RGB565,   /* YUV422,GRAYSCALE,RGB565,JPEG */
    .frame_size = FRAMESIZE_QVGA,       /* QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates */
 
    .jpeg_quality = 12,                 /* 0-63, for OV series camera sensors, lower number means higher quality */
    .fb_count = 2,                      /* When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode */
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};
 
/**
 * @brief       摄像头初始化
 * @param       无
 * @retval      esp_err_t
 */
esp_err_t init_camera(void)
{
    // 电源和复位时序（XL9555 控制）
    CAM_PWDN(1);        // PWDN 高电平先断电
    vTaskDelay(pdMS_TO_TICKS(10));
    CAM_PWDN(0);        // 上电
    vTaskDelay(pdMS_TO_TICKS(10));

    CAM_RST(0);
    vTaskDelay(pdMS_TO_TICKS(20));
    CAM_RST(1);
    vTaskDelay(pdMS_TO_TICKS(20));

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE("CAM", "Camera init failed with error 0x%x", err);
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV2640_PID) {
        s->set_vflip(s, 1);           // 根据实际画面上下颠倒情况改 0 或 1
        s->set_hmirror(s, 0);         // 左右镜像，改 0 或 1
        
        s->set_brightness(s, 1);      // -2 ~ 2，试 0、1、2
        s->set_contrast(s, 1);        // -2 ~ 2，试 0 或 1
        s->set_saturation(s, 0);      // -2 ~ 2，试 0 或 1
        
        s->set_whitebal(s, 1);        // 启用自动白平衡
        s->set_awb_gain(s, 1);        // 自动白平衡增益
        s->set_wb_mode(s, 0);         // 0=自动，1=晴天，2=多云，3=办公室，4=家里
        
        s->set_exposure_ctrl(s, 1);   // 自动曝光
        s->set_aec_value(s, 300);     // 手动曝光值（如果自动不好，可尝试 100~600）
        s->set_gain_ctrl(s, 1);       // 自动增益
        s->set_agc_gain(s, 10);       // 增益值
        
        s->set_dcw(s, 1);             // 降采样
        s->set_bpc(s, 1);             // 坏点校正
        s->set_wpc(s, 1);             // 白点校正
    }
    ESP_LOGI("CAM", "Camera init success: OV2640");
    xSemaphoreTake(display_1, portMAX_DELAY);
    lcd_show_string(30, 60, 100, 32, 32, "Camera OK!", GREEN);
    xSemaphoreGive(display_1);

    return ESP_OK;
}