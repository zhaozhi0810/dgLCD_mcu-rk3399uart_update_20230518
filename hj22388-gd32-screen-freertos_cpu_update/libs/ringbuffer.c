/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：ringbuffer.c
* 摘要：循环buf接口定义
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "ringbuffer.h"
#include <stdio.h>
#include <string.h>

/*
* 函数介绍：初始化
* 参数：rb：循环buf
* 返回值：无
* 备注：无
*/
void ringbuffer_init(ringbuffer_t *rb)
{
    memset(rb, 0, sizeof(ringbuffer_t));
    rb->capacity = RINGBUFFER_SIZE;
}

/*
* 函数介绍：判断buf是否为空
* 参数：rb：循环buf
* 返回值：0：不为空；1：为空
* 备注：无
*/
uint8_t ringbuffer_is_empty(const ringbuffer_t *const rb)
{
    if (rb->used > 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


/*
* 函数介绍：将新数据加入在循环buf中
* 参数：rb：循环buf
*      buf：新数据
*     size：新数据长度
* 返回值：uint16_t：写入循环buf的数据长度
* 备注：无
*/
uint16_t ringbuffer_push_back(ringbuffer_t *const rb, const uint8_t *const buf, uint16_t size)
{
    uint16_t wr_size = size;
    // 缓存区中可用字节数大于缓存区容量
    if (rb->used >= rb->capacity)
    {
        return 0;
    }
    // 缓存区中可用字节数加要写入字节数大于缓存区容量
    else if (rb->used + size > rb->capacity)
    {
        wr_size = rb->capacity - rb->used;
    }
    else
    {

    }
    // 失能中断
    __disable_irq();

    uint16_t i, wr_cnt = 0;
    // 写指针位置+要写入字节数大于缓存区容量；则需分段写
    if (rb->wr_pos + wr_size >= rb->capacity)
    {
        for (i = rb->wr_pos; i < rb->capacity; i++)
        {
            rb->buf[i] = buf[wr_cnt];
            wr_cnt++;
        }
        // 分段写，将未写完的字节写在缓存区头部
        for (i = 0; i < wr_size - wr_cnt; i++)
        {
            rb->buf[i] = buf[wr_cnt + i];
        }

        rb->used += wr_size;
        rb->wr_pos = rb->wr_pos + wr_size - rb->capacity;
    }
    else
    {
        // 一次性写完
        for (i = rb->wr_pos; i < rb->wr_pos + wr_size; i++)
        {
            rb->buf[i] = buf[wr_cnt];
            wr_cnt++;
        }

        rb->used += wr_size;
        rb->wr_pos += wr_size;
    }
    __enable_irq();

    return wr_size;
}

/*
* 函数介绍：将新数据加入在循环buf中
* 参数：rb：循环buf
*      buf：新数据
*     size：新数据长度
* 返回值：uint16_t：写入循环buf的数据长度
* 备注：无
*/
uint16_t ringbuffer_push_back_irq(ringbuffer_t *const rb, const uint8_t *const buf, uint16_t size)
{
    uint16_t wr_size = size;
    // 缓存区中可用字节数大于缓存区容量
    if (rb->used >= rb->capacity)
    {
        return 0;
    }
    // 缓存区中可用字节数加要写入字节数大于缓存区容量
    else if (rb->used + size > rb->capacity)
    {
        wr_size = rb->capacity - rb->used;
    }
    else
    {

    }

    uint16_t i, wr_cnt = 0;
    // 写指针位置+要写入字节数大于缓存区容量；则需分段写
    if (rb->wr_pos + wr_size >= rb->capacity)
    {
        for (i = rb->wr_pos; i < rb->capacity; i++)
        {
            rb->buf[i] = buf[wr_cnt];
            wr_cnt++;
        }
        // 分段写，将未写完的字节写在缓存区头部
        for (i = 0; i < wr_size - wr_cnt; i++)
        {
            rb->buf[i] = buf[wr_cnt + i];
        }

        rb->used += wr_size;
        rb->wr_pos = rb->wr_pos + wr_size - rb->capacity;
    }
    else
    {
        // 一次性写完
        for (i = rb->wr_pos; i < rb->wr_pos + wr_size; i++)
        {
            rb->buf[i] = buf[wr_cnt];
            wr_cnt++;
        }

        rb->used += wr_size;
        rb->wr_pos += wr_size;
    }

    return wr_size;
}

/*
* 函数介绍：从循环buf中弹出数据
* 参数：rb：循环buf
*      buf：传出参数
*     size：传出参数长度
* 返回值：uint16_t：传出数据长度
* 备注：无
*/
uint16_t ringbuffer_pop_front(ringbuffer_t *const rb, uint8_t *const buf, uint16_t size)
{
    uint16_t rd_size = size;
    // 可用字节数为0，直接返回
    if (rb->used == 0)
    {
        return 0;
    }
    else if (size > rb->used)
    {
        rd_size = rb->used;
    }
    else
    {

    }

    __disable_irq();

    uint16_t i, rd_cnt = 0;
    // 读指针位置+读字节数大于缓存区容量
    // 从都指针位置开始读到缓存区结尾，再从头部读
    if (rb->rd_pos + rd_size >= rb->capacity)
    {
        for (i = rb->rd_pos; i < rb->capacity; i++)
        {
            buf[rd_cnt] = rb->buf[i];
            rd_cnt++;
        }
        // 分段读，从缓存区头部开始读
        for (i = 0; i < rd_size - rd_cnt; i++)
        {
            buf[rd_cnt + i] = rb->buf[i];
        }

        rb->used -= rd_size;
        rb->rd_pos = rd_size - rd_cnt;
    }
    else
    {
        for (i = rb->rd_pos; i < rb->rd_pos + rd_size; i++)
        {
            buf[rd_cnt] = rb->buf[i];
            rd_cnt++;
        }

        rb->used -= rd_size;
        rb->rd_pos += rd_size;
    }

    __enable_irq();

    return rd_size;
}



#if 0  // 测试用
/*
* 函数介绍：将循环buf的信息显示出来
* 参数：rb：循环buf
* 返回值：无
* 备注：无
*/
static void ringbuffer_show(ringbuffer_t *rb)
{
    printf("wr_pos: %d\r\n", rb->wr_pos);
    printf("rd_pos: %d\r\n", rb->rd_pos);
    printf("used: %d\r\n", rb->used);
    printf("capacity: %d\r\n", rb->capacity);
    printf("buf: %s\r\n", rb->buf);
    printf("\r\n");
}


void ringbuffer_test(void)
{
    ringbuffer_t rb;
    uint8_t str1[32] = "";
    uint8_t str2[32] = "";
    uint8_t str3[64] = "";

    for (uint8_t i = 0; i < 24; i++)
    {
        str1[i] = 'a' + i;
    }

    for (uint8_t i = 0; i < 10; i++)
    {
        str2[i] = '0' + i;
    }

    ringbuffer_init(&rb);

    uint8_t ret;

    ret = ringbuffer_pop_front(&rb, str3, 1);
    printf("pop front [%s] from ringbuffer, ret: %d\r\n", str3, ret);
    ringbuffer_show(&rb);

    ret = ringbuffer_push_back(&rb, str1, 12);
    printf("push back [%s] to ringbuffer, ret: %d\r\n", str1, ret);
    ringbuffer_show(&rb);

    ret = ringbuffer_push_back(&rb, str2, sizeof(str2));
    printf("push back [%s] to ringbuffer, ret: %d\r\n", str2, ret);
    ringbuffer_show(&rb);

    memset(str3, 0, sizeof(str3));
    ret = ringbuffer_pop_front(&rb, str3, 12);
    printf("pop front [%s] from ringbuffer, ret: %d\r\n", str3, ret);
    ringbuffer_show(&rb);

    memset(str3, 0, sizeof(str3));
    ret = ringbuffer_pop_front(&rb, str3, 6);
    printf("pop front [%s] from ringbuffer, ret: %d\r\n", str3, ret);
    ringbuffer_show(&rb);
}
#endif
