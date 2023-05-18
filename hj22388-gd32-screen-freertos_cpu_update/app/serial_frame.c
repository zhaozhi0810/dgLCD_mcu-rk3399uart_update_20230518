/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：serial_frame.c
* 摘要：串口数据解析
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "serial_frame.h"
#include <string.h>

static sf_info_t g_sf_info = {0};

// 给g_sf_info的cb字段赋值
void sf_register_cb(sf_callback cb)
{
    g_sf_info.cb = cb;
}

/*
* 函数介绍：串口数据解析
* 参数：ch:一个字节的串口数据
* 返回值：无
* 备注：
*/
void sf_get_char(uint8_t ch)
{
    switch (g_sf_info.step)
    {
    /* start */
    case 0:
        if (ch == (SERIAL_FRAME_HEAD >> 8))  //帧头1
            g_sf_info.step++;
        else
            g_sf_info.step = 0;
        break;
    case 1:
        if (ch == (SERIAL_FRAME_HEAD & 0x0ff))  //帧头2
        {
            g_sf_info.step++;
            g_sf_info.data_cnt = 0;
            memset(&g_sf_info.frame, 0, sizeof(g_sf_info.frame));
            g_sf_info.frame.length = 5;   //设置数据长度
        }
        else
        {
            g_sf_info.step = 0;
        }
        break;
    /* data */
    case 2:
        g_sf_info.frame.data[g_sf_info.data_cnt] = ch;
        g_sf_info.data_cnt++;
        if (g_sf_info.data_cnt >= g_sf_info.frame.length)  //收到的数据满足长度了
        {
            g_sf_info.step++;  //下一步
        }
        break;
    /* crc */
    case 3:
        g_sf_info.frame.crc = ch;
        g_sf_info.step = 0;
        if (g_sf_info.cb)
        {
            g_sf_info.cb((void *)&g_sf_info.frame);
        }
        break;
    default:
        g_sf_info.step = 0;
        break;
    }
}

// 标注此帧是否有用
uint8_t sf_is_vailed_frame(void)
{
    return g_sf_info.is_valid_frame;
}

// 获取一帧数据
serial_frame_t *sf_get_frame(void)
{
    return &g_sf_info.frame;
}

/*
* 函数介绍：校验和计算
* 参数：buf：传出参数，携带数据
*       buf_size：缓存区长度
*       data_size：数据长度
* 返回值：uint16_t：返回数据长度
* 备注：
*/
uint16_t sf_packet_data(uint8_t *buf, uint16_t buf_size, uint16_t data_size)
{
    uint16_t temp = 0;
    if (buf_size < data_size)
    {
        return 0;
    }
    // crc
    for (int i = 2; i < (data_size - 1); i++)
    {
        temp += buf[i];
    }
    buf[data_size - 1] = temp & 0x0ff;
    return data_size;
}
