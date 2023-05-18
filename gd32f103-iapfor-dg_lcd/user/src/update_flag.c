

#include "gd32f10x.h"
#include "update_flag.h"
#include "common.h"
/*
VBT6  总共有128Kflash，把前面24K拿出来用于iap，剩下108K用于app(起始地址0x6000)
其中0-22K共23k用于iap程序，23k(0x5c00)这个位置用于保存一些标志位update_flag_t结构体中的内容，

这个flash 是1k页，擦除和编程都是1k为单位。


状态标志的变化：
1. 下载完成后，未更新，擦除flash，写入md5码  mcu_download_done(void)
1.1 下载标志0xffff 表示不下载
1.2 更新标识0xffff 表示需要更新


2. 更新完成  mcu_update_done(void)
2.1 更新标志0xff，表示不需要更新
2.2 下载标志0xffff 表示不下载


3.进入下载模式，通讯串口控制    goto_ota_update(void)
2.1 下载标志0xff  表示需要下载
2.2 更新标志0xff，表示不需要更新
2.3 md5值还在
*/

extern uint8_t tab_1024[1024];
extern uint8_t md5sum_down[34];  //存放md5值
extern int32_t Size;

//起始地址
static update_flag_t *g_updateflag = (void*)UPDATE_FLAG_START_ADDR;



void goto_ota_update(void)
{
	printf("goto_ota_update \r\n");
		
	if(g_updateflag->need_download == (uint16_t)0xffff)  //已经是下载标志
	{	
		/* Flash unlock */
		fmc_unlock();
		//g_updateflag->need_update = 0x0f;    //表示是需要升级
		//g_updateflag->update_where = 0x0f;   //表示是通信串口升级
		fmc_halfword_program(UPDATE_FLAG_START_ADDR+2, 0xff);   //need_download需要下载
	}
	NVIC_SystemReset(); // 复位
}




//单片机是否需要升级？
//1.need_update 标志不是0xff或者0x0
//2.update_success 标志不是0xff或者0x0 表示上一次升级失败。
//返回值是0表示不需要升级，非0表示要升级
uint8_t is_mcu_need_update(void)
{
	if(g_updateflag->need_update == 0xffff)
	{
		SerialPutString("=is_mcu_need_update 1 ====\r\n");
		return 1;	 //需要升级
	}	
	return 0;  //不需要升级
}
	


//单片机是否需要下载，0xffff就是不需要下载,其他就是需要下载，
//返回1表示需要下载，0表示不需要
uint8_t is_mcu_need_download(void)
{
	if(g_updateflag->need_download != (uint16_t)0xffff)
	{
		SerialPutString("=is_mcu_need_update 1 ====\r\n");
		return 1;	 //需要升级
	}	
	return 0;  //不需要升级
}


int32_t cal_md5(unsigned char *result, unsigned char *data, int length);


uint8_t mcu_download_done(void)
{
	uint8_t i;
//	unsigned char result[16];
	unsigned char md5result[34] = {0};
	uint8_t *download_addr = (void*) ApplicationDownAddress; 
	uint32_t md5sum_addr = UPDATE_FLAG_START_ADDR + DOWN_MD5_OFFET; //md5的起始地址
	uint32_t size;
	
	size = Size > 1024? Size:g_updateflag->firm_size;
	if(size<1024 || size > FLASH_IMAGE_SIZE)
	{
		size = FLASH_IMAGE_SIZE;
	}
	
	if(!cal_md5(md5result, download_addr, size))   //计算下载的md5
	{		
		printf("cal_md5 md5result = %s\r\n",md5result);
		
		if(is_cpu_update_cmd)  //从cpu rk3399端下载的才需要对比
		{
			if(strncmp((char*)md5result,(void*)md5sum_down,32))  //比较md5，是否一致
			{
				printf("strncmp md5 error! not update!\n");
				return 1;
			}
		}
	}
	else  //没有算出md5值，对flash不修改
	{
		for(i=0;i<32;i++)
			md5result[i] = 0xff;
	}
		
	fmc_unlock();
	SerialPutString("=mcu_download_done====\r\n");
	fmc_page_erase(UPDATE_FLAG_START_ADDR);  //升级标志被设置，未成功标识被设置（需要app去清除）
	
	//设置需要升级，写入md5到download分区
	for(i=0;i<32;i+=4)
	{
		fmc_word_program(md5sum_addr, *(uint32_t*)(md5result+i));   //用计算的值存起来
		md5sum_addr += 4;   //app_addr 一直再更新
	}
	
	if(Size > 1024)  //至少大于1k，将数据的字节数写入
	{
		fmc_word_program(UPDATE_FLAG_START_ADDR+4, (uint32_t)(Size));
	}
	return 0;
}



//升级完成
void mcu_update_done(void)
{	
	SerialPutString("=mcu_update_done====\r\n");
//	fmc_page_erase(UPDATE_FLAG_START_ADDR);  //升级标志被清除，未成功标识被设置（需要app去清除）	
	fmc_unlock();
	fmc_halfword_program(UPDATE_FLAG_START_ADDR, 0xff);   //设置为0xff，表示不需要升级了	
}


//返回值1表示从调试串口升级，0则表示从通信串口升级
uint8_t is_update_from_debug_uart(void)
{
	return 0;
//	return g_updateflag->update_where == 0xffff;
}





//flash 拷贝
uint8_t flash_download_copyto_app(void)
{
//	uint8_t i;//,j
//	uint8_t count = 0;
	uint8_t *download_addr = (void*) ApplicationDownAddress;   //flash的大小，实际是76k(0x8013000)的位置，download的起始地址
	uint32_t app_addr = ApplicationAddress; //app的起始地址 
	uint32_t size,size_read = 0;

	fmc_unlock();
	
	if (((*(__IO uint32_t*)download_addr) & 0xfFFE0000 ) != 0x20000000)
	{
		printf("not update\r\n");
		mcu_update_done();   //更新标志，不再升级
		return 2;   //不能升级，rom不对
	}
	else if(((*(__IO uint32_t*)(download_addr+4)) & 0xfFFffc00 ) != ApplicationAddress)
	{
		printf("not update 3\r\n");
		mcu_update_done();   //更新标志，不再升级
		return 2;   //不能升级，rom不对
	}
	
	
	size = Size > 1024? Size:g_updateflag->firm_size;
	if(size<1024 || size > FLASH_IMAGE_SIZE)
	{
		size = FLASH_IMAGE_SIZE;
	}
	printf("flash_download_copyto_app size = %#x // %d\r\n",size,size);
	
	
	
	fmc_page_erase(app_addr);
	
	while(size_read< size)
	{		
		fmc_word_program(app_addr, *(uint32_t*)(download_addr+size_read));
		app_addr += 4;   //app_addr 一直再更新
		size_read += 4;
		
		if(app_addr%PAGE_SIZE == 0)
			fmc_page_erase(app_addr);
		
//		for(i=0;(i<1024);i++,size_read++)  //从download区读取出来
//		{
//			//tab_1024[i] = download_addr[size_read];
//			if(download_addr[size_read] == 0xff)  //连续出现10次0xff，拷贝结束
//			{
//				count++;
//				if(count >= 16)
//					break;
//			}
//			else
//			{
//				count = 0;
//			}			
//		}

//		//写入到app区
//		fmc_page_erase(app_addr);
//		
//		//不一定是写入1024个字节		
//		for(j=0;j<i;j+=4)
//		{
//			fmc_word_program(app_addr, *(uint32_t*)(tab_1024+j));
//			app_addr += 4;   //app_addr 一直再更新
//		}

//		if(count >= 16)  //完成
//		{
//			break;
//		}
	}
	
	//设置更新成功的标志，修改app md5值。
	mcu_update_done();
	
	return 0;
}




#if 1  

#define ROTATELEFT(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/**
 * @desc: convert message and mes_bkp string into integer array and store them in w
 */
static void md5_process_part1(uint32_t *w, unsigned char *message, uint32_t *pos, uint32_t mes_len, const unsigned char *mes_bkp)
{
    uint32_t i; // used in for loop

    for(i = 0; i <= 15; i++)
    {
        int32_t count = 0;
        while(*pos < mes_len && count <= 24)
        {
            w[i] += (((uint32_t)message[*pos]) << count);
            (*pos)++;
            count += 8;
        }
        while(count <= 24)
        {
            w[i] += (((uint32_t)mes_bkp[*pos - mes_len]) << count);
            (*pos)++;
            count += 8;
        }
    }
}

/**
 * @desc: start encryption based on w
 */
static void md5_process_part2(uint32_t abcd[4], uint32_t *w, const uint32_t k[64], const uint32_t s[64])
{
    uint32_t i; // used in for loop

    uint32_t a = abcd[0];
    uint32_t b = abcd[1];
    uint32_t c = abcd[2];
    uint32_t d = abcd[3];
    uint32_t f = 0;
    uint32_t g = 0;

    for(i = 0; i < 64; i++)
    {
        if(i <= 15) //i >= 0 && 
        {
            f = (b & c) | ((~b) & d);
            g = i;
        }else if(i >= 16 && i <= 31)
        {
            f = (d & b) | ((~d) & c);
            g = (5 * i + 1) % 16;
        }else if(i >= 32 && i <= 47)
        {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        }else if(i >= 48 && i <= 63)
        {
            f = c ^ (b | (~d));
            g = (7 * i) % 16;
        }
        uint32_t temp = d;
        d = c;
        c = b;
        b = ROTATELEFT((a + f + k[i] + w[g]), s[i]) + b;
        a = temp;
    }

    abcd[0] += a;
    abcd[1] += b;
    abcd[2] += c;
    abcd[3] += d;
}

static const uint32_t k_table[]={
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
    0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,
    0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,
    0xa679438e,0x49b40821,0xf61e2562,0xc040b340,0x265e5a51,
    0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,
    0xfcefa3f8,0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,
    0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,
    0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
    0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,0xf4292244,
    0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,
    0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,
    0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};

static const uint32_t s_table[]={
    7,12,17,22,7,12,17,22,7,12,17,22,7,
    12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,
    15,21,6,10,15,21,6,10,15,21,6,10,15,21
};

int32_t cal_md5(unsigned char *result, unsigned char *data, int length){
    if (result == NULL)
    {
        return 1;
    }

    uint32_t w[16];

    uint32_t i; // used in for loop

    uint32_t mes_len = length;
    uint32_t looptimes = (mes_len + 8) / 64 + 1;
    uint32_t abcd[] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

    uint32_t pos = 0; // position pointer for message
    uint32_t bkp_len = 64 * looptimes - mes_len; // 经过计算发现不超过72

//    unsigned char *bkp_mes = (unsigned char *)calloc(1, bkp_len);
    unsigned char bkp_mes[80];
    for(int i = 0; i < 80; i++) //初始化
    {
        bkp_mes[i] = 0;
    }

    bkp_mes[0] = (unsigned char)(0x80);
    uint64_t mes_bit_len = ((uint64_t)mes_len) * 8;
    for(i = 0; i < 8; i++)
    {
        bkp_mes[bkp_len-i-1] = (unsigned char)((mes_bit_len & (0x00000000000000FF << (8 * (7 - i)))) >> (8 * (7 - i)));
    }

    for(i = 0; i < looptimes; i++)
    {
        for(int j = 0; j < 16; j++) //初始化
        {
            w[j] = 0x00000000;
        }

        md5_process_part1(w, data, &pos, mes_len, bkp_mes); // compute w

        md5_process_part2(abcd, w, k_table, s_table); // calculate md5 and store the result in abcd
    }

    for(int i = 0; i < 16; i++)
    {
        //result[i] = ((unsigned char*)abcd)[i];
		sprintf((char*)result+i*2,"%02x",((unsigned char*)abcd)[i]);   //2023-05-09 返回字符串
    }

    return 0;
}

#endif



