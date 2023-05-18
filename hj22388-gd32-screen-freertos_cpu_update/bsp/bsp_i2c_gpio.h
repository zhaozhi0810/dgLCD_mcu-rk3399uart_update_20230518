/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_i2c_gpio.h
* 摘要：gpio模拟i2c接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef _BSP_I2C_GPIO_H
#define _BSP_I2C_GPIO_H

#include <inttypes.h>

#define I2C_BUS_0   0
#define I2C_BUS_1   1

#define I2C_WR	0		/* 写控制bit */
#define I2C_RD	1		/* 读控制bit */

void i2c_Start(uint8_t id);
void i2c_Stop(uint8_t id);
void i2c_SendByte(uint8_t id, uint8_t _ucByte);
uint8_t i2c_ReadByte(uint8_t id);
uint8_t i2c_WaitAck(uint8_t id);
void i2c_Ack(uint8_t id);
void i2c_NAck(uint8_t id);
uint8_t i2c_CheckDevice(uint8_t id, uint8_t _Address);
uint8_t i2c_WaitAck_us(uint8_t id, uint8_t cnt);

void bsp_i2c_gpio_init(void);
void bsp_i2c_detect(uint8_t id);

#endif
