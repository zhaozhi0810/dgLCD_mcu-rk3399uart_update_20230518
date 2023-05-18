/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：serial_frame.h
* 摘要：串口数据解析函数声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __SERIAL_FRAME_H
#define __SERIAL_FRAME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERIAL_FRAME_HEAD 0xA55A
#define SERIAL_FRAME_SIZE 16
#define SERIAL_FRAME_EXCHANGE 0x77

enum
{
    SF_TYPE_ORIGIN = 0x00,
    SF_TYPE_PROTO = 0x01,
    SF_TYPE_JSON = 0x02,
};

typedef struct
{
    uint8_t type;
    uint16_t length;
    uint8_t data[SERIAL_FRAME_SIZE];
    uint16_t crc;
} serial_frame_t;

typedef void(*sf_callback)(void *);
typedef struct
{
    sf_callback cb;
    uint8_t type;
    uint8_t is_valid_frame: 1;
    uint8_t step;
    uint16_t data_cnt;
    serial_frame_t frame;
} sf_info_t;

void sf_register_cb(sf_callback cb);
void sf_get_char(uint8_t ch);
uint8_t sf_is_vailed_frame(void);
serial_frame_t *sf_get_frame(void);
uint16_t sf_packet_data(uint8_t *buf, uint16_t buf_size, uint16_t data_size);

#ifdef __cplusplus
}
#endif

#endif
