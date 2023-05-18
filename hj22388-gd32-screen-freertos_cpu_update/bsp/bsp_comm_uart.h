/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_comm_uart.h
* 摘要：与上位机通信串口任务接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __BSP_COMM_UART_H
#define __BSP_COMM_UART_H

#include "common.h"


uint8_t comm_uart_init(void);
void StartCommTask(void *argument);




#endif

