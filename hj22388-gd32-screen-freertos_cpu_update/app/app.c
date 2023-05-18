/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：app.c
* 摘要：创建任务
* 当前版本：1.0.1
* 历史版本：1.0.0
*/
#include "bsp.h"

/*
* 函数介绍：任务创建
* 参数：argument：任务创建输入参数
* 返回值：无
* 备注：无
*/
void AppTask(void *argument)
{
//    uint32_t startup_tick;
//    uint32_t startup_s = 0;
    bsp_ts_reset();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        bsp_led_toggle();  // led灯翻转
		fwdgt_counter_reload();  //喂狗
    }
}

void start_freertos(void)
{
    // 创建AppTask
    xTaskCreate(AppTask, "app", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	
	
	// 创建comm任务，用于串口通信
    xTaskCreate(StartCommTask, "comm", configMINIMAL_STACK_SIZE*2, NULL, 4, NULL);  //freetros 数值越大，优先级越高
    // 创建debug任务，用于串口调试
    xTaskCreate(StartDebugTask, "debug", configMINIMAL_STACK_SIZE*2, NULL, 3, NULL);
	
	xTaskCreate(StartlcdPwmTask, "lcdPwm", configMINIMAL_STACK_SIZE*2, NULL, 2, NULL);
	// 启动调度器，任务开始执行
    vTaskStartScheduler();
	
    while (1) ;
}

