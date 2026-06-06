#ifndef __LWIP_DEMO_H
#define __LWIP_DEMO_H

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lcd.h"
#include "led.h"
#include "mqtt_client.h"

/* ========== 巴法云配置 - 请修改为你自己的信息 ========== */
#define BEMFA_HOST          "mqtt.bemfa.com"              /* 巴法云MQTT服务器 */
#define BEMFA_PORT          9501                     /* 巴法云MQTT端口 */
#define BEMFA_UID           "b6351e37efec490cb774e8dd05914b13"           /* 巴法云私钥/UID */
#define BEMFA_TOPIC         "zeKjJiwLy002"                 /* 主题名称，需与巴法云创建设备名称一致 */
#define BEMFB_TOPIC         "7d6WiD4Hg004"                 /* 主题名称，需与巴法云创建设备名称一致 */
#define BEMFC_TOPIC         "i4thUbdqC006"                 /* 主题名称，需与巴法云创建设备名称一致 */
/* =================================================== */

/* MQTT客户端句柄 */
extern esp_mqtt_client_handle_t g_mqtt_client;

/* 控制LED的标志位 */
extern uint8_t g_led_control_flag;

/* 函数声明 */
void lwip_demo(void);

#endif
