#ifndef __ESP32_WEATHER_TIME_H
#define __ESP32_WEATHER_TIME_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

/* 时间结构体 */
typedef struct
{
    char year[16];
    char month[16];
    char date[16];
    char weekday[16];
    char hour[16];
    char min[16];
    char second[16];
    char gmt[16];
} time_info_t;

/* 天气结构体 */
typedef struct
{
    char city[32];
    char weather[32];
    char temp[16];
    char humidity[16];
    char wind[32];
    char quality[16];
} weather_info_t;

/* 函数声明 */
esp_err_t esp32_get_network_time(time_info_t *time_info);
esp_err_t esp32_get_weather(weather_info_t *weather_info);
void display_time_task(void *pvParameters);      /* 独立的时间显示任务 */
void display_weather_task(void *pvParameters);   /* 独立的天气显示任务 */

#endif