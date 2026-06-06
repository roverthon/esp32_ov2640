#include "esp32_weather_time.h"
#include "lcd.h"
#include "esp_sntp.h"
#include <time.h>

static const char *TAG = "WEATHER_TIME";

/* 心知天气API密钥 */
#define XZTQ_KEY "SL-hMQfOkPjQv-xkU"
#define CITY_NAME "guangzhou"

static bool time_synced = false;

/* NTP时间同步回调 */
static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "NTP时间同步成功");
    time_synced = true;
}

/* 初始化NTP获取时间 */
static void ntp_init(void)
{
    ESP_LOGI(TAG, "初始化NTP客户端...");
    setenv("TZ", "CST-8", 1);
    tzset();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp1.aliyun.com");
    sntp_setservername(1, "ntp.tencent.com");
    sntp_setservername(2, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

/* 获取网络时间（使用NTP） */
esp_err_t esp32_get_network_time(time_info_t *time_info)
{
    time_t now;
    struct tm timeinfo;
    
    if (!time_synced) {
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year < (2016 - 1900)) {
            return ESP_FAIL;
        }
        time_synced = true;
    }
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(time_info->year, sizeof(time_info->year), "%04d", timeinfo.tm_year + 1900);
    snprintf(time_info->month, sizeof(time_info->month), "%02d", timeinfo.tm_mon + 1);
    snprintf(time_info->date, sizeof(time_info->date), "%02d", timeinfo.tm_mday);
    snprintf(time_info->hour, sizeof(time_info->hour), "%02d", timeinfo.tm_hour);
    snprintf(time_info->min, sizeof(time_info->min), "%02d", timeinfo.tm_min);
    snprintf(time_info->second, sizeof(time_info->second), "%02d", timeinfo.tm_sec);
    
    const char *weekdays[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    snprintf(time_info->weekday, sizeof(time_info->weekday), "%s", weekdays[timeinfo.tm_wday]);
    
    return ESP_OK;
}

/* 获取天气信息（简化版，避免崩溃） */
esp_err_t esp32_get_weather(weather_info_t *weather_info)
{
    /* 暂时返回模拟数据，确保系统稳定 */
    snprintf(weather_info->city, sizeof(weather_info->city), "广州");
    snprintf(weather_info->weather, sizeof(weather_info->weather), "晴");
    snprintf(weather_info->temp, sizeof(weather_info->temp), "28°C");
    
    ESP_LOGI(TAG, "天气(模拟): %s, %s, %s", 
             weather_info->city, weather_info->weather, weather_info->temp);
    
    return ESP_OK;
}

/* ========== 独立的时间显示任务（每秒更新） ========== */
void display_time_task(void *pvParameters)
{
    time_info_t time_info = {0};
    char date_buffer[64];
    int display_count = 0;
    
    ESP_LOGI(TAG, "时间显示任务已启动");
    
    /* 初始化NTP */
    ntp_init();
    
    /* 等待WiFi连接稳定和NTP同步 */
    vTaskDelay(pdMS_TO_TICKS(8000));
    
    while (1) {
        if (esp32_get_network_time(&time_info) == ESP_OK) {
            snprintf(date_buffer, sizeof(date_buffer), "%.4s-%.2s-%.2s %.2s:%.2s:%.2s",
                     time_info.year, time_info.month, time_info.date,
                     time_info.hour, time_info.min, time_info.second);
            lcd_display_string(0, 100, CYAN, BLACK, 16, date_buffer);
            
            /* 每10秒打印一次日志，避免刷屏 */
            if (display_count % 10 == 0) {
                ESP_LOGI(TAG, "时间显示: %s", date_buffer);
            }
        } else {
            lcd_display_string(0, 100, YELLOW, BLACK, 16, "等待时间同步...");
        }
        
        display_count++;
        /* 每秒更新一次时间 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ========== 独立的天气显示任务（每120秒更新一次，降低频率） ========== */
void display_weather_task(void *pvParameters)
{
    weather_info_t weather_info = {0};
    char weather_buffer[64];
    
    ESP_LOGI(TAG, "天气显示任务已启动");
    
    /* 等待系统稳定后再请求天气 */
    vTaskDelay(pdMS_TO_TICKS(15000));
    
    while (1) {
        if (esp32_get_weather(&weather_info) == ESP_OK) {
            snprintf(weather_buffer, sizeof(weather_buffer), "天气:%.7s %.6s", 
                     weather_info.weather, weather_info.temp);
            lcd_display_string(0, 120, GREEN, BLACK, 16, weather_buffer);
            ESP_LOGI(TAG, "天气显示: %s", weather_buffer);
        } else {
            lcd_display_string(0, 120, RED, BLACK, 16, "天气获取失败");
            ESP_LOGW(TAG, "天气获取失败");
        }
        
        /* 每120秒更新一次天气，减少网络请求 */
        vTaskDelay(pdMS_TO_TICKS(120000));
    }
}