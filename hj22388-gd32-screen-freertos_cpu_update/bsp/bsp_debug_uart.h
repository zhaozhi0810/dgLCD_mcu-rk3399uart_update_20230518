/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_debug_uart.h
* 摘要：调试串口任务接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __BSP_DEBUG_UART_H
#define __BSP_DEBUG_UART_H

#include "common.h"

uint8_t debug_uart_init(void);
void StartDebugTask(void *argument);
uint8_t uart_init(user_uart_cfg_t *cfg,uint8_t );

#endif
