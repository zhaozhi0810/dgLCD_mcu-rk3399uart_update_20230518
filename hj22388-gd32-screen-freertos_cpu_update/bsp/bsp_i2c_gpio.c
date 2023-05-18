/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_i2c_gpio.c
* 摘要：gpio模拟i2c
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "common.h"
#include "bsp_i2c_gpio.h"


typedef struct
{
    uint32_t i2c;
    rcu_periph_enum clk;
    uint32_t speed;
    uint8_t addr;

    // scl
    rcu_periph_enum scl_clk;
    uint32_t scl_port;
    uint32_t scl_af;
    uint32_t scl_pin;

    // sda
    rcu_periph_enum sda_clk;
    uint32_t sda_port;
    uint32_t sda_af;
    uint32_t sda_pin;
} user_i2c_cfg_t;

// 定义并初始化i2c0的配置信息
static user_i2c_cfg_t g_i2c0_cfg =
{
    .scl_clk  = RCU_GPIOB,
    .scl_port = GPIOB,
    .scl_pin  = GPIO_PIN_8,
    .sda_clk  = RCU_GPIOB,
    .sda_port = GPIOB,
    .sda_pin  = GPIO_PIN_9,
};

// 定义并初始化i2c1的配置信息
static user_i2c_cfg_t g_i2c1_cfg =
{
    .scl_clk  = RCU_GPIOB,
    .scl_port = GPIOB,
    .scl_pin  = GPIO_PIN_10,
    .sda_clk  = RCU_GPIOB,
    .sda_port = GPIOB,
    .sda_pin  = GPIO_PIN_11,
};

static user_i2c_cfg_t *bsp_i2c_get_bus(uint8_t bus)
{
    if (bus == 0)
    {
        return &g_i2c0_cfg;
    }
    else if (bus == 1)
    {
        return &g_i2c1_cfg;
    }
    else
    {

    }
    return 0;
}

/* 定义读写SCL和SDA的宏，已增加代码的可移植性和可阅读性 */
#if 0	/* 条件编译： 1 选择GPIO的库函数实现IO读写 */
#define I2C_SCL_1(cfg->scl_port, cfg->scl_pin)  HAL_GPIO_WritePin(GPIO_PORT_I2C, I2C_SCL_PIN, GPIO_PIN_SET)		/* SCL = 1 */
#define I2C_SCL_0(cfg->scl_port, cfg->scl_pin)  HAL_GPIO_WritePin(GPIO_PORT_I2C, I2C_SCL_PIN, GPIO_PIN_RESET)		/* SCL = 0 */

#define I2C_SDA_1(cfg->sda_port, cfg->sda_pin)  HAL_GPIO_WritePin(GPIO_PORT_I2C, I2C_SDA_PIN, GPIO_PIN_SET)		/* SDA = 1 */
#define I2C_SDA_0(cfg->sda_port, cfg->sda_pin)  HAL_GPIO_WritePin(GPIO_PORT_I2C, I2C_SDA_PIN, GPIO_PIN_RESET)		/* SDA = 0 */

#define I2C_SDA_READ(cfg->sda_port, cfg->sda_pin)  HAL_GPIO_ReadPin(GPIO_PORT_I2C, I2C_SDA_PIN)	/* 读SDA口线状态 */
#else	/* 这个分支选择直接寄存器操作实现IO读写 */
/*　注意：如下写法，在IAR最高级别优化时，会被编译器错误优化 */
#define I2C_SCL_1(gpio, pin)  GPIO_BOP(gpio) = pin				/* SCL = 1 */
#define I2C_SCL_0(gpio, pin)  GPIO_BC(gpio) = pin				/* SCL = 0 */

#define I2C_SDA_1(gpio, pin)  GPIO_BOP(gpio) = pin				/* SDA = 1 */
#define I2C_SDA_0(gpio, pin)  GPIO_BC(gpio) = pin				/* SDA = 0 */

#define I2C_SDA_READ(gpio, pin)  gpio_input_bit_get(gpio, pin)	/* 读SDA口线状态 */
#endif


/*
* 函数介绍：I2C总线位延迟，最快400KHz
* 参数：无
* 返回值：无
* 备注：
*/
void i2c_Delay(void)
{
    uint8_t i;

    /*　
     	下面的时间是通过安富莱AX-Pro逻辑分析仪测试得到的。
    	CPU主频72MHz时，在内部Flash运行, MDK工程不优化
    	循环次数为10时，SCL频率 = 205KHz
    	循环次数为7时，SCL频率 = 347KHz， SCL高电平时间1.5us，SCL低电平时间2.87us
     	循环次数为5时，SCL频率 = 421KHz， SCL高电平时间1.25us，SCL低电平时间2.375us

        IAR工程编译效率高，不能设置为7
    */
    for (i = 0; i < 120; i++);
}

/*
* 函数介绍：CPU发起I2C总线启动信号
* 参数：id:0或1
* 返回值：无
* 备注：
*/
void i2c_Start(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);

    /* 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号 */
    I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
    I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();  // 延时
    i2c_Delay();
    I2C_SDA_0(cfg->sda_port, cfg->sda_pin);
    i2c_Delay();
    i2c_Delay();
    I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
}


/*
* 函数介绍：CPU发起I2C总线停止信号
* 参数：id:0或1
* 返回值：无
* 备注：
*/
void i2c_Stop(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);

    /* 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号 */
    I2C_SDA_0(cfg->sda_port, cfg->sda_pin);
    I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    i2c_Delay();
    i2c_Delay();
    I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
}


/*
* 函数介绍：CPU向I2C总线设备发送8bit数据
* 参数：ucByte：等待发送的字节
* 返回值：无
* 备注：
*/
void i2c_SendByte(uint8_t id, uint8_t _ucByte)
{
    uint8_t i;

    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);

    // 先发送字节的高位bit7
    for (i = 0; i < 8; i++)
    {
        if (_ucByte & 0x80)
        {
            I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
        }
        else
        {
            I2C_SDA_0(cfg->sda_port, cfg->sda_pin);
        }
        i2c_Delay();
        I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
        i2c_Delay();
        I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
        if (i == 7)
        {
            I2C_SDA_1(cfg->sda_port, cfg->sda_pin); // 释放总线
        }
        _ucByte <<= 1;	// 左移一个bit
        i2c_Delay();
    }
}


/*
* 函数介绍：CPU从I2C总线设备读取8bit数据
* 参数：id：0或1
* 返回值：读到的数据
* 备注：
*/
uint8_t i2c_ReadByte(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);

    uint8_t i;
    uint8_t value;

    // 读到第1个bit为数据的bit7
    value = 0;
    for (i = 0; i < 8; i++)
    {
        i2c_Delay();
        value <<= 1;
        I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
        i2c_Delay();
        i2c_Delay();
        if (I2C_SDA_READ(cfg->sda_port, cfg->sda_pin))
        {
            value++;
        }
        I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
        i2c_Delay();
    }
    return value;
}


/*
* 函数介绍：CPU产生一个时钟，并读取器件的ACK应答信号
* 参数：id：0或1
* 返回值：返回0表示正确应答，1表示无器件响应
* 备注：
*/
uint8_t i2c_WaitAck(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);
    uint8_t re;
    // CPU释放SDA总线
    I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
    i2c_Delay();
    // CPU驱动SCL = 1, 此时器件会返回ACK应答
    I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    // CPU读取SDA口线状态
    if (I2C_SDA_READ(cfg->sda_port, cfg->sda_pin))
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    return re;
}

uint8_t i2c_WaitAck_us(uint8_t id, uint8_t cnt)
{
    uint8_t re;

    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);
    // CPU释放SDA总线
    I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
    while (cnt-- > 0)
    {
        i2c_Delay();
    }
    // CPU驱动SCL = 1, 此时器件会返回ACK应答
    I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
    while (cnt-- > 0)
    {
        i2c_Delay();
    }
    // CPU读取SDA口线状态
    if (I2C_SDA_READ(cfg->sda_port, cfg->sda_pin))
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    return re;
}


/*
* 函数介绍：CPU产生一个ACK信号
* 参数：id：0或1
* 返回值：无
* 备注：
*/
void i2c_Ack(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);
    // CPU驱动SDA = 0
    I2C_SDA_0(cfg->sda_port, cfg->sda_pin);
    i2c_Delay();
    // CPU产生1个时钟
    I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    // CPU释放SDA总线
    I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
}


/*
* 函数介绍：CPU产生1个NACK信号
* 参数：id：0或1
* 返回值：无
* 备注：
*/
void i2c_NAck(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);
    // CPU驱动SDA = 1
    I2C_SDA_1(cfg->sda_port, cfg->sda_pin);
    i2c_Delay();
    // CPU产生1个时钟
    I2C_SCL_1(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
    I2C_SCL_0(cfg->scl_port, cfg->scl_pin);
    i2c_Delay();
}


/*
* 函数介绍：配置I2C总线的GPIO，采用模拟IO的方式实现
* 参数：id：0或1
* 返回值：无
* 备注：
*/
static void i2c_CfgGpio(uint8_t id)
{
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);

    rcu_periph_clock_enable(cfg->scl_clk);
    rcu_periph_clock_enable(cfg->sda_clk);

    /* connect PB6 to I2C1_SCL */
    // gpio_af_set(cfg->scl_port, cfg->scl_af, cfg->scl_pin);
    /* connect PB7 to I2C1_SDA */
    // gpio_af_set(cfg->sda_port, cfg->sda_af, cfg->sda_pin);

    gpio_init(cfg->scl_port, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, cfg->scl_pin);
    gpio_init(cfg->sda_port, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, cfg->sda_pin);

    // 给一个停止信号, 复位I2C总线上的所有设备到待机模式
    i2c_Stop(id);
}

/*
* 函数介绍：检测I2C总线设备，CPU向发送设备地址，然后读取设备应答来判断该设备是否存在
* 参数：id：0或1
* 返回值：返回值 0 表示正确， 返回1表示未探测到
* 备注：
*/
void bsp_i2c_detect(uint8_t id)
{
    uint8_t addr;
    user_i2c_cfg_t *cfg = bsp_i2c_get_bus(id);
    printf("\r\n");
    for (uint8_t pos = 0; pos < 0x80; pos++)
    {
        //for (uint8_t pos = 0x5a; pos < 0x80; pos++) {
        if ((pos % 8) == 0)
        {
            printf("%02X: ", pos);
        }

        // 第1步：发起I2C总线启动信号
        i2c_Start(id);
        // 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读
        addr = (pos << 1) | I2C_WR;
        i2c_SendByte(id, addr);
        // 第3步：发送ACK
        if (i2c_WaitAck_us(id, 5) != 0)
        {
            printf("-- ");
        }
        else
        {
            printf("%02x ", pos);
        }
        i2c_Stop(id);
        for (int i = 0; i < 5; i++) i2c_Delay();

        if ((pos % 8) == 7)
        {
            printf("\r\n");
        }
    }
}

// gpio初始化
void bsp_i2c_gpio_init(void)
{
    i2c_CfgGpio(0);
    i2c_CfgGpio(1);
}



