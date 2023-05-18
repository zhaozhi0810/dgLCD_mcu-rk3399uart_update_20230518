/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：common.h
* 摘要：通用变量、函数声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef GD32F103
#define GD32F103
#endif
#ifndef USE_STDPERIPH_DRIVER
#define USE_STDPERIPH_DRIVER
#endif

#include "gd32f10x.h"
#include "gd32f10x_libopt.h"
#include "version.h"
#include "version_info.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "nr_micro_shell.h"
#include "ringbuffer.h"

typedef struct
{
    uint32_t com;
    rcu_periph_enum clk;
    IRQn_Type irq;

    uint32_t af;
    rcu_periph_enum tx_port_clk;
    uint32_t tx_port;
    uint32_t tx_pin;
    rcu_periph_enum rx_port_clk;
    uint32_t rx_port;
    uint32_t rx_pin;
} user_uart_cfg_t;

typedef struct
{
    uint8_t id;
    rcu_periph_enum clk;
    uint32_t port;
    uint32_t pin;
    uint8_t pin_source;
    uint8_t port_source;
    exti_line_enum exti_line;
    uint8_t valied;
    IRQn_Type irq;
} user_gpio_cfg_t;

#define GPIO_USER_CONFIG(port, pin) RCU_##port, port, GPIO_PIN_##pin, EXTI_SOURCE_PIN##pin, EXTI_SOURCE_##port, EXTI_##pin


#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define force_inline inline __attribute__((always_inline))
#define no_inline __attribute__((noinline))
#define __maybe_unused __attribute__((unused))

#define tostring(s)     #s
#define stringify(s)    tostring(s)

#ifndef offsetof
#define offsetof(type, field) ((size_t) &((type *)0)->field)
#endif
#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif


#define ERR_LOG_LEVEL     1
#define INFO_LOG_LEVEL    2
#define DEBUG_LOG_LEVEL   3
#define DEFAULT_LOG_LEVEL INFO_LOG_LEVEL

#define ApplicationAddress    0x8006000
#define PAGE_SIZE             (0x400)    /* 1 Kbyte */

extern uint8_t is_printf_debug(void);


extern int s_old_level;


#endif
