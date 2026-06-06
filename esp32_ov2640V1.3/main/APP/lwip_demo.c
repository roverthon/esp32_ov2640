#include "lwip_demo.h"
#include "esp_log.h"

// 定义接收数据缓冲区大小
#define LWIP_DEMO_RX_BUFSIZE         200                        

// 定义巴法云连接参数
#define BEMFA_CLIENT_ID             "ESP32S3_Client"            /* 客户端ID，可自定义 */

// 全局变量
uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE]; 
uint8_t g_led_control_flag = 0;                                 /* LED控制标志 */
esp_mqtt_client_handle_t g_mqtt_client = NULL;                  /* MQTT客户端句柄 */

// 日志标签
static const char *TAG = "BEMFA_MQTT";

/**
 * @brief       MQTT事件回调函数
 * @param       handler_args: 回调参数
 * @param       base: 事件基类
 * @param       event_id: 事件ID
 * @param       event_data: 事件数据
 * @retval      无
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:      /* MQTT连接成功 */
            ESP_LOGI(TAG, "MQTT连接成功！");
            xSemaphoreTake(display_1, portMAX_DELAY);
            lcd_display_string(0, 50, WHITE, BLACK, 16, "状态:巴法云已连接");
            xSemaphoreGive(display_1);
            
            /* 订阅主题 */
            msg_id = esp_mqtt_client_subscribe(client, BEMFA_TOPIC, 0);
            ESP_LOGI(TAG, "订阅主题: %s, msg_id=%d", BEMFA_TOPIC, msg_id);
            
            /* 发布上线消息 */
            esp_mqtt_client_publish(client, BEMFA_TOPIC, "ESP32-S3 已上线", 0, 0, 0);
            break;
            
        case MQTT_EVENT_DISCONNECTED:   /* MQTT连接断开 */
            ESP_LOGI(TAG, "MQTT连接断开");
            xSemaphoreTake(display_1, portMAX_DELAY);
            lcd_display_string(0, 50, WHITE, BLACK, 16, "状态:巴法云未连接");
            xSemaphoreGive(display_1);
            break;
            
        case MQTT_EVENT_SUBSCRIBED:     /* 订阅成功 */
            ESP_LOGI(TAG, "订阅成功, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:   /* 取消订阅成功 */
            ESP_LOGI(TAG, "取消订阅成功, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:      /* 发布成功 */
            ESP_LOGI(TAG, "发布成功, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:           /* 收到MQTT消息 */
            ESP_LOGI(TAG, "收到消息: TOPIC=%.*s, DATA=%.*s", 
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            
            /* 处理接收到的数据 - 控制LED */
            if (event->data_len > 0) {
                char *cmd = malloc(event->data_len + 1);
                if (cmd) {
                    memcpy(cmd, event->data, event->data_len);
                    cmd[event->data_len] = '\0';
                    
                    /* 显示收到的指令 */
                    xSemaphoreTake(display_1, portMAX_DELAY);
                    lcd_display_string_fmt(0, 70, WHITE, BLACK, 16, "收到指令:%.*s", event->data_len, cmd);
                    xSemaphoreGive(display_1);
                    
                    /* 控制LED */
                    if (strcmp(cmd, "on") == 0) {
                        led_on();                           /* 点亮LED */
                        g_led_control_flag = 1;
                        ESP_LOGI(TAG, "LED已开启");
                        xSemaphoreTake(display_1, portMAX_DELAY);
                        lcd_display_string(0, 90, YELLOW, BLACK, 16, "LED状态:开启");
                        xSemaphoreGive(display_1);
                    } 
                    else if (strcmp(cmd, "off") == 0) {
                        led_off();                          /* 关闭LED */
                        g_led_control_flag = 0;
                        ESP_LOGI(TAG, "LED已关闭");
                        xSemaphoreTake(display_1, portMAX_DELAY);
                        lcd_display_string(0, 90, YELLOW, BLACK, 16, "LED状态:关闭");
                        xSemaphoreGive(display_1);
                    }
                    else {
                        ESP_LOGI(TAG, "未知指令: %s", cmd);
                    }
                    
                    free(cmd);
                }
            }
            break;
            
        case MQTT_EVENT_ERROR:          /* MQTT错误 */
            ESP_LOGE(TAG, "MQTT发生错误");
            break;
            
        default:
            break;
    }
}

/**
 * @brief       lwip_demo实验入口 - 连接巴法云MQTT
 * @param       无
 * @retval      无
 */
void lwip_demo(void)
{
    ESP_LOGI(TAG, "开始连接巴法云...");
    
    /* 在LCD上显示连接信息 */
    lcd_display_string_fmt(0, 170, WHITE, BLACK, 16, "巴法云服务器:%s", BEMFA_HOST);
    lcd_display_string_fmt(0, 190, WHITE, BLACK, 16, "端口:%d", BEMFA_PORT);
    lcd_display_string_fmt(0, 210, WHITE, BLACK, 16, "主题:%s", BEMFA_TOPIC);
    lcd_display_string(0, 230, YELLOW, BLACK, 16, "等待连接巴法云...");
    
    /* 配置MQTT客户端参数 */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = BEMFA_HOST,
        .broker.address.port = BEMFA_PORT,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .credentials.client_id = BEMFA_UID,
        .credentials.username = NULL,           /* 巴法云使用UID作为用户名 */
        .credentials.authentication.password = "",   /* 巴法云一般不需要密码 */
        .session.keepalive = 120,           // 增加保活时间,MQTT的心跳机制，定期验证连接是否正常,每120秒无通信时，发送一次心跳包
        .session.disable_clean_session = false,
        .network.timeout_ms = 10000,
        .network.disable_auto_reconnect = false,
    };

    /* 初始化MQTT客户端 */
    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        lcd_display_string(0, 250, RED, BLACK, 16, "MQTT初始化失败!");
        return;
    }
    
    /* 注册事件回调 */
    esp_mqtt_client_register_event(g_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    /* 启动MQTT客户端 */
    esp_mqtt_client_start(g_mqtt_client);
    
    ESP_LOGI(TAG, "MQTT客户端已启动，等待连接...");
}

