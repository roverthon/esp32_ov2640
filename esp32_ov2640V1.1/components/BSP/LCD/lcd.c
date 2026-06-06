/**
 ****************************************************************************************************
* @file        lcd.c
* @author      正点原子团队(ALIENTEK)
* @version     V1.0
* @date        2023-08-26
* @brief       SPI LCD(MCU屏) 驱动代码
*              支持驱动IC型号包括:ILI9341等
*
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

#define __LCD_VERSION__  "1.0"

#include "lcd.h"
#include "lcdfont.h"


#define SPI_LCD_TYPE    1           /* SPI接口屏幕类型（1：2.4寸SPILCD  0：1.3寸SPILCD） */  

spi_device_handle_t MY_LCD_Handle;
uint8_t lcd_buf[LCD_TOTAL_BUF_SIZE];
lcd_obj_t lcd_self;
esp_lcd_panel_handle_t lcd_panel_handle = NULL;

/* LCD需要初始化一组命令/参数值。它们存储在此结构中  */
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; /* 数据中没有数据；比特7＝设置后的延迟；0xFF=cmds结束 */
} lcd_init_cmd_t;

/**
 * @brief       发送命令到LCD，使用轮询方式阻塞等待传输完成(由于数据传输量很少，因此在轮询方式处理可提高速度。使用中断方式的开销要超过轮询方式)
 * @param       cmd 传输的8位命令数据
 * @retval      无
 */
void lcd_write_cmd(const uint8_t cmd)
{
    LCD_WR(0);
    spi2_write_cmd(MY_LCD_Handle, cmd);
}

/**
 * @brief       发送数据到LCD，使用轮询方式阻塞等待传输完成(由于数据传输量很少，因此在轮询方式处理可提高速度。使用中断方式的开销要超过轮询方式)
 * @param       data 传输的8位数据
 * @retval      无
 */
void lcd_write_data(const uint8_t *data, int len)
{
    LCD_WR(1);
    spi2_write_data(MY_LCD_Handle, data, len);
}

/**
 * @brief       发送数据到LCD，使用轮询方式阻塞等待传输完成(由于数据传输量很少，因此在轮询方式处理可提高速度。使用中断方式的开销要超过轮询方式)
 * @param       data 传输的16位数据
 * @retval      无
 */
void lcd_write_data16(uint16_t data)
{
    uint8_t dataBuf[2] = {0,0};
    dataBuf[0] = data >> 8;
    dataBuf[1] = data & 0xFF;
    LCD_WR(1);
    spi2_write_data(MY_LCD_Handle, dataBuf,2);
}

/**
 * @brief       设置窗口大小
 * @param       xstar：左上角x轴
 * @param       ystar：左上角y轴
 * @param       xend：右下角x轴
 * @param       yend：右下角y轴
 * @retval      无
 */
void lcd_set_window(uint16_t xstar, uint16_t ystar,uint16_t xend,uint16_t yend)
{	
    uint8_t databuf[4] = {0,0,0,0};
    databuf[0] = xstar >> 8;
    databuf[1] = 0xFF & xstar;
    databuf[2] = xend >> 8;
    databuf[3] = 0xFF & xend;
    lcd_write_cmd(lcd_self.setxcmd);
    lcd_write_data(databuf,4);

    databuf[0] = ystar >> 8;
    databuf[1] = 0xFF & ystar;
    databuf[2] = yend >> 8;
    databuf[3] = 0xFF & yend;
    lcd_write_cmd(lcd_self.setycmd);
    lcd_write_data(databuf,4);

    lcd_write_cmd(lcd_self.wramcmd);    /* 开始写入GRAM */
}   

/**
 * @brief       以一种颜色清空LCD屏
 * @param       color 清屏颜色
 * @retval      无
 */
void lcd_clear(uint16_t color)
{
    uint16_t i, j;
    uint8_t data[2] = {0};

    data[0] = color >> 8;
    data[1] = color;
    
    lcd_set_window(0, 0, lcd_self.width - 1, lcd_self.height - 1);

    for(j = 0; j < LCD_BUF_SIZE / 2; j++)
    {
        lcd_buf[j * 2] =  data[0];
        lcd_buf[j * 2 + 1] =  data[1];
    }

    for(i = 0; i < (LCD_TOTAL_BUF_SIZE / LCD_BUF_SIZE); i++)
    {
        lcd_write_data(lcd_buf, LCD_BUF_SIZE);
    }
}

/**
 * @brief       在指定区域内填充单个颜色
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color:要填充的颜色(32位颜色,方便兼容LTDC)
 * @retval      无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint16_t i;
    uint16_t j;
    uint16_t width;
    uint16_t height;

    width = ex - sx + 1;
    height = ey - sy + 1;
    lcd_set_window(sx, sy, ex, ey);

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            lcd_write_data16(color);
        }
    }
    lcd_set_window(sx, sy, ex, ey);
}


/**
 * @brief       设置光标的位置
 * @param       Xpos：左上角x轴
 * @param       Ypos：左上角y轴
 * @retval      无
 */
void lcd_set_cursor(uint16_t xpos, uint16_t ypos)
{
    lcd_set_window(xpos,ypos,xpos,ypos);	
} 

/**
 * @brief       设置LCD的自动扫描方向(对RGB屏无效)
 * @param       dir:0~7,代表8个方向(具体定义见lcd.h)
 * @retval      无
 */
void lcd_scan_dir(uint8_t dir)
{
    uint8_t regval = 0;
    uint8_t dirreg = 0;
    uint16_t temp;

    /* 横屏时，对1963不改变扫描方向, 其他IC改变扫描方向！竖屏时1963改变方向, 其他IC不改变扫描方向 */
    if (lcd_self.dir == 1)
    {
        dir = 5;
    }

    /* 根据扫描方式 设置 0X36/0X3600 寄存器 bit 5,6,7 位的值 */
    switch (dir)
    {
        case L2R_U2D:                           /* 从左到右,从上到下 */
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;

        case L2R_D2U:                           /* 从左到右,从下到上 */
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;

        case R2L_U2D:                           /* 从右到左,从上到下 */
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;

        case R2L_D2U:                           /* 从右到左,从下到上 */
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;

        case U2D_L2R:                           /* 从上到下,从左到右 */
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;

        case U2D_R2L:                           /* 从上到下,从右到左 */
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;

        case D2U_L2R:                           /* 从下到上,从左到右 */
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;

        case D2U_R2L:                           /* 从下到上,从右到左 */
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
    }

    dirreg = 0x36;                              /* 对绝大部分驱动IC, 由0X36寄存器控制 */
    
    uint8_t date_send[1] = {regval};
    
    lcd_write_cmd(dirreg);
    lcd_write_data(date_send,1);
    
    if (regval & 0x20)
    {
        if (lcd_self.width < lcd_self.height)   /* 交换X,Y */
        {
            temp = lcd_self.width;
            lcd_self.width = lcd_self.height;
            lcd_self.height = temp;
        }
    }
    else
    {
        if (lcd_self.width > lcd_self.height)   /* 交换X,Y */
        {
            temp = lcd_self.width;
            lcd_self.width = lcd_self.height;
            lcd_self.height = temp;
        }
    }
    
    lcd_set_window(0, 0, lcd_self.width,lcd_self.height);
}

/**
 * @brief       设置LCD显示方向
 * @param       dir:0,竖屏; 1,横屏
 * @retval      无
 */
void lcd_display_dir(uint8_t dir)
{
    lcd_self.dir = dir;
    
    if (lcd_self.dir == 0)                  /* 竖屏 */
    {
        lcd_self.width      = 240;
        lcd_self.height     = 320;
        lcd_self.wramcmd    = 0X2C;
        lcd_self.setxcmd    = 0X2A;
        lcd_self.setycmd    = 0X2B;
    }
    else                                    /* 横屏 */
    {
        lcd_self.width      = 320;          /* 默认宽度 */
        lcd_self.height     = 240;          /* 默认高度 */
        lcd_self.wramcmd    = 0X2C;
        lcd_self.setxcmd    = 0X2A;
        lcd_self.setycmd    = 0X2B;
    }

    lcd_scan_dir(DFT_SCAN_DIR);             /* 默认扫描方向 */
}

/**
 * @brief       硬件复位
 * @param       self_in：LCD结构体
 * @retval      无
 */
void lcd_hard_reset(void)
{
    /* 复位显示屏 */
    LCD_RST(0);
    vTaskDelay(100);
    LCD_RST(1);
    vTaskDelay(100);
}

/**
 * @brief       绘画一个像素点
 * @param       self_in：LCD结构体
 * @param       x：x轴坐标
 * @param       y：y轴坐标
 * @param       color：颜色值
 * @retval      无
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_cursor(x, y);
    lcd_write_data16(color);
}

/**
 * @brief       画线函数(直线、斜线)
 * @param       x1,y1   起点坐标
 * @param       x2,y2   终点坐标
 * @param       color 填充颜色
 * @retval      无
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t; 
    int xerr = 0, yerr = 0, delta_x, delta_y, distance; 
    
    int incx, incy, urow, ucol; 

    delta_x = x2 - x1;                      /* 计算坐标增量 */
    delta_y = y2 - y1; 
    urow = x1; 
    ucol = y1; 
    
    if (delta_x > 0)
    {
        incx = 1;                           /* 设置单步方向 */
    }
    else if (delta_x == 0)
    {
        incx = 0;                           /* 垂直线 */
    }
    else
    {
        incx =-1;
        delta_x =-delta_x;
    } 
    if(delta_y > 0)
    {
        incy = 1; 
    }
    else if(delta_y == 0)
    {
        incy = 0;                           /* 水平线 */
    }
    else
    {
        incy =-1;
        delta_y=-delta_y;
    } 
    
    if( delta_x>delta_y)
    {
        distance = delta_x;                 /* 选取基本增量坐标轴 */
    }
    else
    {
        distance = delta_y; 
    }
    
    for (t = 0;t <= distance + 1;t++ )      /* 画线输出 */
    {
        lcd_draw_pixel(urow,ucol,color);    /* 画点 */ 
        xerr += delta_x ; 
        yerr += delta_y ; 
        
        if(xerr>distance)
        { 
            xerr -= distance; 
            urow += incx; 
        } 
        
        if (yerr > distance)
        { 
            yerr -= distance; 
            ucol += incy; 
        } 
    } 
}

/**
 * @brief       画水平线
 * @param       x0,y0: 起点坐标
 * @param       len  : 线长度
 * @param       color: 矩形的颜色
 * @retval      无
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcd_self.width) || (y > lcd_self.height))return;

    lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * @brief       画一个矩形
 * @param       x1,y1   起点坐标
 * @param       x2,y2   终点坐标
 * @param       color 填充颜色
 * @retval      无
 */
void lcd_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,uint16_t color)
{
    lcd_draw_line(x0, y0, x1, y0,color);
    lcd_draw_line(x0, y0, x0, y1,color);
    lcd_draw_line(x0, y1, x1, y1,color);
    lcd_draw_line(x1, y0, x1, y1,color);
}

/**
 * @brief       画一个圆
 * @param       x0,y0   圆心坐标
 * @param       r   圆半径
 * @param       color 填充颜色
 * @retval      无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);

    while (a <= b)
    {
        lcd_draw_pixel(x0 - b, y0 - a, color);
        lcd_draw_pixel(x0 + b, y0 - a, color);
        lcd_draw_pixel(x0 - a, y0 + b, color);
        lcd_draw_pixel(x0 - b, y0 - a, color);
        lcd_draw_pixel(x0 - a, y0 - b, color);
        lcd_draw_pixel(x0 + b, y0 + a, color);
        lcd_draw_pixel(x0 + a, y0 - b, color);
        lcd_draw_pixel(x0 + a, y0 + b, color);
        lcd_draw_pixel(x0 - b, y0 + a, color);
        a++;

        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }

        lcd_draw_pixel(x0 + a, y0 + b, color);
    }
}

/**
 * @brief       在指定位置显示一个字符
 * @param       x,y  : 坐标
 * @param       chr  : 要显示的字符:" "--->"~"
 * @param       size : 字体大小 12/16/24/32
 * @param       mode : 叠加方式(1); 非叠加方式(0);
 * @param       color : 字符的颜色;
 * @retval      无
 */
void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp = 0,t1 = 0, t = 0;
    uint8_t *pfont = 0;
    uint8_t csize = 0;                                      /* 得到字体一个字符对应点阵集所占的字节数 */
    uint16_t colortemp = 0;
    uint8_t sta = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = chr - ' ';                                        /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

    if ((x > (lcd_self.width - size / 2)) || (y > (lcd_self.height - size)))
    {
        return;
    }

    lcd_set_window(x, y, x + size / 2 - 1, y + size - 1);   /* (x,y,x+8-1,y+16-1) */

    switch (size)
    {
        case 12:
            pfont = (uint8_t *)asc2_1206[chr];              /* 调用1206字体 */
            sta = 6;
            break;

        case 16:
            pfont = (uint8_t *)asc2_1608[chr];              /* 调用1608字体 */
            sta = 8;
            break;

        case 24:
            pfont = (uint8_t *)asc2_2412[chr];              /* 调用2412字体 */
            break;

        case 32:
            pfont = (uint8_t *)asc2_3216[chr];              /* 调用3216字体 */
            sta = 8;
            break;

        default:
            return ;
    }

    if (size != 24)
    {
        csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
        
        for (t = 0; t < csize; t++)
        {
            temp = pfont[t];                                /* 获取字符的点阵数据 */

            for (t1 = 0; t1 < sta; t1++)
            {
                if (temp & 0x80)
                {
                    colortemp = color;
                }
                else if (mode == 0)                     /* 无效点,不显示 */
                {
                    colortemp = 0xFFFF;
                }

                lcd_write_data16(colortemp);
                temp <<= 1;
            }
        }
    }
    else
    {
        csize = (size * 16) / 8;
        
        for (t = 0; t < csize; t++)
        {
            temp = asc2_2412[chr][t];

            if (t % 2 == 0)
            {
                sta = 8;
            }
            else
            {
                sta = 4;
            }

            for (t1 = 0; t1 < sta; t1++)
            {
                if(temp & 0x80)
                {
                    colortemp = color;
                }
                else if (mode == 0)                         /* 无效点,不显示 */
                {
                    colortemp = 0xFFFF;
                }

                lcd_write_data16(colortemp);
                temp <<= 1;
            }
        }
    }
}

/**
 * @brief       m^n函数
 * @param       m,n     输入参数
 * @retval      m^n次方
 */
uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while(n--)result *= m;

    return result;
}

/**
 * @brief       显示len个数字
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @retval      无
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)                                               /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;                       /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))                                   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color);    /* 显示空格,占位 */
                continue;                                                   /* 继续下个一位 */
            }
            else
            {
                enshow = 1;                                                 /* 使能显示 */
            }

        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color);     /* 显示字符 */
    }
}

/**
 * @brief       扩展显示len个数字(高位是0也显示)
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @param       mode: 显示模式
 *              [7]:0,不填充;1,填充0.
 *              [6:1]:保留
 *              [0]:0,非叠加显示;1,叠加显示.
 * @param       color : 数字的颜色;
 * @retval      无
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)                                                           /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;                                   /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))                                               /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                if (mode & 0X80)                                                        /* 高位需要填充0 */
                {
                    lcd_show_char(x + (size / 2)*t, y, '0', size, mode & 0X01, color);  /* 用0占位 */
                }
                else
                {
                    lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0X01, color);  /* 用空格占位 */
                }
                continue;
            }
            else
            {
                enshow = 1;                                                             /* 使能显示 */
            }
        }
        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0X01, color);
    }
}


/**
 * @brief       显示字符串
 * @param       x,y         : 起始坐标
 * @param       width,height: 区域大小
 * @param       size        : 选择字体 12/16/24/32
 * @param       p           : 字符串首地址
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)break;  /* 退出 */

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}

/**
 * @brief       打开LCD
 * @param       self_in：SPI控制块
 * @retval      mp_const_none：初始化成功
 */
void lcd_on(void)
{
    LCD_PWR(1);
    vTaskDelay(10);
}

/**
 * @brief       关闭LCD
 * @param       self_in：SPI控制块
 * @retval      mp_const_none：初始化成功
 */
void lcd_off(void)
{
    LCD_PWR(0);
    vTaskDelay(10);
}

/**
 * @brief       LCD初始化
 * @param       无
 * @retval      无
 */
void lcd_init(void)
{
    int cmd = 0;
    esp_err_t ret = 0;
    
    lcd_self.dir = 0;
    lcd_self.wr = LCD_NUM_WR;                                       /* 配置WR引脚 */
    lcd_self.cs = LCD_NUM_CS;                                       /* 配置CS引脚 */
    
    gpio_config_t gpio_init_struct;

    /* SPI驱动接口配置 */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 60 * 1000 * 1000,                         /* SPI时钟 */
        .mode = 0,                                                  /* SPI模式0 */
        .spics_io_num = lcd_self.cs,                                /* SPI设备引脚 */
        .queue_size = 7,                                            /* 事务队列尺寸 7个 */
    };
    
    /* 添加SPI总线设备 */
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &MY_LCD_Handle);   /* 配置SPI总线设备 */
    ESP_ERROR_CHECK(ret);

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;                 /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_OUTPUT;                       /* 配置输出模式 */
    gpio_init_struct.pin_bit_mask = 1ull << lcd_self.wr;            /* 配置引脚位掩码 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;          /* 失能下拉 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;               /* 使能下拉 */
    gpio_config(&gpio_init_struct);                                 /* 引脚配置 */

    lcd_hard_reset();                                               /* LCD硬件复位 */

    /* 初始化代码 */
#if SPI_LCD_TYPE                                                    /* 对2.4寸LCD寄存器进行设置 */
    lcd_init_cmd_t ili_init_cmds[] =
    {
        {0x11, {0}, 0x80},
        {0x36, {0x00}, 1},
        {0x3A, {0x65}, 1},
        {0X21, {0}, 0x80},
        {0x29, {0}, 0x80},
        {0, {0}, 0xff},
    };

#else                                                               /* 不为0则视为使用1.3寸SPILCD屏，那么屏幕将不会反显 */
    lcd_init_cmd_t ili_init_cmds[] =
    {
        {0x11, {0}, 0x80},
        {0x36, {0x00}, 1},
        {0x3A, {0x65}, 1},
        {0xB2, {0x0C, 0x0C, 0x00, 0x33,0x33}, 5},
        {0xB7, {0x75}, 1},
        {0xBB, {0x1C}, 1},
        {0xC0, {0x2c}, 1},
        {0xC2, {0x01}, 1},
        {0xC3, {0x0F}, 1},
        {0xC4, {0x20}, 1},
        {0xC6, {0X01}, 1},
        {0xD0, {0xA4,0xA1}, 2},
        {0xE0, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 14},
        {0xE1, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 14},
        {0X21, {0}, 0x80},
        {0x29, {0}, 0x80},
        {0, {0}, 0xff},
    };
#endif

    /* 循环发送设置所有寄存器 */
    while (ili_init_cmds[cmd].databytes != 0xff)
    {
        lcd_write_cmd(ili_init_cmds[cmd].cmd);
        lcd_write_data(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
        
        if (ili_init_cmds[cmd].databytes & 0x80)
        {
            vTaskDelay(120);
        }
        
        cmd++;
    }

    lcd_display_dir(1);                                             /* 设置屏幕方向 */
    LCD_PWR(1);
    lcd_clear(WHITE);                                               /* 清屏 */
}

/*----------------------------------------
 * @brief       显示图片
 * @param       x_s：图片的X轴起始坐标
 * @param       y_s：图片的Y轴起始坐标
 * @param       width：图片的宽度
 * @param       height：图片的高度
 * @param       pic：图片数据（RGB565格式）
 * @retval      无
----------------------------------------*/
void lcd_display_picture(uint32_t x_s, uint32_t y_s, uint32_t width, uint32_t height, uint8_t *pic)
{
    // 声明指向图片数据的指针p，用于遍历像素数据
    uint8_t *p = pic;
   
    // 声明循环计数器及缓冲区计算变量（i:循环变量，m:完整缓冲区块数，n:剩余字节数）
    uint32_t i,m,n = 0;

    // 设置LCD显示窗口为图片的显示区域（从(x_s,y_s)到(x_s+width-1,y_s+height-1)）
    lcd_set_window(x_s, y_s, x_s + width - 1, y_s + height - 1);

    // 循环遍历图片数据（每个像素为2字节RGB565格式，总字节数=width*height*2）
    // i每次递增2，处理一个像素的2个字节
    // 使用PortHelper执行图片取模时更改输出大小端模式，可忽略下方的交换字节序代码
    for (i = 0; i < width * height *2; i+=2) 
    {
        // 将图片数据的高字节存入lcd_buf当前位置（交换字节顺序以匹配LCD要求）
        lcd_buf[i ]  = p[i +1];
        // 将图片数据的低字节存入lcd_buf下一个位置（完成RGB565格式的字节调整）
        lcd_buf[i+1] = p[i];
    }

    // 计算完整的LCD缓冲区块数（LCD_BUF_SIZE为单次可发送的最大字节数）
    m = width * height * 2/ LCD_BUF_SIZE;
    // 计算剩余字节数（总字节数除以缓冲区大小的余数）
    n = width * height * 2 % LCD_BUF_SIZE;

    // 循环发送所有完整的缓冲区数据块
    for (i = 0; i < m; i++) 
    {
        // 发送第i个缓冲区数据块，起始地址为lcd_buf[i * LCD_BUF_SIZE]，长度为LCD_BUF_SIZE
        lcd_write_data(&lcd_buf[i * LCD_BUF_SIZE], LCD_BUF_SIZE);
    }
    // 如果存在剩余字节（n>0），发送剩余数据
    if(n)
    {
        // 发送剩余数据，起始地址为lcd_buf[m * LCD_BUF_SIZE]，长度为n
        lcd_write_data(&lcd_buf[m * LCD_BUF_SIZE], n);
    }
}

/*----------------------------------------
 * @brief       显示一个字符
 * @param       x：字符的X轴坐标
 * @param       y：字符的Y轴坐标
 * @param       fc：字符的前景色
 * @param       bc：字符的背景色
 * @param       font_size：字符的大小
 * @param       index：字符的索引
 * @retval      无
----------------------------------------*/
void lcd_display_char(uint32_t x, uint32_t y, uint32_t fc, uint32_t bc, uint32_t font_size,
                      uint32_t index)
{
    // 声明循环计数器i和j，用于遍历字体点阵数据
    uint32_t i = 0, j = 0;
    // 保存字符显示区域的起始X坐标
    uint32_t x_s = x;
    // 计算字符显示区域的结束X坐标（起始X + 字体大小 - 1）
    uint32_t x_e = x + font_size - 1;
    // 保存字符显示区域的起始Y坐标
    uint32_t y_s = y;
    // 计算字符显示区域的结束Y坐标（起始Y + 字体大小 - 1）
    uint32_t y_e = y + font_size - 1;
    // 声明临时变量tmp，用于存储当前读取的字体点阵数据字节
    uint8_t tmp = 0;

    // 设置LCD显示窗口为当前字符的显示区域
    lcd_set_window(x_s, y_s, x_e, y_e);

    // 循环读取字体点阵数据，总字节数 = (字体大小 * 字体大小) / 8（每个字节表示8个像素点）
    for (i = 0; i < font_size * font_size / 8; i++) 
    {
        // 如果字体大小为16x16，从16点阵字体数据数组中读取对应索引的点阵数据
        if (font_size == 16)
            tmp = g_font_dot_matrix_16[index][i];
        // 如果字体大小为32x32，从32点阵字体数据数组中读取对应索引的点阵数据
        else if (font_size == 32)
            tmp = g_font_dot_matrix_32[index][i];

        // 循环处理当前字节的8个比特位（每个比特位对应一个像素点）
        for (j = 0; j < 8; j++) {
            // 如果当前比特位为1，向LCD写入前景色像素数据
            if (tmp & (1 << j)) {
                lcd_write_data16(fc);
            } 
            // 如果当前比特位为0，向LCD写入背景色像素数据
            else {
                lcd_write_data16(bc);
            }
        }
    }
}

/*----------------------------------------
 * @brief       获取UTF-8字符字节长度
 * @param       target：UTF-8字符
 * @retval      字符字节长度
----------------------------------------*/
int GetUtf8CharLength(char *target)
{
    // 判断是否为单字节UTF-8字符（0xxxxxxx格式）
    if (target[0] <= 0x7F) {
        return 1;
    } 
    // 判断是否为双字节UTF-8字符（110xxxxx 10xxxxxx格式）
    else if ((target[0] >= 0xC2 && target[0] <= 0xDF) && (target[1] & 0xC0) == 0x80) {
        return 2;
    } 
    // 判断是否为三字节UTF-8字符（1110xxxx 10xxxxxx 10xxxxxx格式）
    else if ((target[0] >= 0xE0 && target[0] <= 0xEF) && (target[1] & 0xC0) == 0x80
               && (target[2] & 0xC0) == 0x80) {
        return 3;
    } 
    // 判断是否为四字节UTF-8字符（11110xxx 10xxxxxx 10xxxxxx 10xxxxxx格式）
    else if ((target[0] >= 0xF0 && target[0] <= 0xF7) && (target[1] & 0xC0) == 0x80
               && (target[2] & 0xC0) == 0x80 && (target[3] & 0xC0) == 0x80) {
        return 4;
    }
    // 默认返回单字节（非法UTF-8字符处理）
    return 1;
}

/*----------------------------------------
 * @brief       查找字体索引
 * @param       target：UTF-8字符
 * @param       font_size：字体大小
 * @retval      字体索引
----------------------------------------*/
int FindFontIndex(char *target, uint32_t font_size)
{
    // 声明循环变量i，用于遍历字体索引表
    int i;

    // 获取目标UTF-8字符的字节长度
    int char_len = GetUtf8CharLength(target);

    // 如果字体大小为16x16，在16点阵字体索引表中查找匹配字符
    if (font_size == 16) {
        // 遍历16点阵字体索引表，比较字符与索引表中的条目
        for (i = 0;
             i < (sizeof(g_font_dot_matrix_16_index) / sizeof(g_font_dot_matrix_16_index[0]));
             i++) {
            // 找到匹配字符时返回对应的索引值
            if (strncmp(g_font_dot_matrix_16_index[i], (char *)target, char_len) == 0) {
                return i;
            }
        }
    }   

    // 如果字体大小为32x32，在32点阵字体索引表中查找匹配字符
    if (font_size == 32) {
        // 遍历32点阵字体索引表，比较字符与索引表中的条目
        for (i = 0;
             i < (sizeof(g_font_dot_matrix_32_index) / sizeof(g_font_dot_matrix_32_index[0]));
             i++) {
            // 找到匹配字符时返回对应的索引值
            if (strncmp(g_font_dot_matrix_32_index[i], (char *)target, char_len) == 0) {
                return i;
            }
        }
    }

    // 未找到匹配字符，返回-1
    return -1;
}

/*----------------------------------------
 * @brief       显示字符串
 * @param       x：字符串的X轴坐标
 * @param       y：字符串的Y轴坐标
 * @param       fc：字符串的前景色
 * @param       bc：字符串的背景色
 * @param       font_size：字符串的大小
 * @param       str：字符串
 * @retval      无
----------------------------------------*/
void lcd_display_string(uint32_t x, uint32_t y, uint32_t fc, uint32_t bc, uint32_t font_size,
                        char *str)
{
    // 保存字符串显示的起始X坐标，用于自动换行时重置X坐标
    uint32_t x_s = x;
    // 声明循环变量i，用于遍历字符串中的字符
    uint32_t i = 0;
    // 声明字体索引变量font_index，用于存储字符在字体表中的索引
    int32_t font_index = 0;

    // 循环遍历字符串，直到遇到结束符'\0'
    while (str[i] != '\0') {
        // 查找当前字符在字体索引表中的索引值
        font_index = FindFontIndex(&str[i], font_size);

        // 如果未找到对应字体索引（字符不存在于字体表中），移动到下一个字符并继续循环
        if (font_index < 0) {
            i++;
            continue;
        }

        // 在当前坐标显示字符，使用获取到的字体索引
        lcd_display_char(x, y, fc, bc, font_size, font_index);

        // 判断当前字符是否为中文字符（ASCII码大于0x7F）
        if (str[i] > 0x7F) {
            // 中文字符宽度等于字体大小，X坐标偏移字体大小
            x += font_size;
            // UTF-8编码的中文字符占用3个字节，索引i向后移动3位
            i += 3;
        } 
        // 当前字符为ASCII字符（非中文字符）
        else {
            // ASCII字符宽度为字体大小的一半，X坐标偏移字体大小的一半
            x += font_size / 2;
            // ASCII字符占用1个字节，索引i向后移动1位
            i += 1;
        }

        // 判断当前X坐标加上字体大小是否超出LCD宽度，若超出则自动换行
        if ((x + font_size) >= lcd_self.width) {
            // 重置X坐标为起始X
            x = x_s;
            // Y坐标偏移字体大小，实现换行
            y += font_size;
        }
    }
}

/*----------------------------------------
 * @brief       显示格式化字符串
 * @param       x：字符串的X轴坐标
 * @param       y：字符串的Y轴坐标
 * @param       fc：字符串的前景色
 * @param       bc：字符串的背景色
 * @param       font_size：字符串的大小
 * @param       str：格式化字符串模板
 * @retval      无
 * @note        支持可变参数列表的格式化字符串显示
----------------------------------------*/
void lcd_display_string_fmt(uint32_t x, uint32_t y, uint32_t fc, uint32_t bc, uint32_t font_size,
                        char *str, ...)
{
    // 声明可变参数列表变量args
    va_list args;
    // 声明临时缓冲区temp_buf，用于存储格式化后的字符串（大小256可根据需求调整）
    char temp_buf[256]; 
    // 初始化可变参数列表，指定最后一个固定参数为str
    va_start(args, str);
    // 使用vsprintf将格式化字符串输出到临时缓冲区temp_buf
    vsprintf(temp_buf, str, args);  
    // 结束可变参数列表的使用
    va_end(args);
    // 调用lcd_display_string显示格式化后的字符串
    lcd_display_string(x, y, fc, bc, font_size, temp_buf);  
}