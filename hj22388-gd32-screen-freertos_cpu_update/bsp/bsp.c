/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp.c
* 摘要：外设初始化
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "bsp.h"

//工作指示led灯
#define LED_PORT GPIOA
#define LED_CLK  RCU_GPIOA
#define LED_PIN  GPIO_PIN_7


//触摸屏复位控制引脚
#define TS_PORT GPIOB
#define TS_CLK  RCU_GPIOB
#define TS_PIN  GPIO_PIN_12

/*
* 函数介绍：板卡GPIO初始化
* 参数：无
* 返回值：无
* 备注：
*/
static uint8_t bsp_gpio_init(void)
{
	//lcd复位引脚拉低，不显示
	rcu_periph_clock_enable(RCU_GPIOB);
    gpio_bit_reset(GPIOB, GPIO_PIN_7);
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_7);
//	
    // LED
    rcu_periph_clock_enable(LED_CLK);
    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, LED_PIN);
    gpio_bit_set(LED_PORT, LED_PIN);

    // TS
    rcu_periph_clock_enable(TS_CLK);
    gpio_init(TS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, TS_PIN);
    gpio_bit_set(TS_PORT, TS_PIN);
	
    return 0;
}

// LED翻转
void bsp_led_toggle(void)
{
    if (gpio_output_bit_get(LED_PORT, LED_PIN))
    {
        gpio_bit_reset(LED_PORT, LED_PIN);
    }
    else
    {
        gpio_bit_set(LED_PORT, LED_PIN);
    }
}

// ts reset
void bsp_ts_reset(void)
{
    gpio_bit_set(TS_PORT, TS_PIN);
	vTaskDelay(pdMS_TO_TICKS(30));
    //delay_ms(30); // 延时30ms
    gpio_bit_reset(TS_PORT, TS_PIN);
	vTaskDelay(pdMS_TO_TICKS(30));
    //delay_ms(30);
    gpio_bit_set(TS_PORT, TS_PIN);
}

// for system startup timer
//static uint32_t g_bsp_startup_tick = 0;
//void bsp_tick_increace(void)
//{
//    g_bsp_startup_tick++;
//}

//uint32_t bsp_get_tick(void)
//{
//    return g_bsp_startup_tick;
//}

// 定时器初始化
//static void bsp_tim_init(void)
//{
//    /* TIMER1 configuration: generate PWM signals with different duty cycles:
//       TIMER1CLK = SystemCoreClock / 200 = 1MHz */
//    timer_parameter_struct timer_initpara;

//    rcu_periph_clock_enable(RCU_TIMER1);

//    timer_deinit(TIMER1);

//    /* TIMER1 configuration */
//    timer_initpara.prescaler         = 108 - 1;
//    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
//    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
//    timer_initpara.period            = 1000 - 1;
//    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
//    timer_initpara.repetitioncounter = 0;
//    timer_init(TIMER1, &timer_initpara);

//    /* auto-reload preload enable */
//    timer_auto_reload_shadow_enable(TIMER1);

//    /* TIMER1 enable */
//    timer_enable(TIMER1);

//    nvic_irq_enable(TIMER1_IRQn, 0, 1);

//    timer_interrupt_enable(TIMER1, TIMER_INT_UP);
//}

//void TIMER1_IRQHandler(void)
//{
//    if (RESET != timer_interrupt_flag_get(TIMER1, TIMER_INT_UP))
//    {
//        timer_interrupt_flag_clear(TIMER1, TIMER_INT_UP);

//        bsp_tick_increace();
//    }
//}

/*
* 函数介绍：外设初始化
* 参数：无
* 返回值：无
* 备注：
*/
uint8_t bsp_init(void)
{
    // gpio初始化
    bsp_gpio_init();
    // 定时器初始化
//    bsp_tim_init();
//    // gpio模拟i2c初始化
    bsp_i2c_gpio_init();

//    // ts初始化
//    bsp_ts_reset();

    return 0;
}


// Common Function
// gd32f013 108M : 27000 counter
//void delay_ms(int32_t ms)
//{
//    ms *= 27000;
//    while (ms-- > 0) ;
//}
