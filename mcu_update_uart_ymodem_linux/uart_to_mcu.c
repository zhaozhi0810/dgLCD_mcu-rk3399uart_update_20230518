// main.c 
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// #include <linux/input.h>
// #include <linux/uinput.h>
#include <linux/kd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>             // exit
#include <sys/ioctl.h>          // ioctl
#include <string.h>             // bzero
// #include <pthread.h>
// #include <semaphore.h>

#include <stdarg.h>

#include "ComFunc.h"


//#include "Common.h"

/*global defines*/
//static QUEUE keyCmdQueue;
//static QUEUE mouseCmdQueue;


static 	int p_opt = 0;   //打印调试信息，默认不打印
static 	int uart_fd;



static void show_version(char* name)
{
    printf( "%s Buildtime :"__DATE__" "__TIME__,name);
}
 
static void usage(char* name)
{
    show_version(name);
 
    printf("    -h,    short this help\n");
    printf("    -v,    show version\n");
    printf("    -d /dev/ttyS0, select com device\n");
    printf("    -p , printf recv data\n");
    printf("    -b , set baudrate\n");
    printf("    -n , set com nonblock mode\n");
    exit(0);
}



static const char* my_opt = "Dvhpwb:d:";

/* This function will open the uInput device. Please make 
sure that you have inserted the uinput.ko into kernel. */ 
int uart_init(int argc, char *argv[]) 
{
	int nonblock=1;   //默认改为非阻塞模式
//	int i=0;
	char* com_port = "/dev/ttyS4";   //导光的lcd是ttyS4，话音接口板是ttyS0

	int c;
	int baudrate = 115200;

	printf("Program %s is running\n", argv[0]);
    if(argc != 1)
	{
	//	printf("usage: ./kmUtil keyboardComName mouseComName\n");		
	    while(1)
	    {
	        c = getopt(argc, argv, my_opt);
	        //printf("optind: %d\n", optind);
	        if (c < 0)
	        {
	            break;
	        }
	        //printf("option char: %x %c\n", c, c);
	        switch(c)
	        {
	        case 'p':
	        		p_opt = 1;
	                //debug_level = atoi(optarg);
	                printf("p_opt = 1\n");
	                break;
	        case 'd':
	        	//	com_port = 	
	                if(strncmp(optarg,"/dev/tty",8) == 0)
	             		com_port = optarg;
	             	else
	             		printf("select device error ,start with /dev/tty please!\n");
	                printf("com_port = %s.\n\n",com_port);
	                break;
	        case 'b':
	        		baudrate = atoi(optarg);
	        		if(baudrate < 1200)
	        			baudrate = 115200;
	                printf("set baudrate is: %d\n\n", baudrate);
	             //   p1 = optarg;
	                break;
	        case 'n':
	                printf("set com nonblock mode\n\n");
	                nonblock = 1;
	                break;
	        case ':':
	                fprintf(stderr, "miss option char in optstring.\n");
	                break;
	        case 'D':
	        	break;
	        case '?':
	        case 'h':

	        default:
	                usage(argv[0]);
	                break;
	                //return 0;
	        }
	    }
	    if (optind == 1)
	    {
	        usage(argv[0]);
	    }
	}
 	
	uart_fd = PortOpen(com_port,nonblock);   //打开串口
	if( uart_fd < 0 )
	{
		printf("open searil port %s error\n",com_port);
		return -1;	
	}

	printf("open searil port %s success\n",com_port);

	return PortSet(uart_fd,baudrate,1,'N');    //设置波特率等	
}



//程序退出时，串口部分的处理
void uart_exit(void) 
{
	close(uart_fd);
}


/*
*********************************************************************************************************
*	函 数 名: UART_ReceiveByte
*	功能说明: 接收发送端发来的字符         
*	形    参：c  字符
*             timeout  溢出时间
*	返 回 值: 0 接收成功， -1 接收失败
*********************************************************************************************************
*/
int UART_ReceiveByte (uint8_t *c, uint32_t timeout)
{
	int ret;
	ret = PortRecv(uart_fd, c, 1,timeout);

	if(ret<=0)
		printf("ERROR:UART_ReceiveByte\n");

	return (ret>0)?0:-1;
}

/*
*********************************************************************************************************
*	函 数 名: UART_ReceivePacket
*	功能说明: 接收发送端发来的字符         
*	形    参：data  数据
*             timeout  溢出时间
*	返 回 值: 0 接收成功， -1 接收失败
*********************************************************************************************************
*/
int UART_ReceivePacket (uint8_t *data, uint16_t length, uint32_t timeout)
{
	uint8_t i;
	int ret;

	ret = PortRecv(uart_fd, data, length,timeout);   //返回读到的字节数

	if(ret <= 0)
		printf("ERROR:UART_ReceivePacket,length = %d,ret = %d\n",length,ret);
	else
	{
		printf("UART_ReceivePacket,length = %d,ret = %d\n",length,ret);
		for(i=0;i<ret;i++)
			printf("%#x ",data[i]);
		printf("\n");


	}	
	return (length == ret)?0:-1;
}






/*
*********************************************************************************************************
*	函 数 名: Uart_SendByte
*	功能说明: 发送一个字节数据         
*	形    参：c  字符
*	返 回 值: 0
*********************************************************************************************************
*/
void UART_SendByte (uint8_t c)
{
	PortSend(uart_fd,&c, 1);
}




/*
*********************************************************************************************************
*	函 数 名: UART_SendPacket
*	功能说明: 发送一串数据
*	形    参: data  数据
*             length  数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void UART_SendPacket(uint8_t *data, uint16_t length)
{
	PortSend(uart_fd,data, length);
}



