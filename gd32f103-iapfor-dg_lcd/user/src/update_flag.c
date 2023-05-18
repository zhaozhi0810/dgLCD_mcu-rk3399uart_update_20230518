

#include "gd32f10x.h"
#include "update_flag.h"
#include "common.h"
/*
VBT6  �ܹ���128Kflash����ǰ��24K�ó�������iap��ʣ��108K����app(��ʼ��ַ0x6000)
����0-22K��23k����iap����23k(0x5c00)���λ�����ڱ���һЩ��־λupdate_flag_t�ṹ���е����ݣ�

���flash ��1kҳ�������ͱ�̶���1kΪ��λ��


״̬��־�ı仯��
1. ������ɺ�δ���£�����flash��д��md5��  mcu_download_done(void)
1.1 ���ر�־0xffff ��ʾ������
1.2 ���±�ʶ0xffff ��ʾ��Ҫ����


2. �������  mcu_update_done(void)
2.1 ���±�־0xff����ʾ����Ҫ����
2.2 ���ر�־0xffff ��ʾ������


3.��������ģʽ��ͨѶ���ڿ���    goto_ota_update(void)
2.1 ���ر�־0xff  ��ʾ��Ҫ����
2.2 ���±�־0xff����ʾ����Ҫ����
2.3 md5ֵ����
*/

extern uint8_t tab_1024[1024];
extern uint8_t md5sum_down[34];  //���md5ֵ
extern int32_t Size;

//��ʼ��ַ
static update_flag_t *g_updateflag = (void*)UPDATE_FLAG_START_ADDR;



void goto_ota_update(void)
{
	printf("goto_ota_update \r\n");
		
	if(g_updateflag->need_download == (uint16_t)0xffff)  //�Ѿ������ر�־
	{	
		/* Flash unlock */
		fmc_unlock();
		//g_updateflag->need_update = 0x0f;    //��ʾ����Ҫ����
		//g_updateflag->update_where = 0x0f;   //��ʾ��ͨ�Ŵ�������
		fmc_halfword_program(UPDATE_FLAG_START_ADDR+2, 0xff);   //need_download��Ҫ����
	}
	NVIC_SystemReset(); // ��λ
}




//��Ƭ���Ƿ���Ҫ������
//1.need_update ��־����0xff����0x0
//2.update_success ��־����0xff����0x0 ��ʾ��һ������ʧ�ܡ�
//����ֵ��0��ʾ����Ҫ��������0��ʾҪ����
uint8_t is_mcu_need_update(void)
{
	if(g_updateflag->need_update == 0xffff)
	{
		SerialPutString("=is_mcu_need_update 1 ====\r\n");
		return 1;	 //��Ҫ����
	}	
	return 0;  //����Ҫ����
}
	


//��Ƭ���Ƿ���Ҫ���أ�0xffff���ǲ���Ҫ����,����������Ҫ���أ�
//����1��ʾ��Ҫ���أ�0��ʾ����Ҫ
uint8_t is_mcu_need_download(void)
{
	if(g_updateflag->need_download != (uint16_t)0xffff)
	{
		SerialPutString("=is_mcu_need_update 1 ====\r\n");
		return 1;	 //��Ҫ����
	}	
	return 0;  //����Ҫ����
}


int32_t cal_md5(unsigned char *result, unsigned char *data, int length);


uint8_t mcu_download_done(void)
{
	uint8_t i;
//	unsigned char result[16];
	unsigned char md5result[34] = {0};
	uint8_t *download_addr = (void*) ApplicationDownAddress; 
	uint32_t md5sum_addr = UPDATE_FLAG_START_ADDR + DOWN_MD5_OFFET; //md5����ʼ��ַ
	uint32_t size;
	
	size = Size > 1024? Size:g_updateflag->firm_size;
	if(size<1024 || size > FLASH_IMAGE_SIZE)
	{
		size = FLASH_IMAGE_SIZE;
	}
	
	if(!cal_md5(md5result, download_addr, size))   //�������ص�md5
	{		
		printf("cal_md5 md5result = %s\r\n",md5result);
		
		if(is_cpu_update_cmd)  //��cpu rk3399�����صĲ���Ҫ�Ա�
		{
			if(strncmp((char*)md5result,(void*)md5sum_down,32))  //�Ƚ�md5���Ƿ�һ��
			{
				printf("strncmp md5 error! not update!\n");
				return 1;
			}
		}
	}
	else  //û�����md5ֵ����flash���޸�
	{
		for(i=0;i<32;i++)
			md5result[i] = 0xff;
	}
		
	fmc_unlock();
	SerialPutString("=mcu_download_done====\r\n");
	fmc_page_erase(UPDATE_FLAG_START_ADDR);  //������־�����ã�δ�ɹ���ʶ�����ã���Ҫappȥ�����
	
	//������Ҫ������д��md5��download����
	for(i=0;i<32;i+=4)
	{
		fmc_word_program(md5sum_addr, *(uint32_t*)(md5result+i));   //�ü����ֵ������
		md5sum_addr += 4;   //app_addr һֱ�ٸ���
	}
	
	if(Size > 1024)  //���ٴ���1k�������ݵ��ֽ���д��
	{
		fmc_word_program(UPDATE_FLAG_START_ADDR+4, (uint32_t)(Size));
	}
	return 0;
}



//�������
void mcu_update_done(void)
{	
	SerialPutString("=mcu_update_done====\r\n");
//	fmc_page_erase(UPDATE_FLAG_START_ADDR);  //������־�������δ�ɹ���ʶ�����ã���Ҫappȥ�����	
	fmc_unlock();
	fmc_halfword_program(UPDATE_FLAG_START_ADDR, 0xff);   //����Ϊ0xff����ʾ����Ҫ������	
}


//����ֵ1��ʾ�ӵ��Դ���������0���ʾ��ͨ�Ŵ�������
uint8_t is_update_from_debug_uart(void)
{
	return 0;
//	return g_updateflag->update_where == 0xffff;
}





//flash ����
uint8_t flash_download_copyto_app(void)
{
//	uint8_t i;//,j
//	uint8_t count = 0;
	uint8_t *download_addr = (void*) ApplicationDownAddress;   //flash�Ĵ�С��ʵ����76k(0x8013000)��λ�ã�download����ʼ��ַ
	uint32_t app_addr = ApplicationAddress; //app����ʼ��ַ 
	uint32_t size,size_read = 0;

	fmc_unlock();
	
	if (((*(__IO uint32_t*)download_addr) & 0xfFFE0000 ) != 0x20000000)
	{
		printf("not update\r\n");
		mcu_update_done();   //���±�־����������
		return 2;   //����������rom����
	}
	else if(((*(__IO uint32_t*)(download_addr+4)) & 0xfFFffc00 ) != ApplicationAddress)
	{
		printf("not update 3\r\n");
		mcu_update_done();   //���±�־����������
		return 2;   //����������rom����
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
		app_addr += 4;   //app_addr һֱ�ٸ���
		size_read += 4;
		
		if(app_addr%PAGE_SIZE == 0)
			fmc_page_erase(app_addr);
		
//		for(i=0;(i<1024);i++,size_read++)  //��download����ȡ����
//		{
//			//tab_1024[i] = download_addr[size_read];
//			if(download_addr[size_read] == 0xff)  //��������10��0xff����������
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

//		//д�뵽app��
//		fmc_page_erase(app_addr);
//		
//		//��һ����д��1024���ֽ�		
//		for(j=0;j<i;j+=4)
//		{
//			fmc_word_program(app_addr, *(uint32_t*)(tab_1024+j));
//			app_addr += 4;   //app_addr һֱ�ٸ���
//		}

//		if(count >= 16)  //���
//		{
//			break;
//		}
	}
	
	//���ø��³ɹ��ı�־���޸�app md5ֵ��
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
    uint32_t bkp_len = 64 * looptimes - mes_len; // �������㷢�ֲ�����72

//    unsigned char *bkp_mes = (unsigned char *)calloc(1, bkp_len);
    unsigned char bkp_mes[80];
    for(int i = 0; i < 80; i++) //��ʼ��
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
        for(int j = 0; j < 16; j++) //��ʼ��
        {
            w[j] = 0x00000000;
        }

        md5_process_part1(w, data, &pos, mes_len, bkp_mes); // compute w

        md5_process_part2(abcd, w, k_table, s_table); // calculate md5 and store the result in abcd
    }

    for(int i = 0; i < 16; i++)
    {
        //result[i] = ((unsigned char*)abcd)[i];
		sprintf((char*)result+i*2,"%02x",((unsigned char*)abcd)[i]);   //2023-05-09 �����ַ���
    }

    return 0;
}

#endif



