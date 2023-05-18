/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_pwm.h
* 摘要：pwm控制接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/

#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "common.h"

void bsp_pwm_init(void);
void bsp_set_duty_cycle(uint16_t val);
void bsp_pwm_show_lcd(void);
void bsp_pwm_set_default(void);
void bsp_pwm_set_level(uint8_t level);
void bsp_pwm_set_start_level(uint8_t level);
void bsp_pwm_set_config(const int *pwm);
void bsp_pwm_set_max_level(int level);
void StartlcdPwmTask(void *arg);

#endif
