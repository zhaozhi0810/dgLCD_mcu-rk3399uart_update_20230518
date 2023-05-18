/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp.h
* 摘要：板卡资源接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __BSP_H
#define __BSP_H

#include "common.h"
#include "bsp_debug_uart.h"
#include "bsp_comm_uart.h"
#include "bsp_i2c_gpio.h"
#include "bsp_pwm.h"
#include "bsp_24cxx.h"
#include "serial_frame.h"



extern const char* g_build_time_str;


uint8_t bsp_init(void);
void delay_ms(int32_t ms);

void bsp_led_toggle(void);
void bsp_led_set(uint8_t enable);
void bsp_heat_set(uint8_t enable);

void bsp_ts_reset(void);
uint32_t bsp_get_tick(void);
void start_freertos(void);

extern uint8_t cli_is_print(void);
uint8_t GetMcuVersion(void);   //获得软件版本
#endif
