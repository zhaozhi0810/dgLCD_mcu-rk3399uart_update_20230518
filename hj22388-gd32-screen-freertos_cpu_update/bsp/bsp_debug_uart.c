/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_debug_uart.c
* 摘要：调试串口任务，模拟shell
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "common.h"
#include "bsp_debug_uart.h"
#include "bsp.h"

typedef struct
{
    ringbuffer_t rd_buf;
    SemaphoreHandle_t sem;
} local_uart_info_t;

// 定义并初始化
static user_uart_cfg_t s_debug_uart =
{
    .com         = USART1,
    .clk         = RCU_USART1,
    .irq         = USART1_IRQn,
    .tx_port_clk = RCU_GPIOA,
    .tx_port     = GPIOA,
    .tx_pin      = GPIO_PIN_2,
    .rx_port_clk = RCU_GPIOA,
    .rx_port     = GPIOA,
    .rx_pin      = GPIO_PIN_3,
};

static local_uart_info_t s_user_debug_info;

/*
* 函数介绍：串口1初始化
* 参数：cfg：串口配置
* 返回值：uint8_t：返回0
* 备注：
*/
uint8_t uart_init(user_uart_cfg_t *cfg,uint8_t irq_priority)
{
    rcu_periph_clock_enable(cfg->tx_port_clk);
    rcu_periph_clock_enable(cfg->rx_port_clk);

    /* enable USART clock */
    rcu_periph_clock_enable(cfg->clk);

    gpio_init(cfg->tx_port, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, cfg->tx_pin);
    gpio_init(cfg->rx_port, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, cfg->rx_pin);

    /* USART configure */
    usart_deinit(cfg->com);
    usart_baudrate_set(cfg->com, 115200U);
    usart_word_length_set(cfg->com, USART_WL_8BIT);
    usart_stop_bit_set(cfg->com, USART_STB_1BIT);
    usart_parity_config(cfg->com, USART_PM_NONE);
    usart_hardware_flow_rts_config(cfg->com, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(cfg->com, USART_CTS_DISABLE);
    usart_receive_config(cfg->com, USART_RECEIVE_ENABLE);
    usart_transmit_config(cfg->com, USART_TRANSMIT_ENABLE);
    usart_enable(cfg->com);
    // 使能串口
    nvic_irq_enable(cfg->irq, irq_priority>>4, irq_priority&0xf);
    usart_interrupt_enable(cfg->com, USART_INT_RBNE);

    return 0;
}

/*
* 函数介绍：串口初始化，包括循环buf、信号量和硬件
* 参数：无
* 返回值：uint8_t：返回0
* 备注：
*/
uint8_t debug_uart_init()
{
    ringbuffer_init(&s_user_debug_info.rd_buf);
    s_user_debug_info.sem = xSemaphoreCreateBinary();

    return uart_init(&s_debug_uart,0xa0);
}

int fputc(int ch, FILE *f)
{
    usart_data_transmit(s_debug_uart.com, (uint32_t)ch);
    while (RESET == usart_flag_get(s_debug_uart.com, USART_FLAG_TBE));
    return ch;
}

int fgetc(FILE *f)
{
    return 0;
}

static uint8_t s_ch;
static BaseType_t s_xHigherPriorityTaskWoken;

/*
* 函数介绍：串口1中断服务函数
* 参数：无
* 返回值：无
* 备注：
*/
void USART1_IRQHandler(void)
{
    s_xHigherPriorityTaskWoken = pdFALSE;

    if (RESET != usart_flag_get(s_debug_uart.com, USART_FLAG_RBNE))
    {
        /* receive data */
        s_ch = (usart_data_receive(s_debug_uart.com) & 0xFF);
        // 将接收到的数据存进buf
        ringbuffer_push_back_irq(&s_user_debug_info.rd_buf, &s_ch, 1);
        // 放置一个信号量到队列中
        xSemaphoreGiveFromISR(s_user_debug_info.sem, &s_xHigherPriorityTaskWoken);
    }

    if((RESET != usart_flag_get(s_debug_uart.com, USART_FLAG_TBE)) &&
            (RESET != usart_interrupt_flag_get(s_debug_uart.com, USART_INT_FLAG_TBE)))
    {
    }
}

/*
* 函数介绍：调试串口任务
* 参数：无
* 返回值：无
* 备注：
*/
void StartDebugTask(void *argument)
{
    uint8_t ch = 0;

    shell_init();
    debug_uart_init();

	//    // pwm初始化
    //bsp_pwm_init();
	
    while (1)
    {
        // 获取信号量
        if (xSemaphoreTake(s_user_debug_info.sem, portMAX_DELAY) == pdTRUE)
        {
            // 如果buf不为空
            if (!ringbuffer_is_empty(&s_user_debug_info.rd_buf))
            {
                while (ringbuffer_pop_front(&s_user_debug_info.rd_buf, &ch, 1))
                {
                    shell(ch);
                }
            }
        //    xSemaphoreGive(s_user_debug_info.sem);
        }
    }

}

