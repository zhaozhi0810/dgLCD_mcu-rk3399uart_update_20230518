/*
* @Author: dazhi
* @Date:   2023-05-09 09:30:03
* @Last Modified by:   dazhi
* @Last Modified time: 2023-05-18 11:14:15
*
* 如果使用md5sum的命令，则在windows下编译时，无法计算md值
* 下载mingw64：
* https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/mingw-w64-v11.0.0.zip
* D:\Programfile\mingw64\bin\gcc combin.c -o combin
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>
#include <libgen.h>
//#include "md5.h"

char md5_readBuf[64] = {0};


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



int get_file_md5sum(const char * filename)
{
	FILE * fin1;
//	char cmd[128] = {"md5sum "};
	int size1 ;
	int ret;
	int readcount,bw;

    fin1 = fopen(filename, "rb");
    if (fin1 != NULL)
    {
        /* 文件打开成功*/
        printf("open %s success\r\n",filename);
    }
    else
    {
        printf("open %s error\r\n",filename);
        return 1;
    }

    fseek(fin1, 0, SEEK_END);
    size1 = ftell(fin1);
	fseek(fin1, 0, SEEK_SET); 


    printf("file size = %d\r\n", size1);

    char* buf = malloc(size1);  //开一个空间
    if(buf == NULL)
    {
    	printf("error: size1 malloc %d\n",size1);
    	fclose(fin1);
    	return 1;
    }


    readcount = 0;
	do
    {
        bw = fread(buf+readcount, 1, size1-readcount, fin1);
        if(bw == 0)
        	break;
        readcount += bw;
    } while (readcount < size1);

    ret = cal_md5(md5_readBuf, buf, size1);
    //md5(buf, size1, md5_readBuf);
    if(!ret) 
    	printf("cal_md5 = %s\n",md5_readBuf);


    free(buf);
    fclose(fin1);
// 	strcat(cmd,filename);

// 	filep = popen(cmd,"r");
// 	if(!filep)
// 		return -1;
//     ret = fread(md5_readBuf,32,1,filep);
 
// //    printf("get_file_md5sum = %s\n",md5_readBuf);

//     pclose(filep);

    return ret;
}



int write_tofile(char* filename,char* buf,int total_size)
{
    FILE *fin1;
    int bw = 0;       
    int readcount = 0;
    int ret,i;

        //生成第三个文件
    fin1 = fopen(filename, "wb");
    if (fin1 != NULL)
    {
        /* 文件打开成功*/
        printf("3.open %s success \n",filename);
    }
    else
    {
        printf("3.open %s error \n",filename);
    //    free(buf);
        return -1;
    }

    //写入文件
    readcount = 0;
    do
    {
        bw = fwrite(&buf[readcount], 1, total_size, fin1);
        readcount += bw;
    } while (readcount < total_size);

    fclose(fin1);

    return 0;
}





void combin_file(char* file1,char* file2,char* outfilename)
{
	FILE *fin1,*fin2;// *fout;
    int size1 = 0,size2 = 0,total_size = 0;
    char outnamebuf[64] = {0};
    uint16_t posision = 0;   //位置
    int bw = 0;       
    int readcount = 0;
    int ret,i;
    int lcd_detect = 0;

    if(file1== NULL || file2 == NULL)
    {
    	printf("ERROR : file is NULL \n");
    	return ;
    }

    get_file_md5sum(file2);

    fin1 = fopen(file1, "rb");
    if (fin1 != NULL)
    {
        /* 文件打开成功*/
        printf("open %s success\r\n",file1);
    }
    else
    {
        printf("open %s error\r\n",file1);
        return ;
    }

    fin2 = fopen(file2, "rb");
    if (fin2 != NULL)
    {
        /* 文件打开成功*/
        printf("open %s success\r\n",file2);
    }
    else
    {
    	fclose(fin1);
        printf("open %s error\r\n",file2);
        return ;
    }

    fseek(fin2, 0, SEEK_END);
    size2 = ftell(fin2);
    
	fseek(fin1, 0, SEEK_END);
    size1 = ftell(fin1);
	fseek(fin1, 0, SEEK_SET); 


    printf("file2 size = %d\r\n", size2);
    total_size = size2+0x6000;
    char* buf = malloc(total_size);  //开一个空间
    if(buf == NULL)
    {
    	printf("error: total_size malloc %d\n",total_size);
    	fclose(fin1);
    	fclose(fin2);
    	return ;
    }

    memset(buf,0xff,total_size);   //先填充0xff

	
	//app文件先偏移4个字节
	fseek(fin2, 0, SEEK_SET);
	ret = fread(buf, 1, 8, fin2);   //从app文件中读取4个字节
	if(ret == 8)
	{
		if (((*(uint32_t*)buf) & 0xfFFE0000 ) != 0x20000000)
		{
			printf("image addr 0 != 0x20000000\n");
			printf("ERROR: bad image(app.bin)!!!!! combin cancle!!!,please check bin file!!!");
			fclose(fin1);
    		fclose(fin2);
			free(buf);
			return ;
		}
		else if(((*(uint32_t*)(buf+4)) & 0xfFFf0000 ) != 0x08000000)
		{
			printf("image  addr %#x != ApplicationAddress %#x\n",((*(uint32_t*)(buf+4)) & 0xfFFffc00 ),0x08000000);
			printf("ERROR: bad image(app.bin)!!!!! combin cancle!!!,please check again!!!");
			fclose(fin1);
    		fclose(fin2);
			free(buf);
			return ;
		}

		posision = ((*(uint16_t*)(buf+4)) & 0xfc00 );
		printf("posision = %#x\n",posision);
	}
	else
	{
		printf("error: fread(buf, 1, 2, fin2)\n");
        fclose(fin1);
        fclose(fin2);
        free(buf);
		return;
	}

    if(posision < 2048)
    {
        printf("ERROR: bad app image(bin)!!!!! ,please check app.bin!!!\n");
        fclose(fin1);
        fclose(fin2);
        free(buf);
        return;
    }

	//iap的大小超过了
    if(size1 > posision-1024)
	{
		printf("ERROR: bad iap image(bin)!!!!! ,please check iap.bin!!!\n");
		fclose(fin1);
		fclose(fin2);
		free(buf);
	}

	if(posision <= 0x6000)
	{
		total_size = total_size - 0x6000 + posision;   //重新调整一下大小
		printf("total_size = %d\n",total_size);
	}
	else
	{
		printf("ERROR: bad app image(bin)!!!!! ,please check app.bin!!!\n");
		fclose(fin1);
        fclose(fin2);
        free(buf);
        return;
	}


	//文件2指针还原
	fseek(fin2, 0, SEEK_SET);

	//读文件1
	printf("size1 = %d\n",size1);
	readcount = 0;
	do
    {
        bw = fread(buf+readcount, 1, size1-readcount, fin1);
        if(bw == 0)
        	break;
        readcount += bw;
    } while (readcount < size1);



    if (((*(uint32_t*)buf) & 0xfFFE0000 ) != 0x20000000)
	{
		printf("image addr 0 != 0x20000000\n");
		printf("ERROR: bad image(iap.bin)!!!!! combin cancle!!!,please check bin file!!!\n");
		fclose(fin1);
		fclose(fin2);
		free(buf);
		return ;
	}
	else if(((*(uint32_t*)(buf+4)) & 0xfFFffc00 ) != 0x08000000)
	{
		printf("image  addr %#x != ApplicationAddress %#x\n",((*(uint32_t*)(buf+4)) & 0xfFFffc00 ),0x08000000);
		printf("ERROR: bad image(iap.bin)!!!!! combin cancle!!!,please check again!!!\n");
		fclose(fin1);
		fclose(fin2);
		free(buf);
		return ;
	}



    //读文件2
    printf("size2 = %d\n",size2);
	readcount = posision;  //app的位置，在bin中已经指定
	do
    {
        bw = fread(&buf[readcount], 1, size2, fin2);
        if(bw == 0)
        	break;
        readcount += bw;
    } while (readcount-posision < size2);


    //加入文件大小，md5值
    posision -= 0x400;   //退1k字节
    printf("posion2 = %#x\n",posision);
    (*(uint32_t*)(buf+4+posision)) = size2;  //只保存app 的bin文件大小
    (*(uint16_t*)(buf+posision)) = 0xff;     //不需要升级





    posision += 512;   //写入md5的值。
    printf("posion3 = %#x\n",posision);
    for(i=0;i<32;i++)
    {
    	buf[i+posision] = md5_readBuf[i];
    }

    fclose(fin1);
    fclose(fin2);


    if(strstr(file2,"lcd") != NULL)
    {
    //    lcd_inch = 4; 
        lcd_detect = 1;   //需要生成3个文件
        //(*(uint8_t*)(buf+posision-1)) = 5;
    }

    //形成输出文件名
    if(outfilename == NULL)
    {
        size2 = strlen(file2)-4;
        if(size2 > 59)
            size2 = 59;

        strncat(outnamebuf,file2,size2);  //拷贝的时候不包括.bin
    }
    else
    {
        size1 = strlen(outfilename);
        size2 = size1;
        if(size1 > 63)  //文件名太长了
        {
            
            size1 = 59;
        }
        else if(lcd_detect)
              size1 -= 4;


        strncpy(outnamebuf,outfilename,size1);

        if(!lcd_detect && size2 > 63)
        {
            strcat(outnamebuf,".bin");
        }
    }   
    printf("outnamebuf = %s\n",outnamebuf);


    if(lcd_detect)
    {
        strcat(outnamebuf,"-old5.bin");
        (*(uint8_t*)(buf+posision-513)) = 5;
        write_tofile(outnamebuf,buf,total_size);
        outnamebuf[strlen(outnamebuf)-9] = '\0';
        (*(uint8_t*)(buf+posision-513)) = 6;
        strcat(outnamebuf,"-new5.bin");
        write_tofile(outnamebuf,buf,total_size);
        outnamebuf[strlen(outnamebuf)-9] = '\0';
        (*(uint8_t*)(buf+posision-513)) = 4;
        strcat(outnamebuf,"-7inc.bin");
        write_tofile(outnamebuf,buf,total_size);
    }
    else
    {
        write_tofile(outnamebuf,buf,total_size);

     //    //生成第三个文件
     //    fin1 = fopen(outnamebuf, "wb");
     //    if (fin1 != NULL)
     //    {
     //        /* 文件打开成功*/
     //        printf("3.open %s success \n",outnamebuf);
     //    }
     //    else
     //    {
     //        printf("3.open %s error \n",outnamebuf);
     //        free(buf);
     //        return ;
     //    }

     //    //写入文件
     //    readcount = 0;
    	// do
     //    {
     //        bw = fwrite(&buf[readcount], 1, total_size, fin1);
     //        readcount += bw;
     //    } while (readcount < total_size);

     //    fclose(fin1);
    }

    free(buf);

    //生成md5文件
    size2 = strlen(outfilename) -3;
    if(size2 > 60)
        size2 = 60;  //防止数组越界
    strncpy(outnamebuf,outfilename,size2); //不需要后缀
    outnamebuf[size2] = '\0';
    strcat(outnamebuf,"md5");
    fin1 = fopen(outnamebuf, "wb");
    if (fin1 != NULL)
    {
        /* 文件打开成功*/
        printf("3.open %s success \n",outnamebuf);
    }
    else
    {
        printf("3.open %s error \n",outnamebuf);
        free(buf);
        return ;
    }

    //写入文件
    readcount = 0;
    fwrite(md5_readBuf, 1, 32, fin1);

    fclose(fin1);

    printf("combin complete!! size = %d\n",total_size);

    return;
}




int main(int argc,char* argv[])
{
	char* outfilename = NULL;

	if(argc < 3){
		printf("Usage : %s <file1.bin> <file2.bin> [outfilename]\n",argv[0]);
		return -1;
	}

	if(argc >= 4)
	{
		outfilename = argv[3];
	}

	//第一个文件，与第二个文件合并，生成第三个文件，第三个可以不指定。
	combin_file(argv[1],argv[2],outfilename);



	return 0;

}



