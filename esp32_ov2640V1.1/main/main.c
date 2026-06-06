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
#include "esp_system.h"
#include "nvs_flash.h"
#include "led.h"
#include "lcd.h"
#include "esp_log.h"
#include "esp_err.h"
#include "xl9555.h"
#include "FontDotMatrix32.h"
#include "wifi_config.h"
#include "iic.h"
#include "esp_camera.h"
#include "esp_lcd_panel_ops.h"
//#include "esp_lcd_panel_io.h"
//#include "esp_camera_fb.h"
#include "Mqtt_demo.h"
#include "lwip_demo.h"
#include "camera.h"
#include "freertos/event_groups.h"


 i2c_obj_t i2c0_master;
camera_fb_t *fb = NULL;                    // 摄像头帧缓冲区


extern esp_lcd_panel_handle_t lcd_panel_handle; // 来自LCD驱动

/** 
 * @brief       程序入口 
 * @param       无 
 * @retval      无  
 */ 
void app_main(void) 
{ 
    //uint8_t x = 0; 
    esp_err_t ret; 
     
    ret = nvs_flash_init();              /* 初始化NVS */ 
 
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||  
         ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    { 
        ESP_ERROR_CHECK(nvs_flash_erase()); 
        ret = nvs_flash_init(); 
    } 
 
    led_init();                          /* 初始化LED */ 
    i2c0_master = iic_init(I2C_NUM_0);  /* 初始化IIC0 */ 
    spi2_init();                         /* 初始化SPI2 */ 
    xl9555_init(i2c0_master);            /* 初始化XL9555 */ 
    lcd_init();                          /* 初始化LCD */ 
 
    lcd_show_string(30, 50, 200, 16, 16, "ESP32", RED); 
    lcd_show_string(30, 70, 200, 16, 16, "CAMERA TEST", RED); 
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED); 
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_clear(BLACK);
    lcd_show_string(30, 50, 200, 16, 16, "CAMERA START", RED);
    /* 初始化摄像头 */ 
    printf("正在初始化摄像头\n");
   ret = init_camera();
    lcd_show_string(30, 60, 200, 16, 16, "main.c84", GREEN);
    if(ret != ESP_OK)
    {
        lcd_show_string(30, 60, 200, 16, 16, "LCD OK", GREEN);
        while(1)
        {
        lcd_show_string(30, 80, 200, 16, 16, "CAMERA FAIL", RED);
         vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
   lcd_show_string(30, 60, 200, 16, 16, "main.c94", GREEN);
    
    // while (camera_init()) 
    // { 
    //     lcd_show_string(30, 110, 200, 16, 16, "CAMERA Fail!", BLUE); 
    //     vTaskDelay(500); 
    //     } 
 lcd_clear(BLACK); 
     //获取图像尺寸（在init_camera后配置已生效）
  //  sensor_t *s = esp_camera_sensor_get();
    int cam_width = 320;   // 例如 320 (QVGA)
    int cam_height = 240; // 240
while (1) 
{ 
    fb = esp_camera_fb_get();
    if(fb == NULL) {
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
    }
   lcd_display_picture(0, 0, cam_width, cam_height, fb->buf);
    esp_camera_fb_return(fb);   
    vTaskDelay(pdMS_TO_TICKS(10));
}
}