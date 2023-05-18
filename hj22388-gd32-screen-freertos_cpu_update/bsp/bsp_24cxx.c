/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_24cxx.c
* 摘要：eeprom驱动
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "bsp_24cxx.h"
#include "bsp_i2c_gpio.h"
#include "bsp.h"

#define EEPROM_PAGE_SIZE     128

/*
* 函数介绍：从eeprom中读多个数据
* 参数：i2c_bus：i2c0=0；i2c1=1
*       _addr：eeprom地址
*       _pReadBuf：传出参数，携带读取的数据
*       _usAddress：数据存储地址
*       _usSize：读取数据长度
* 返回值：0表示失败；1表示成功
* 备注：
*/
uint8_t eeprom_read_bytes(uint8_t i2c_bus, uint8_t _addr, uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize)
{
    uint16_t i;
    uint8_t addr = _addr << 1;

    // 第1步：发起I2C总线启动信号
    i2c_Start(i2c_bus);

    // 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读
    addr = (_addr << 1) | I2C_WR;
    // 此处是写指令
    i2c_SendByte(i2c_bus, addr);

    // 第3步：发送ACK
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    // 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址
    i2c_SendByte(i2c_bus, (uint8_t)(_usAddress >> 8));
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    i2c_SendByte(i2c_bus, (uint8_t)(_usAddress & 0x0ff));
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    // 第6步：重新启动I2C总线。前面的代码的目的向EEPROM传送地址，下面开始读取数据
    i2c_Start(i2c_bus);

    // 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读
    addr = (_addr << 1) | I2C_RD;
    // 此处是读指令
    i2c_SendByte(i2c_bus, addr);

    // 第8步：发送ACK
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    // 第9步：循环读取数据
    for (i = 0; i < _usSize; i++)
    {
        // 读1个字节
        _pReadBuf[i] = i2c_ReadByte(i2c_bus);

        // 每读完1个字节后，需要发送Ack， 最后一个字节不需要Ack，发Nack
        if (i != _usSize - 1)
        {
            // 中间字节读完后，CPU产生ACK信号(驱动SDA = 0)
            i2c_Ack(i2c_bus);
        }
        else
        {
            // 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1)
            i2c_NAck(i2c_bus);
        }
    }

    // 发送I2C总线停止信号
    i2c_Stop(i2c_bus);
    // 执行成功
    return 1;
}

/*
* 函数介绍：向eeprom中写入多个数据
* 参数：i2c_bus：i2c0=0；i2c1=1
*       _addr：eeprom地址
*       _pWriteBuf：需写入的数据
*       _usAddress：数据存储地址
*       _usSize：读取数据长度
* 返回值：0表示失败；1表示成功
* 备注：
*/
uint8_t eeprom_write_bytes(uint8_t i2c_bus, uint8_t _addr, uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize)
{
    uint16_t i;
    uint8_t addr = _addr << 1;
    int start_pos = _usAddress % EEPROM_PAGE_SIZE;
    int wr_size = 0;
    int pre_wr = 0;
    // 第1步：发起I2C总线启动信号
    i2c_Start(i2c_bus);
    // 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读
    addr = (_addr << 1) | I2C_WR;
    i2c_SendByte(i2c_bus, addr);
    // 发送ACK
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    i2c_SendByte(i2c_bus, (uint8_t)(_usAddress >> 8));
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    i2c_SendByte(i2c_bus, (uint8_t)(_usAddress & 0x0ff));
    if (i2c_WaitAck(i2c_bus) != 0)
    {
        // 发送I2C总线停止信号
        i2c_Stop(i2c_bus);
        return 0;
    }

    // 判读是否需要跨页写
    if (start_pos != 0)
    {
        // 判断页剩余字节数是否大于需写入的字节
        if ((EEPROM_PAGE_SIZE - start_pos) >= _usSize)
        {
            wr_size = _usSize;
        }
        else
        {
            wr_size = EEPROM_PAGE_SIZE - start_pos;
        }
        // 循环写入
        for (i = 0; i < wr_size; i++)
        {
            i2c_SendByte(i2c_bus, _pWriteBuf[i]);
            if (i2c_WaitAck(i2c_bus) != 0)
            {
                // 发送I2C总线停止信号
                i2c_Stop(i2c_bus);
                return 0;
            }
        }
    }
    // 循环写入数据
    while (wr_size < _usSize)
    {
        // 判断是否需要翻页
        if ((_usSize - wr_size) >= EEPROM_PAGE_SIZE)
        {
            pre_wr = 128;
        }
        else
        {
            pre_wr = _usSize - wr_size;
        }

        for (i = 0; i < pre_wr; i++)
        {
            i2c_SendByte(i2c_bus, _pWriteBuf[wr_size + i]);
            if (i2c_WaitAck(i2c_bus) != 0)
            {
                // 发送I2C总线停止信号
                i2c_Stop(i2c_bus);
                return 0;
            }
        }
        wr_size += pre_wr;
    }

    // 命令执行成功，发送I2C总线停止信号
    i2c_Stop(i2c_bus);
    return 1;
}


