/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：bsp_pwm.c
* 摘要：pwm控制
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "bsp_pwm.h"
#include "bsp.h"

#define MAX_PERIOD      6750
#define MAX_LIGHT_LEVEL 2600 // 默认最大亮度等级


typedef struct
{
    uint32_t start_level;
    uint32_t current_level;
    uint32_t l15;
    uint32_t l35;
    uint32_t l50;
    uint32_t l255;
} lcd_light_t;
static lcd_light_t g_lcd_light =
{
    .start_level   = 255,
    .current_level = 100,
    .l15           = 19,
    .l35           = 90,
    .l50           = 215,
    .l255          = MAX_LIGHT_LEVEL,
};

typedef struct
{
    // timer
    uint32_t timer;
    rcu_periph_enum clk;
    uint16_t channel;
    uint16_t prescaler;
    uint32_t period;
    uint32_t value;

    // pwm pin out
    uint8_t is_enable_pwm;
    rcu_periph_enum pwm_clk;
    uint32_t pwm_port;
    uint32_t pwm_af;
    uint32_t pwm_pin;

} user_pwm_cfg_t;

static user_pwm_cfg_t g_user_pwm_cfg =
{
    .pwm_clk  = RCU_GPIOB,
    .pwm_port = GPIOB,
    .pwm_pin  = GPIO_PIN_7,

    .timer = TIMER3,
    .clk = RCU_TIMER3,

    .is_enable_pwm = 1,
    .channel = TIMER_CH_1,
    // timer clock = SystemCoreClock / 1080 = 100kHz
    .prescaler = 1 - 1,

    .period    = MAX_PERIOD - 1,
    .value     = 1 - 1,
};

/*
* 函数介绍：定时器3初始化，输出pwm
* 参数：cfg：定时器3配置参数
* 返回值：无
* 备注：
*/
void pwm_init(user_pwm_cfg_t *cfg)
{
    // pwm pin
    rcu_periph_clock_enable(cfg->pwm_clk);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(cfg->pwm_port, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, cfg->pwm_pin);

    // timer
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(cfg->clk);

    timer_deinit(cfg->timer);

    // cfg->timer configuration
    timer_initpara.prescaler         = cfg->prescaler;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = cfg->period;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(cfg->timer, &timer_initpara);

    if (cfg->is_enable_pwm)
    {
        // CH configuration in PWM mode
        timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
        timer_ocintpara.outputstate = TIMER_CCX_ENABLE;

        timer_channel_output_config(cfg->timer, cfg->channel, &timer_ocintpara);

        // CH configuration in PWM mode
        timer_channel_output_pulse_value_config(cfg->timer, cfg->channel, cfg->value);
        timer_channel_output_mode_config(cfg->timer, cfg->channel, TIMER_OC_MODE_PWM0);
        timer_channel_output_shadow_config(cfg->timer, cfg->channel, TIMER_OC_SHADOW_DISABLE);
    }

    // auto-reload preload enable
    timer_auto_reload_shadow_enable(cfg->timer);
    // auto-reload preload enable
    timer_enable(cfg->timer);
}

/*
* 函数介绍：从eeprom中读取配置参数，初始化亮度校准相关参数
* 参数：无
* 返回值：无
* 备注：
*/
static void bsp_pwm_load_config(void)
{
    uint8_t i, *buf, is_eeprom_init = 1;
    uint8_t ret;
    lcd_light_t config;

    // 从eeprom中读取
    ret = eeprom_read_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&config, 0x00, sizeof(lcd_light_t));
    if (ret)
    {
        buf = (uint8_t *)&config;
        for (i = 0; i < sizeof(lcd_light_t); i++)
        {
            if (buf[i] != 0xff)  //读到数据了
            {
                is_eeprom_init = 0;
                printf("factory eeprom init.\r\n");   //表示读取了数据
                break;
            }
        }
        // 如果成功读取eeprom
        if (!is_eeprom_init)
        {
            // 给全局g_lcd_light赋值，把eeprom中的内容读出
            memcpy((uint8_t *)&g_lcd_light, (uint8_t *)&config, sizeof(lcd_light_t));
        }
        else
		{	//设置亮度到eeprom
            bsp_pwm_set_start_level(g_lcd_light.start_level);
        }
    }
    g_lcd_light.current_level = g_lcd_light.start_level;
	s_old_level = g_lcd_light.start_level;   //2023-04-27 added
}

/*
* 函数介绍：打印屏幕背光参数信息
* 参数：无
* 返回值：无
* 备注：
*/
void bsp_pwm_show_lcd(void)
{
    printf("lcd config:\r\n");
    printf("default level: %d\r\n", (int)g_lcd_light.start_level);
    printf("current level: %d\r\n", (int)g_lcd_light.current_level);
    printf("0.5cd/m*m    : %d\r\n", (int)g_lcd_light.l15);
    printf("2cd/m*m      : %d\r\n", (int)g_lcd_light.l35);
    printf("5cd/m*m      : %d\r\n", (int)g_lcd_light.l50);
    printf("max cd/m*m   : %d\r\n", (int)g_lcd_light.l255);
}

/*
* 函数介绍：将屏幕背光参数默认值写进eeprom
* 参数：无
* 返回值：无
* 备注：
*/
void bsp_pwm_set_default(void)
{
    uint8_t ret;
    lcd_light_t config =
    {
        .start_level   = 100,
        .current_level = 100,
        .l15           = 19,
        .l35           = 90,
        .l50           = 215,
        .l255          = MAX_LIGHT_LEVEL,
    };
    // 写eeprom
    ret = eeprom_write_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&config, 0x00, sizeof(lcd_light_t));
    if (ret)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        memcpy((uint8_t *)&g_lcd_light, (uint8_t *)&config, sizeof(lcd_light_t));
    }

    if (!ret)
    {
        printf("save lcd config failed.\r\n");
    }
}

/*
* 函数介绍：设置亮度级数
* 参数：level：亮度级数
* 返回值：无
* 备注：
*/
void bsp_pwm_set_level(uint8_t level)
{
    uint16_t val = 0;
//    uint8_t ret = 0;
    // 校准亮度，计算pwm占空比
    if (level <= 15)
    {
        val = g_lcd_light.l15 * level / 15;
    }
    else if (level <= 35)
    {
        val = (g_lcd_light.l35 - g_lcd_light.l15) * (level - 15) / 20 + g_lcd_light.l15;
    }
    else if (level <= 50)
    {
        val = (g_lcd_light.l50 - g_lcd_light.l35) * (level - 35) / 15 + g_lcd_light.l35;
    }
    else if (level <= 255)
    {
        val = (g_lcd_light.l255 - g_lcd_light.l50) * (level - 50) / 206 + g_lcd_light.l50;
    }
    else
    {

    }
    // 设置pwm占空比
    bsp_set_duty_cycle(val);

    g_lcd_light.current_level = level;
}

/*
* 函数介绍：将初始化亮度写入eeprom
* 参数：level：亮度级数
* 返回值：无
* 备注：
*/
void bsp_pwm_set_start_level(uint8_t level)
{
    uint8_t ret;
    lcd_light_t config;
    g_lcd_light.start_level = level;
    // 写eeprom
    ret = eeprom_write_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&g_lcd_light, 0x00, sizeof(lcd_light_t));
    if (ret)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        // 读eeprom，判断是否写成功
        ret = eeprom_read_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&config, 0x00, sizeof(lcd_light_t));
        if (ret)
        {
            if (g_lcd_light.start_level != config.start_level)
            {
                ret = 0;
            }
        }
    }

    if (!ret)
    {
        printf("save lcd config failed.\r\n");
    }
}

/*
* 函数介绍：将亮度校准参数写入eeprom
* 参数：pwm：pwm占空比
* 返回值：无
* 备注：
*/
void bsp_pwm_set_config(const int *pwm)
{
    uint8_t ret;
    lcd_light_t config;
    g_lcd_light.l15 = pwm[0];
    g_lcd_light.l35 = pwm[1];
    g_lcd_light.l50 = pwm[2];
    // 写eeprom
    ret = eeprom_write_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&g_lcd_light, 0x00, sizeof(lcd_light_t));
    if (ret)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        // 读eeprom，判断是否写入成功
        ret = eeprom_read_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&config, 0x00, sizeof(lcd_light_t));
        if (ret)
        {
            if ((g_lcd_light.l15 != config.l15) || \
                    (g_lcd_light.l35 != config.l35) || \
                    (g_lcd_light.l50 != config.l50))
            {
                ret = 0;
            }
        }
    }

    if (!ret)
    {
        printf("save lcd config failed.\r\n");
    }
}

/*
* 函数介绍：pwm初始化
* 参数：无
* 返回值：无
* 备注：
*/
void bsp_pwm_init(void)
{
    // pwm配置初始化
    pwm_init(&g_user_pwm_cfg);
    // 读取亮度校准参数信息
    bsp_pwm_load_config();

    // 设置初始化亮度
    bsp_pwm_set_level(g_lcd_light.start_level);
}

// 设置pwm占空比
void bsp_set_duty_cycle(uint16_t val)
{
    uint32_t a = (g_user_pwm_cfg.period + 1) * val / MAX_PERIOD - 1;

    if (val == 0) a = 0;
    timer_channel_output_pulse_value_config(g_user_pwm_cfg.timer, g_user_pwm_cfg.channel, a);
}

/*
* 函数介绍：设置最大亮度等级
* 参数：level：最大亮度等级
* 返回值：无
* 备注：
*/
void bsp_pwm_set_max_level(int level)
{
    uint8_t ret;
    lcd_light_t config;
    g_lcd_light.l255 = level;

    ret = eeprom_write_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&g_lcd_light, 0x00, sizeof(lcd_light_t));
    if (ret)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        ret = eeprom_read_bytes(I2C_BUS_1, EEPROM_ADDR, (uint8_t *)&config, 0x00, sizeof(lcd_light_t));
        if (ret)
        {
            if (g_lcd_light.l255 != config.l255)
            {
                ret = 0;
            }
        }
    }

    if (!ret)
    {
        printf("save lcd config failed.\r\n");
    }
}


void StartlcdPwmTask(void *arg)
{
	vTaskDelay(pdMS_TO_TICKS(1800));
	bsp_pwm_init();    //延时2秒初始化屏幕pwm
	vTaskDelete(NULL);   //自己删除自己
//	while(1)
//	{
//		vTaskDelay(pdMS_TO_TICKS(2000));
//	}
	
}



