/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：ringbuffer.h
* 摘要：循环buf接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <stdint.h>

#define RINGBUFFER_SIZE     64

typedef struct
{
    uint16_t rd_pos;    // 读指针
    uint16_t wr_pos;    // 写指针
    uint16_t used;      // 可用字节数
    uint16_t capacity;  // 缓存区容量
    uint8_t buf[RINGBUFFER_SIZE]; // 缓存区
} ringbuffer_t;

void ringbuffer_init(ringbuffer_t *rb);
uint8_t ringbuffer_is_empty(const ringbuffer_t *const rb);
uint16_t ringbuffer_push_back(ringbuffer_t *const rb, const uint8_t *const buf, uint16_t size);
uint16_t ringbuffer_pop_front(ringbuffer_t *const rb, uint8_t *const buf, uint16_t size);

uint16_t ringbuffer_push_back_irq(ringbuffer_t *const rb, const uint8_t *const buf, uint16_t size);

#endif

