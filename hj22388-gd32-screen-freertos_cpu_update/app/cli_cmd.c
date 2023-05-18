/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：cli_cmd.c
* 摘要：系统初始化
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "bsp.h"

/*
* 函数介绍：指令帮助函数
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：打印各个命令使用方法
*/
static void cli_help_func(char argc, char *argv)
{
    printf("\r\n");
    printf("CLI Command:\r\n");
    printf("version                : show firmware infomation.\r\n");
    printf("show-config            : show correct information of lcd pannel.\r\n");
    printf("set-default            : set default configuration.\r\n");
    printf("set-lcd pwm1 pwm2 pwm3 : config the specific level with pwm duty cycle.\r\n");
    printf("                       : pwm(1-3) for 0.5cd/m*m 2cd/m*m 5cd/m*m\r\n");
    printf("                       : pwm(0-65535) duty cycle\r\n");
    printf("set-pwm pwm            : used to correct pannel for fatory test.\r\n");
    printf("                       : pwm(0-65535) duty cycle\r\n");
    printf("set-level level        : set lcd level.\r\n");
    printf("                       : level(0-255)\r\n");
    printf("set-start-level level  : set start lcd level.\r\n");
    printf("                       : level(0-255)\r\n");
    printf("\r\n");
}

/*
* 函数介绍：打印版本信息
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：
*/
static void cli_version_func(char argc, char *argv)
{
    printf("\r\n");
    printf("%-18s: %s %s\r\n", "Complie Time", COMPILE_DATE, COMPILE_TIME);
    printf("%-18s: %s\r\n", "Firmware Version", FIRMWARE_VERSION);
    printf("\r\n");
}

/*
* 函数介绍：设置PWM占空比，调节亮度
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：
*/
static void cli_set_level_func(char argc, char *argv)
{
    if (argc >= 2)
    {
        int val = atoi((const char *)&argv[argv[1]]);
        if (val > 0 && val < 256)
        {
            bsp_pwm_set_level(val);
            printf("set lcd level to %d.\r\n", val);
        }
				else
				{
            printf("set level failed, value beyond range.\r\n");
				}
    }
}

/*
* 函数介绍：设置最大亮度等级
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：
*/
static void cli_set_max_level_func(char argc, char *argv)
{
    if (argc >= 2)
    {
        int val = atoi((const char *)&argv[argv[1]]);

        if (val > 0 && val < 10000)
        {
            bsp_pwm_set_max_level(val);
            printf("set lcd max level to %d.\r\n", val);
        }
				else
				{
            printf("set max level failed, value beyond range.\r\n");
				}
    }
}

/*
* 函数介绍：设置开机时初始化亮度
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：
*/
static void cli_set_start_level_func(char argc, char *argv)
{
    if (argc >= 2)
    {
        int val = atoi((const char *)&argv[argv[1]]);
        if (val > 0 && val < 256)
        {
            bsp_pwm_set_start_level(val);
            printf("set lcd start level to %d.\r\n", val);
        }
		else
		{
            printf("set lcd start level failed, value beyond range.\r\n");
		}
    }
}

/*
* 函数介绍：设置PWM输出占空比
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：
*/
static void cli_set_pwm_func(char argc, char *argv)
{
    if (argc >= 2)
    {
        int val = atoi((const char *)&argv[argv[1]]);
        if (val > 0 && val < 65536)
        {
            bsp_set_duty_cycle(val);
            printf("set lcd pwm to %d.\r\n", val);
        }
		else
		{
            printf("set lcd pwm failed, value beyond range.\r\n");
		}
    }
}

/*
* 函数介绍：设置几个亮度结点，用于校准亮度
* 参数：argc：参数个数
*       argv：参数
* 返回值：无
* 备注：
*/
static void cli_set_lcd_func(char argc, char *argv)
{
    if (argc >= 4)
    {
        int pwm[3];
        for (int i = 0; i < 3; i++)
        {
            pwm[i] = atoi((const char *)&argv[argv[1 + i]]);
        }
        if (pwm[0] < pwm[1] && pwm[1] < pwm[2] && pwm[2] < 65536)
        {
            bsp_pwm_set_config(pwm);
        }
    }
}

static void cli_show_config_func(char argc, char *argv)
{
    bsp_pwm_show_lcd();
}

static void cli_set_defalut_func(char argc, char *argv)
{
    bsp_pwm_set_default();
}

static void cli_ts_reset_func(char argc, char *argv)
{
    bsp_ts_reset();
}

static void cli_uptime_func(char argc, char *argv)
{
//    uint32_t val = bsp_get_tick();
//    printf("uptime: %ums\r\n", val);
}

static uint8_t _is_print = 1;
uint8_t cli_is_print(void)
{
    return _is_print;
}
static void cli_set_print_func(char argc, char *argv)
{
    if (argc >= 2)
    {
        int val = atoi((const char *)&argv[argv[1]]);
        if (val == 0 || val == 1)
        {
            _is_print = val;
        }
    }
}

// 指令表
const static_cmd_st static_cmd[] =
{
    {"help", cli_help_func},
    {"version", cli_version_func},
    {"set-print", cli_set_print_func},
    {"set-pwm", cli_set_pwm_func},
    {"set-lcd", cli_set_lcd_func},
    {"set-level", cli_set_level_func},
    {"set-start-level", cli_set_start_level_func},
    {"set-default", cli_set_defalut_func},
    {"show-config", cli_show_config_func},
    {"ts-reset", cli_ts_reset_func},
    {"uptime", cli_uptime_func},
    {"set-max-level", cli_set_max_level_func},
    {"\0", NULL}
};
