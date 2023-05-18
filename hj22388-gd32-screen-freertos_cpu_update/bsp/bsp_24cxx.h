/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_24cxx.h
* 摘要：eeprom驱动接口声明
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#ifndef __BSP_24CXX_H
#define __BSP_24CXX_H

#include <stdint.h>

//typedef struct
//{

//} eeprom_info_t;

#define EEPROM_ADDR 0x57

uint8_t eeprom_write_bytes(uint8_t i2c_bus, uint8_t _addr, uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize);
uint8_t eeprom_read_bytes(uint8_t i2c_bus, uint8_t _addr, uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize);

#endif
