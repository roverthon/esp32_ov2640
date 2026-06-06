/**
 ****************************************************************************************************
 * @file        camera.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       摄像头驱动代码
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

#ifndef __CAMERA_H
#define __CAMERA_H

#include "driver/gpio.h"
#include "xl9555.h"
//#include "driver/i2c.h"
//#include "esp_err.h"
/* 引脚声明 */ 
// // 正点原子 ESP32-S3 开发板 + OV2640 官方正确引脚
#define CAM_PIN_PWDN     GPIO_NUM_NC 
#define CAM_PIN_RESET    GPIO_NUM_NC 
#define CAM_PIN_XCLK     GPIO_NUM_10
#define CAM_PIN_SIOD     GPIO_NUM_39 
#define CAM_PIN_SIOC     GPIO_NUM_38 
#define CAM_PIN_D7       GPIO_NUM_18 
#define CAM_PIN_D6       GPIO_NUM_17 
#define CAM_PIN_D5       GPIO_NUM_16 
#define CAM_PIN_D4       GPIO_NUM_15 
#define CAM_PIN_D3       GPIO_NUM_7 
#define CAM_PIN_D2       GPIO_NUM_6 
#define CAM_PIN_D1       GPIO_NUM_5 
#define CAM_PIN_D0       GPIO_NUM_4 
#define CAM_PIN_VSYNC    GPIO_NUM_47 
#define CAM_PIN_HREF     GPIO_NUM_48 
#define CAM_PIN_PCLK     GPIO_NUM_45 
// //根据原理图重新定义ESP32-S3真实GPIO
// #define CAM_PIN_PWDN     GPIO_NUM_NC   // 由 XL9555 扩展IO控制
// #define CAM_PIN_RESET    GPIO_NUM_NC   // 由 XL9555 扩展IO控制
// #define CAM_PIN_XCLK     GPIO_NUM_10   // 官方固定：GPIO10
// #define CAM_PIN_SIOD     GPIO_NUM_39   // 官方固定：GPIO39
// #define CAM_PIN_SIOC     GPIO_NUM_38   // 官方固定：GPIO38

// #define CAM_PIN_D7       GPIO_NUM_18
// #define CAM_PIN_D6       GPIO_NUM_17
// #define CAM_PIN_D5       GPIO_NUM_16
// #define CAM_PIN_D4       GPIO_NUM_15
// #define CAM_PIN_D3       GPIO_NUM_7
// #define CAM_PIN_D2       GPIO_NUM_6
// #define CAM_PIN_D1       GPIO_NUM_5
// #define CAM_PIN_D0       GPIO_NUM_4

// #define CAM_PIN_VSYNC    GPIO_NUM_47
// #define CAM_PIN_HREF     GPIO_NUM_48
// #define CAM_PIN_PCLK     GPIO_NUM_45
#define CAM_PWDN(x)      do{ x ?\
                             (xl9555_pin_write(OV_PWDN_IO, 1)):\
                             (xl9555_pin_write(OV_PWDN_IO, 0));\
                         }while(0) 
 
#define CAM_RST(x)       do{ x ?\
                             (xl9555_pin_write(OV_RESET_IO, 1)):\
                             (xl9555_pin_write(OV_RESET_IO, 0));\
                         }while(0) 
 extern esp_err_t init_camera(void);

#endif
