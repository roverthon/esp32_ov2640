// 包含LWIP演示相关头文件，提供函数声明和宏定义
#include "lwip_demo.h"

/* 需要自己设置远程IP地址 */
// 定义远程服务器IP地址，需根据实际服务器IP修改
#define IP_ADDR "192.168.0.104"

// 定义最大接收数据缓冲区大小(100字节)
#define LWIP_DEMO_RX_BUFSIZE 100                     /* 最大接收数据长度 */
// 定义本地TCP连接端口号(8080)
#define LWIP_DEMO_PORT 8080                          /* 连接的本地端口号 */
// 定义发送数据线程优先级(空闲优先级+3)
#define LWIP_SEND_THREAD_PRIO (tskIDLE_PRIORITY + 3) /* 发送数据线程优先级 */
/* 接收数据缓冲区 */
// 全局接收缓冲区，用于存储从TCP服务器接收的数据
uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];

/* 发送数据内容 */
// 全局发送缓冲区，存储默认发送数据"你好粤嵌科技\r\n"
uint8_t g_lwip_demo_sendbuf[] = "你好粤嵌科技\r\n";
/* 数据发送标志位 */
// 全局发送标志位，用于控制数据发送触发(需配合外部事件置位)
uint8_t g_lwip_send_flag;
// 全局socket描述符，-1表示未创建/已关闭
int g_sock = -1;
// 全局连接状态标志：0-未连接，1-已连接
int g_lwip_connect_state = 0;
// 静态函数声明：发送数据线程函数(在下方定义)
static void lwip_send_thread(void *arg);

/**
 * @brief       发送数据线程
 * @param       无
 * @retval      无
 */
// 创建发送数据线程的函数
void lwip_data_send(void)
{
    // 使用FreeRTOS API创建发送线程：
    // 线程函数名lwip_send_thread，线程名"lwip_send_thread"，栈大小4096字节，
    // 无参数，优先级LWIP_SEND_THREAD_PRIO，不保存线程句柄
    xTaskCreate(lwip_send_thread, "lwip_send_thread", 4096, NULL, LWIP_SEND_THREAD_PRIO, NULL);
}

/**
 * @brief       lwip_demo实验入口
 * @param       无
 * @retval      无
 */
// LWIP演示主函数：负责TCP客户端初始化、连接服务器及接收数据
void lwip_demo(void)
{
    // 定义IPv4地址结构，用于存储服务器地址信息
    struct sockaddr_in client_addr;
    // 错误码变量，用于接收LWIP API返回值
    err_t err;
    // 接收数据长度变量，存储每次recv返回的实际接收字节数
    int recv_data_len;
    // 临时缓冲区指针，用于格式化LCD显示字符串
    char *tbuf;

    // 调用函数创建发送数据线程(在连接建立前提前创建线程)
    lwip_data_send(); /* 创建发送数据线程 */

    // 主循环：持续维护TCP连接(断开后自动重连)
    while (1) {
    sock_start:  // 连接起点标签，用于连接失败/断开后跳转重连
        // 重置连接状态为未连接
        g_lwip_connect_state = 0;
        // 设置地址族为IPv4
        client_addr.sin_family = AF_INET;                 /* 表示IPv4网络协议 */
        // 设置目标端口号：将本地字节序转换为网络字节序(大端模式)
        client_addr.sin_port = htons(LWIP_DEMO_PORT);     /* 端口号 */
        // 设置远程服务器IP地址：将字符串IP转换为网络字节序整数
        client_addr.sin_addr.s_addr = inet_addr(IP_ADDR); /* 远程IP地址 */
        // 创建TCP socket：IPv4协议族(SOCK_STREAM表示TCP)，默认传输协议(0)
        g_sock = socket(AF_INET, SOCK_STREAM, 0);             /* 可靠数据流交付服务既是TCP协议 */
        // 清零地址结构中的填充字段(确保兼容性)
        memset(&(client_addr.sin_zero), 0, sizeof(client_addr.sin_zero));

        // 动态分配200字节内存，用于存储端口号显示字符串
        tbuf = malloc(200);                               /* 申请内存 */
        // 格式化端口号字符串(如"端口:8080")
        sprintf((char *)tbuf, "端口:%d", LWIP_DEMO_PORT); /* 客户端端口号 */
        // 在LCD(0,170)位置显示端口号信息，白色字体黑色背景16号字
        lcd_display_string(0, 170, WHITE, BLACK, 16, tbuf);

        /* 连接远程IP地址 */
        // 调用connect连接服务器：传入socket描述符、服务器地址结构及长度
        err = connect(g_sock, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

        // 判断连接是否失败(err == -1表示连接失败)
        if (err == -1) {
            // 在LCD(0,190)位置显示"状态:未连接"
            lcd_display_string(0, 190, WHITE, BLACK, 16, "状态:未连接");
            // 重置socket描述符为-1(标记未创建)
            g_sock = -1;
            // 关闭socket(释放资源)
            closesocket(g_sock);
            // 释放之前分配的tbuf内存(避免内存泄漏)
            free(tbuf);
            // 延时10ms后重试连接
            vTaskDelay(10);
            // 跳转到sock_start标签，重新开始连接流程
            goto sock_start;
        }

        // 连接成功：在LCD(0,190)位置显示"状态:已连接"
        lcd_display_string(0, 190, WHITE, BLACK, 16, "状态:已连接");
        // 更新连接状态标志为已连接(1)
        g_lwip_connect_state = 1;

        // 进入数据接收循环(连接成功后持续接收数据)
        while (1) {
            // 接收数据：从socket接收数据到接收缓冲区，最大长度为LWIP_DEMO_RX_BUFSIZE，阻塞模式(0)
            recv_data_len = recv(g_sock, g_lwip_demo_recvbuf, LWIP_DEMO_RX_BUFSIZE, 0);
            // 判断接收是否失败(<=0表示连接断开或出错)
            if (recv_data_len <= 0) {
                // 关闭socket(释放连接资源)
                closesocket(g_sock);
                // 重置socket描述符为-1
                g_sock = -1;
                // 在LCD(0,190)位置更新显示"状态:未连接"
                lcd_display_string(0, 190, WHITE, BLACK, 16, "状态:未连接");
                // 释放tbuf内存
                free(tbuf);
                // 跳转到sock_start标签，重新发起连接
                goto sock_start;
            }
            // 动态分配120字节内存，用于格式化接收数据显示字符串
            char *buf = malloc(120);
            // 格式化接收数据字符串(如"接收数据:xxx")
            sprintf((char *)buf, "接收数据:%s", g_lwip_demo_recvbuf);
            // 在LCD(0,210)位置显示接收数据内容
            lcd_display_string(0, 210, WHITE, BLACK, 16, buf);
            // 释放buf内存(避免内存泄漏)
            free(buf);
            // 延时10ms，降低CPU占用
            vTaskDelay(10);
        }
    }
}

/**
 * @brief       发送数据线程函数
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
// 发送数据线程函数：独立线程处理TCP数据发送
void lwip_send_thread(void *pvParameters)
{
    // 未使用传入参数，避免编译器警告
    pvParameters = pvParameters;

    // 错误码变量，用于接收write函数返回值
    err_t err;

    // 线程主循环(永久运行)
    while (1) {
        // 内层循环：持续检查发送条件
        while (1) {
            // 判断发送条件：发送标志位被置位(LWIP_SEND_DATA)且连接已建立
            if (((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
                && (g_lwip_connect_state == 1)) /* 有数据要发送 */
            {
                // 发送数据：通过socket发送缓冲区内容，长度为发送缓冲区大小
                err = write(g_sock, g_lwip_demo_sendbuf, sizeof(g_lwip_demo_sendbuf));

                // 判断发送是否失败(err < 0表示发送错误)
                if (err < 0) {
                    // 跳出内层循环，执行错误处理
                    break;
                }

                // 清除发送标志位(重置发送状态)
                g_lwip_send_flag &= ~LWIP_SEND_DATA;
            }

            // 延时10ms，降低线程调度频率
            vTaskDelay(10);
        }

        // 发送失败时关闭socket(释放资源)
        closesocket(g_sock);
    }
}