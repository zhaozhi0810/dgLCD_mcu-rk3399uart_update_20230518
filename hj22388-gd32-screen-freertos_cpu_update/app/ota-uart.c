/*
* @Author: dazhi
* @Date:   2023-02-01 19:42:13
* @Last Modified by:   dazhi
* @Last Modified time: 2023-02-01 20:02:03
*/

/*
	适用于跳转到ota程序，进行串口ota升级

	1. 之前OTA程序的弊端是OTA放到flash的前端，这时单片机的应用程序不可调试，也不能使用keil直接下载。
	2. 解决1这个问题，就想着把OTA的部分放到Flash的后面去，单片机的程序还是前面。
	3. 单片机增加一个串口指令，跳转到OTA去执行

 */
#include "gd32f10x.h"
#include "common.h"


typedef  void (*pFunction)(void);


#define UPDATE_FLAG_START_ADDR   (ApplicationAddress-PAGE_SIZE)     //设置起始地址，0x805c00

//uint8_t update_success_flag = 0;  //升级成功设置为1，否则为0
//static uint8_t mcu_update_flag = 0;    //升级标志


typedef struct update_flag
{
	uint16_t need_update;    //需要升级（从back区拷贝到app区）吗？0xffff是需要升级(同时表示升级不成功)，0x00ff表示升级成功
	uint16_t need_download;    //需要升级吗？0xffff是不需要下载(同时表示下载成功)，0x00ff表示需要下载，
	uint32_t firm_size;       //固件大小，下载的值
}update_flag_t;

//起始地址
static update_flag_t *g_updateflag = (void*)UPDATE_FLAG_START_ADDR;

void goto_ota_update(void)
{
	printf("goto_ota_update \r\n");
		
	if(g_updateflag->need_download == 0xffff)  //不是下载标志
	{	
		/* Flash unlock */
		fmc_unlock();
		//g_updateflag->need_update = 0x0f;    //表示是需要升级
		//g_updateflag->update_where = 0x0f;   //表示是通信串口升级
		fmc_halfword_program(UPDATE_FLAG_START_ADDR+2, 0xff);   //need_download需要下载
	}
	NVIC_SystemReset(); // 复位
}











