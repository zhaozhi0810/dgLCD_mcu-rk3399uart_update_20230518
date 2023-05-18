/**
  ******************************************************************************
  * @file    IAP/src/common.c 
  * @author  MCD Application Team
  * @version V3.3.1
  * @date    01/31/2023
  * @brief   This file provides all the common functions.
  ******************************************************************************
	±¾°æ±¾½öÕë¶Ôgd32f103VBT6 ÄÚ´æ20K£¬flash 128KµÄÇé¿ö£¬ÆäËûÇé¿öÃ»ÓÐ²âÊÔ
  */ 

/** @addtogroup IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "ymodem.h"
#include "uart.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t BlockNbr = 0, UserMemoryMask = 0;
__IO uint32_t FlashProtection = 0;
extern uint32_t FlashDestination;
extern uint8_t is_cpu_update_cmd;   //ÊÇrk3399µÄÉý¼¶Âð£¿
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Convert an Integer to a string
  * @param  str: The string
  * @param  intnum: The intger to be converted
  * @retval None
  */
void Int2Str(uint8_t* str, int32_t intnum)
{
  uint32_t i, Div = 1000000000, j = 0, Status = 0;

  for (i = 0; i < 10; i++)
  {
    str[j++] = (intnum / Div) + 48;

    intnum = intnum % Div;
    Div /= 10;
    if ((str[j-1] == '0') & (Status == 0))
    {
      j = 0;
    }
    else
    {
      Status++;
    }
  }
}

/**
  * @brief  Convert a string to an integer
  * @param  inputstr: The string to be converted
  * @param  intnum: The intger value
  * @retval 1: Correct
  *         0: Error
  */
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum)
{
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X'))
  {
    if (inputstr[2] == '\0')
    {
      return 0;
    }
    for (i = 2; i < 11; i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
      if (ISVALIDHEX(inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* over 8 digit hex --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }
  else /* max 10-digit decimal input */
  {
    for (i = 0;i < 11;i++)
    {
      if (inputstr[i] == '\0')
      {
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0))
      {
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      }
      else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0))
      {
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      }
      else if (ISVALIDDEC(inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11)
    {
      res = 0;
    }
  }

  return res;
}



/**
  * @brief  Get an integer from the HyperTerminal
  * @param  num: The inetger
  * @retval 1: Correct
  *         0: Error
  */
//uint32_t GetIntegerInput(int32_t * num)
//{
//  uint8_t inputstr[16];

//  while (1)
//  {
//    GetInputString(inputstr);
//    if (inputstr[0] == '\0') continue;
//    if ((inputstr[0] == 'a' || inputstr[0] == 'A') && inputstr[1] == '\0')
//    {
//      SerialPutString("User Cancelled \r\n");
//      return 0;
//    }

//    if (Str2Int(inputstr, num) == 0)
//    {
//      SerialPutString("Error, Input again: \r\n");
//    }
//    else
//    {
//      return 1;
//    }
//  }
//}


/**
	 ´Óµ÷ÊÔ¶Ë¿Ú»ñÈ¡×Ö·û
  * @brief  Get a key from the HyperTerminal
  * @param  None
  * @retval The Key Pressed
  */
uint8_t GetKey(void)
{
  uint8_t key = 0;

  /* Waiting for user input */
  while (1)
  {
    if (SerialKeyPressed_Uart0((uint8_t*)&key)) break;
  }
  return key;
}

//´ÓÏÂÔØ¶Ë¿Ú»ñµÃ×Ö·û
uint8_t Uploader_Get_Ready(void)
{
  uint8_t key = 0;

  /* Waiting for user input */
  while (1)
  {
    if (SerialKeyPressed((uint8_t*)&key)) break;
  }
  return key;
}



/**
  * @brief  Print a string on the HyperTerminal
  * @param  s: The string to be printed
  * @retval None
  */
//void Serial_PutString(uint8_t *s)
//{
//  while (*s != '\0')
//  {
//    SerialPutChar(*s);
//    s++;
//  }
//}


void Serial_PutString_Uart0(uint8_t *s)
{
  while (*s != '\0')
  {
    SerialPutChar_uart0(*s);
    s++;
  }
}



void Serial_PutString_Uart1(uint8_t *s)
{
  while (*s != '\0')
  {
    SerialPutChar_uart1(*s);
    s++;
  }
}

/**
  * @brief  Get Input string from the HyperTerminal
  * @param  buffP: The input string
  * @retval None
  */
//void GetInputString (uint8_t * buffP)
//{
//  uint32_t bytes_read = 0;
//  uint8_t c = 0;
//  do
//  {
//    c = GetKey();
//    if (c == '\r')
//      break;
//    if (c == '\b') /* Backspace */
//    {
//      if (bytes_read > 0)
//      {
//        SerialPutString("\b \b");
//        bytes_read --;
//      }
//      continue;
//    }
//    if (bytes_read >= CMD_STRING_SIZE )
//    {
//      SerialPutString("Command string size overflow\r\n");
//      bytes_read = 0;
//      continue;
//    }
//    if (c >= 0x20 && c <= 0x7E)
//    {
//      buffP[bytes_read++] = c;
//      SerialPutChar(c);
//    }
//  }
//  while (1);
//  SerialPutString(("\n\r"));
//  buffP[bytes_read] = '\0';
//}

/**
  * @brief  Calculate the number of pages
  * @param  Size: The image size
  * @retval The number of pages
  */
uint32_t FLASH_PagesMask(__IO uint32_t Size)
{
  uint32_t pagenumber = 0x0;
  uint32_t size = Size;

  if ((size % PAGE_SIZE) != 0)
  {
    pagenumber = (size / PAGE_SIZE) + 1;
  }
  else
  {
    pagenumber = size / PAGE_SIZE;
  }
  return pagenumber;

}

/**
  * @brief  Disable the write protection of desired pages
  * @param  None
  * @retval None
  */
void FLASH_DisableWriteProtectionPages(void)
{
  uint32_t useroptionbyte = 0, WRPR = 0;
  uint16_t var1 = OB_FWDGT_SW, var2 = OB_DEEPSLEEP_NRST, var3 = OB_STDBY_NRST;
  fmc_state_enum status = FMC_BUSY;

  WRPR = ob_write_protection_get();

  /* Test if user memory is write protected */
  if ((WRPR & UserMemoryMask) != UserMemoryMask)
  {
    useroptionbyte = ob_user_get();

    UserMemoryMask |= WRPR;

    status = ob_erase();

    if (UserMemoryMask != 0xFFFFFFFF)
    {
      status = ob_write_protection_enable((uint32_t)~UserMemoryMask);
    }

    /* Test if user Option Bytes are programmed */
    if ((useroptionbyte & 0x07) != 0x07)
    { 
      /* Restore user Option Bytes */
      if ((useroptionbyte & 0x01) == 0x0)
      {
        var1 = OB_FWDGT_HW;
      }
      if ((useroptionbyte & 0x02) == 0x0)
      {
        var2 = OB_DEEPSLEEP_RST;
      }
      if ((useroptionbyte & 0x04) == 0x0)
      {
        var3 = OB_STDBY_RST;
      }

      ob_user_write(var1, var2, var3,OB_BOOT_B0);
    }

    if (status == FMC_READY)
    {
      SerialPutString("Write Protection disabled...\r\n");

      SerialPutString("...and a System Reset will be generated to re-load the new option bytes\r\n");

      /* Generate System Reset to load the new option byte values */
      NVIC_SystemReset();
    }
    else
    {
      SerialPutString("Error: Flash write unprotection failed...\r\n");
    }
  }
  else
  {
    SerialPutString("Flash memory not write protected\r\n");
  }
}


void uart1_print_help(void)
{
	/* Test if any page of Flash memory where program user will be loaded is write protected */
	if ((ob_write_protection_get() & UserMemoryMask) != UserMemoryMask)
	{
		FlashProtection = 1;
	}
	else
	{
		FlashProtection = 0;
	}
	
	SerialPutString("\r\n================== Main Menu ============================\r\n");
	SerialPutString("1.  Download Image To the GD32F10x Internal Flash (debug  uart0)\r\n");
	SerialPutString("2.  Reboot the system , Run APP \r\n");	
//	SerialPutString("8.  Download Image To the GD32F10x Internal Flash (from rk3399 uart1)\r\n");		
	//SerialPutString("9.  Upload Image From the GD32F10x Internal Flash (debug  uart0)\r\n");
	if(FlashProtection != 0)
	{
		SerialPutString("3.  Disable the write protection\r\n");
	}

	SerialPutString("==========================================================\r\n");
}



void get_cmd_from_debug_uart1(void)
{
	uint8_t key;
	
	if(SerialKeyPressed_Uart1((uint8_t*)&key))
	{
		if (key == '1')
		{
			set_download_uart(0);
			is_cpu_update_cmd = 0;
			/* Download user application in the Flash */
			if(0==SerialDownload())
			{
				printf("debug uart SerialDownload done!\r\n");
				if(!mcu_download_done()){
					flash_download_copyto_app();  //ÏÂÔØ³É¹¦ºóÍê³ÉÒ»´Î¿½±´¡£
					printf("debug uart update done!\r\n");
					NVIC_SystemReset();
				}
			}
		}
		else if (key == '9')   //Òþ²ØÃüÁî
		{
			set_download_uart(0);
			/* Upload user application from the Flash */
			SerialUpload();
		}
		else if (key == '8')   //Òþ²ØÃüÁî
		{
			set_download_uart(1);
			is_cpu_update_cmd = 1; //´Ó3399ÏÂÔØ
			goto_ota_update();   //Ö8ØÆô½øÈërk3399ÏÂÔØÄ£Ê½
		}
		else if (key == '2')
		{
			NVIC_SystemReset();    //2023-02-02  ¸ÄÎªÖØÆôÁË
//			JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);

//			/* Jump to user application */
//			Jump_To_Application = (pFunction) JumpAddress;
//			/* Initialize user application's Stack Pointer */
//			__set_MSP(*(__IO uint32_t*) ApplicationAddress);
//			Jump_To_Application();
		}
		else if ((key == '3') && (FlashProtection == 1))
		{
			/* Disable the write protection of desired pages */
			FLASH_DisableWriteProtectionPages();
		}
		else
		{
			uart1_print_help();
//			if (FlashProtection == 0)
//			{
//				SerialPutString("Invalid Number ! ==> The number should be either 1, 2 or 3\r");
//			}
//			else
//			{
//				SerialPutString("Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
//			} 
		}
	}
}








void get_cmd_from_rk3399_uart0(void)
{
	uint8_t key,i;
	uint8_t *pflash_md5 = (void*)(UPDATE_FLAG_START_ADDR + DOWN_MD5_OFFET);
	static uint8_t buf[8];
	static uint8_t index = 0;
	uint8_t  checksum = 0;
	
	if(SerialKeyPressed_Uart0((uint8_t*)&key))
	{		
		if(key == 0xa5)
			index = 0;
		
		buf[index] = key;
		
		if(index < 8)  //µ¼¹âÆÁÊÇ8¸ö×Ö½ÚµÄÃüÁî
		{
			index ++;
			if(index == 8)
			{
				printf("index = 8\r\n");
				if(buf[1] == 0x5a && buf[2] == 0x70)   //0x70 ÊÇÉý¼¶ÃüÁî
				{	
					set_download_uart(1); //Í¨Ñ¶´®¿Ú
					if((buf[3] == 0) && (buf[7] == 0x70))  //ÊÇÉý¼¶ÃüÁî
					{
						checksum = (uint8_t)(0x5a+0xa5);
						SerialPutChar_uart0(0x5a);					
						SerialPutChar_uart0(0xa5);						
						//1. ÉÏ´«md5Âë
						for (i = 0; (i < FILE_MD5_LENGTH);i++)
						{
							SerialPutChar_uart0(pflash_md5[i]);
							
							checksum += pflash_md5[i];							
						}
						SerialPutChar_uart0(checksum);  //×Ü¹²35¸ö×Ö½Ú£¡£¡£¡£¡
					//	printf("buf[3] == 0) && (buf[7] == 0x70) checksum = %d\r\n",checksum);
					}
					else if((buf[3] == 1) && (buf[7] == 0x71))  //ÉÏÎ»»úÈ·ÈÏÐèÒªÏÂÔØ¸üÐÂ
					{
					//	printf("(buf[3] == 1) && (buf[3] == 0x71)\r\n");
						is_cpu_update_cmd = 1; //´Ó3399ÏÂÔØ
						//ÉèÖÃÏÂÔØ±ê¼Ç£¬ÖØÆô
						goto_ota_update();
					}
				}
				else if(buf[1] == 0x5a && buf[2] == 0x88)
				{
					if((buf[3] == 0) && (buf[7] == 0x88))  //ÊÇÉý¼¶ÃüÁî
					{
						char type = *(char*)(UPDATE_FLAG_START_ADDR-1);
						SerialPutChar_uart0(0xa5);					
						SerialPutChar_uart0(0x5a);
						SerialPutChar_uart0(0x89);
						SerialPutChar_uart0(type);  //
						SerialPutChar_uart0(0x0);
						SerialPutChar_uart0(0x0);
						SerialPutChar_uart0(0x0);
						SerialPutChar_uart0(0x89+type);
						
						printf("lcd type = %d\r\n",type);
					}
				}
				index = 0;
			}
		}
		else
			index = 0;
	}

}







/**
  * @brief  Display the Main Menu on to HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
//	uint8_t key = 0;

	/* Get the number of block (4 or 2 pages) from where the user program will be loaded */
	BlockNbr = (FlashDestination - 0x08000000) >> FLASH_ADDR_OFFSET;

	/* Compute the mask to test if the Flash memory, where the user program will be
	loaded, is write protected */
#if defined (GD32F10X_MD) || defined (GD32F10X_MD_VL)
	UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
#else /* USE_STM3210E_EVAL */
	if (BlockNbr < 62)
	{
		UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
	}
	else
	{
		UserMemoryMask = ((uint32_t)0x80000000);
	}
#endif /* (STM32F10X_MD) || (STM32F10X_MD_VL) */

	uart1_print_help();

	while (1)
	{
		get_cmd_from_debug_uart1();   //´Óµ÷ÊÔ´®¿ÚÅÐ¶ÏÊÇ·ñÐèÒª¸üÐÂ
		get_cmd_from_rk3399_uart0();  //´ÓÍ¨Ñ¶´®¿Ú£¨rk3399£©ÅÐ¶ÏÊÇ·ñ¸üÐÂ¡£
	}
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2010 STMicroelectronics *****END OF FILE******/
