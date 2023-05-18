/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_comm_uart.c
* 摘要：与上位机通信串口任务
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "common.h"
#include "bsp_comm_uart.h"
#include "bsp.h"

#define SERIAL_CMD_SIZE      32

typedef struct
{
    uint8_t cmd[SERIAL_CMD_SIZE];  // 控制指令
    ringbuffer_t rd_buf;    // 循环buf
    SemaphoreHandle_t sem;  // 二值信号量
} user_cli_info_t;

// 串口配置
static user_uart_cfg_t s_comm_uart =
{
    .com         = USART0,
    .clk         = RCU_USART0,
    .irq         = USART0_IRQn,
    .tx_port_clk = RCU_GPIOA,
    .tx_port     = GPIOA,
    .tx_pin      = GPIO_PIN_9,
    .rx_port_clk = RCU_GPIOA,
    .rx_port     = GPIOA,
    .rx_pin      = GPIO_PIN_10,
};

static user_cli_info_t s_user_cli_info;

/*
* 函数介绍：串口0初始化
* 参数：无
* 返回值：uint8_t：返回0
* 备注：初始化串口配置，初始化循环buf和信号量
*/
uint8_t comm_uart_init()
{
    ringbuffer_init(&s_user_cli_info.rd_buf);
    s_user_cli_info.sem = xSemaphoreCreateBinary();

    return uart_init(&s_comm_uart,0x60);
}

static uint8_t s_ch;
static BaseType_t s_xHigherPriorityTaskWoken;
/*
* 函数介绍：串口0中断服务函数
* 参数：无
* 返回值：无
* 备注：
*/
void USART0_IRQHandler(void)
{
    s_xHigherPriorityTaskWoken = pdFALSE;

    if (RESET != usart_flag_get(s_comm_uart.com, USART_FLAG_RBNE))
    {
        /* receive data */
        s_ch = (usart_data_receive(s_comm_uart.com) & 0xFF);
        // 将接收到的数据存进buf
        ringbuffer_push_back_irq(&s_user_cli_info.rd_buf, &s_ch, 1);
        // 放置一个信号量到队列中
        xSemaphoreGiveFromISR(s_user_cli_info.sem, &s_xHigherPriorityTaskWoken);
    }

    if((RESET != usart_flag_get(s_comm_uart.com, USART_FLAG_TBE)) &&
            (RESET != usart_interrupt_flag_get(s_comm_uart.com, USART_INT_FLAG_TBE)))
    {
    }
}

/*
* 函数介绍：亮度设置
* 参数：level：亮度等级
* 返回值：无
* 备注：
*/
static void sf_level_set(int level)
{
    int pwm = 0;
    // 计算pwm占空比
    if (level < 50)
    {
        pwm = level * 5;
    }
    else if (level < 256)
    {
        pwm = 250 + (40000 - 250) * (level - 50) / 206;
    }
    else
    {

    }
    bsp_set_duty_cycle(pwm);
}

// 控制指令
#define CMD_LIGHT_CTRL        0x80  // 亮度控制指令
#define CMD_SCREEN_OFF        0x84  // 息屏指令
#define CMD_SCREEN_WAKE       0x86  // 屏幕唤醒指令
#define CMD_QUERY_DEVICE_TYPE 0x88  // 查询设备类型
#define CMD_QUERY_VERSION     0x8a  // 查询版本号

//#define DEVICE_TYPE           0x00         // 0x00：5寸屏；0x01：7寸屏
//#define DEVICE_TYPE           0x05         // 0x05：5寸屏；0x04：7寸屏,0x06: new5寸屏  2022-12-09
#define VERSION               0x03         // 版本号: a.b.c; a:[bit7-bit6], b:[bit5-bit4], c:[bit3-bit0]
										//2022-12-09 VERSION-->2，//2022-12-15 VERSION-->3

static int s_old_level = 0;
/*
* 函数介绍：串口指令处理函数
* 参数：ptr：串口数据帧
* 返回值：无
* 备注：
*/
static void sf_receive_handler(void *ptr)
{
    serial_frame_t *frame = (serial_frame_t *)ptr;
    uint8_t tx_buf[12] = {0};
    uint16_t tx_len = 0;
    // 发送帧帧头
    tx_buf[0] = 0xa5;
    tx_buf[1] = 0x5a;

    switch (frame->data[0])
    {
    case CMD_LIGHT_CTRL:  // 亮度控制指令处理
        sf_level_set(frame->data[1]);
        bsp_pwm_set_level(frame->data[1]);
        s_old_level = frame->data[1];

        tx_buf[2] = CMD_LIGHT_CTRL + 1;
        tx_buf[3] = frame->data[1];
        tx_len = sf_packet_data(tx_buf, 12, 8);
        break;
    case CMD_SCREEN_OFF:  // 息屏处理
        sf_level_set(0);

        tx_buf[2] = CMD_SCREEN_OFF + 1;
        tx_len = sf_packet_data(tx_buf, 12, 8);
        break;
    case CMD_SCREEN_WAKE:  // 屏幕唤醒处理
        sf_level_set(s_old_level);

        tx_buf[2] = CMD_SCREEN_WAKE + 1;
        tx_len = sf_packet_data(tx_buf, 12, 8);
        break;
    case CMD_QUERY_DEVICE_TYPE:  // 设备类型查询处理
        tx_buf[2] = CMD_QUERY_DEVICE_TYPE + 1;
        tx_buf[3] = DEVICE_TYPE;
        tx_len = sf_packet_data(tx_buf, 12, 8);
        break;
    case CMD_QUERY_VERSION:  // 版本号查询处理
        tx_buf[2] = CMD_QUERY_VERSION + 1;
        tx_buf[3] = VERSION;
        tx_len = sf_packet_data(tx_buf, 12, 8);
        break;
    default:
        break;
    }
    // 回复帧
    for (int i = 0; i < tx_len; i++)
    {
        usart_data_transmit(s_comm_uart.com, (uint32_t)tx_buf[i]);
        while (RESET == usart_flag_get(s_comm_uart.com, USART_FLAG_TBE));
    }
}

/*
* 函数介绍：串口通信任务
* 参数：无
* 返回值：无
* 备注：
*/
void StartCommTask(void *argument)
{
    uint8_t ch = 0;
    // 注册指令处理函数回调
    sf_register_cb(sf_receive_handler);
    // 串口初始化
    comm_uart_init();

    while (1)
    {
        // 获取信号量
        if (xSemaphoreTake(s_user_cli_info.sem, portMAX_DELAY) == pdTRUE)
        {
            // 判断接收buf是否为空
            if (!ringbuffer_is_empty(&s_user_cli_info.rd_buf))
            {
                // 获取一个字节数据
                while (ringbuffer_pop_front(&s_user_cli_info.rd_buf, &ch, 1))
                {
                    // 数据解析
                    sf_get_char(ch);
                }
            }
        //    xSemaphoreGive(s_user_cli_info.sem);
        }
    }
}

