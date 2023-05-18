/*
* Copyright(c)2022, 湖南航天捷诚电子装备有限责任公司
* ALL right reserved
* 文件名称：main.c
* 摘要：系统初始化
* 当前版本：1.0.1
* 历史版本：1.0.0

//2022-12-09  
PB7  LCD 亮度的pwm输出
亮度信息保存在eeprom中，iicbus1（PB10,PB11，模拟使用），地址0x57


*/
#include "bsp.h"

const char* g_build_time_str = "Buildtime :"__DATE__" "__TIME__;   //获得编译时间
static uint8_t g_McuVersion = 104;   //1.04,2023-05-11升级104

uint8_t GetMcuVersion(void)
{
	return g_McuVersion;
}


/*
* 函数介绍：初始化滴答定时器
* 参数：无
* 返回值：无
* 备注：FreeRTOS的系统时钟是由滴答定时器提供
*/
//static void systick_config(void)
//{
//    /* setup systick timer for 1000Hz interrupts */
//    if (SysTick_Config(SystemCoreClock / 1000U))
//    {
//        /* capture error */
//        while (1)
//        {
//        }
//    }
//    /* configure the systick handler priority */
//    NVIC_SetPriority(SysTick_IRQn, 0x00U);
//}

void nvic_config(void)
{
    // 配置中断分组
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
}




//800ms 看门狗
static void iwdog_init(void)
{	
	fwdgt_write_enable();	
	fwdgt_config(0xfff,FWDGT_PSC_DIV64);    //设置分配系数,FWDGT_PSC_DIV8最长800ms，FWDGT_PSC_DIV32最大3s		

	//	fwdgt_config(0xfff,FWDGT_PSC_DIV64);    //设置分配系数,最长6s	
	//fwdgt_counter_reload();  //等待时间约3s
	fwdgt_enable(); //使能看门狗

}







int main()
{
	nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x6000);   //注意变化！！！2023-02-01
	
	//0. 中断分组初始化
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);    //2022-09-09 优先级被我修改了，现在只有抢占优先级了！！
	
    // 外设资源初始化
    bsp_init();
	iwdog_init();
//    // 滴答定时器初始化
//    systick_config();
    // freertos任务创建
    start_freertos();
//	while(1) ;
    return 1;
}

