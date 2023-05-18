/**
  ******************************************************************************
  * @file    IAP/src/ymodem.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   This file provides all the software functions related to the ymodem 
  *          protocol.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/** @addtogroup IAP
  * @{
  */ 
  
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "gd32f10x_fmc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];
uint32_t FlashDestination = ApplicationAddress; /* Flash user program offset */
uint16_t PageSize = PAGE_SIZE;
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;
fmc_state_enum FLASHStatus = FMC_READY;
uint32_t RamSource;
extern uint8_t tab_1024[1024];
extern uint8_t md5sum_down[34];  //���md5ֵ
extern uint8_t is_cpu_update_cmd;   //��rk3399�������� ��0��ʾ��rk3399����
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size);

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive byte from sender
  * @param  c: Character
  * @param  timeout: Timeout
  * @retval 0: Byte received
  *         -1: Timeout
  */
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
    if (SerialKeyPressed(c) == 1)
    {
		return 0;
    }
  }
  return -1;
}

/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
static uint32_t Send_Byte (uint8_t c)
{
	SerialPutChar(c);
	return 0;
}

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    -1: abort by sender
  *    >0: packet length
  * @retval 0: normally return
  *        -1: timeout or packet error
  *         1: abort by user
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
	uint16_t i, packet_size;
	uint8_t c;
	*length = 0;
	uint16_t tempCRC,tempCRC1;
	
	if (Receive_Byte(&c, timeout) != 0)
	{
		printf("Receive_Byte timeout\r\n");
		return -1;
	}
//	printf("Receive_Packet c = %#x\r\n",c);
	switch (c)
	{
		case SOH:
			packet_size = PACKET_SIZE;
			break;
		case STX:
			packet_size = PACKET_1K_SIZE;
			break;
		case EOT:
			return 0;
		case CA:
			if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
			{
				*length = -1;
				return 0;
			}
			else
			{
				return -1;
			}
		case ABORT1:
		case ABORT2:
			printf("Receive_Packet c = %#x\r\n",c);
			return 1;
		case 0xa5:
		//	data[0] = 0xa5;
			for (i = 1; i < 8; i ++)
			{
			//	printf("for %d\r\n",packet_size + PACKET_OVERHEAD);
				if (Receive_Byte(data + i, timeout) != 0)
				{
					printf("for Receive_Byte error i = %d\r\n",i);
					return -1;
				}
			}
			if(data[1] == 0x5a && data[2] == 0x88)
			{
				if((data[3] == 0) && (data[7] == 0x88))  //����������
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
		default:
			return -1;
	}
	*data = c;
	for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
	{
	//	printf("for %d\r\n",packet_size + PACKET_OVERHEAD);
		if (Receive_Byte(data + i, timeout) != 0)
		{
			printf("for Receive_Byte error i = %d\r\n",i);
			return -1;
		}
	}
	if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
	{
		printf("error: data[PACKET_SEQNO_INDEX] = %#x\r\n",data[PACKET_SEQNO_INDEX]);
		return -1;
	}
	
	tempCRC = Cal_CRC16(&data[PACKET_HEADER], packet_size);
	tempCRC1 = data[packet_size + PACKET_HEADER]<<8 | data[packet_size + PACKET_HEADER+1];
	if(tempCRC != tempCRC1)
	{
		printf("checksum error checksum = %#x,data[%d] = %#x,%#x\r\n",tempCRC,packet_size + PACKET_HEADER,data[packet_size + PACKET_HEADER],data[packet_size + PACKET_HEADER+1]);
		return -1;
	}
	
	*length = packet_size;
	return 0;
}

/**
  * @brief  Receive a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_Receive (void)  //uint8_t *buf
{
	uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
	int32_t i, j, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
	uint8_t key;
	uint8_t *pdown_md5 = (void*)(UPDATE_FLAG_START_ADDR + DOWN_MD5_OFFET);
	uint8_t md5_same = 0;  //md5��ͬΪ0������Ϊ1
	uint32_t FlashDestination_const;	
	uint8_t time_out = 0;   //��һ����ʱ���������120�Σ���������ӣ��������˳�����ģʽ
	/* Initialize FlashDestination variable */
//	if(is_cpu_update_cmd) //rk3399���ص�λ�ò�ͬ����down����
//	{
		FlashDestination = ApplicationDownAddress;		
//	}
//	else{
//		FlashDestination = ApplicationAddress;		
//	}
	FlashDestination_const = FlashDestination;
//	printf("FlashDestination = %#x\r\n",FlashDestination);
	buf_ptr = packet_data + PACKET_HEADER;
	for (session_done = 0, errors = 0, session_begin = 0; ;)
	{
		for (packets_received = 0, file_done = 0; ;)//, buf_ptr = buf
		{
			switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
			{
				case 0:   //0: normally return
					errors = 0;
					switch (packet_length)
					{
					/* Abort by sender */
						case - 1:
							Send_Byte(ACK);
							return 0;
						/* End of transmission */
						case 0:
							Send_Byte(ACK);
							file_done = 1;
							break;
						/* Normal packet */
						default:
							if((packet_data[PACKET_SEQNO_INDEX]) < packets_received)  //�����ظ��İ���ymodem���ܻᷢ�Ͷ��
							{
								printf("---packet_data[PACKET_SEQNO_INDEX] = %#x packets_received = %d\r\n",packet_data[PACKET_SEQNO_INDEX],packets_received);
								continue;
							}
							if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
							{
								printf("packet_data[PACKET_SEQNO_INDEX] = %#x packets_received = %d\r\n",packet_data[PACKET_SEQNO_INDEX],packets_received);
								Send_Byte(NAK);
							}
							else
							{
								if (packets_received == 0)   //2023-04-12 ����
								{
									/* Filename packet */
									if (packet_data[PACKET_HEADER] != 0)
									{
										//j=0;
										/* Filename packet has valid data */
										for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
										{
											file_name[i++] = *file_ptr++;
										//	j++;
										}
										file_name[i++] = '\0';
									//	printf("file_name = %s,j=%d\r\n",file_name,j);
//										while(*file_ptr == 0)
//											file_ptr ++;
										//*file_ptr != '\0' ||
										for (i = 0,file_ptr ++; ( *file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
										{
											file_size[i++] = *file_ptr++;
										//	j++;
										}
										file_size[i++] = '\0';
									//	printf("file_size = %s,j=%d\r\n",file_size,j);
										Str2Int(file_size, &size);
										file_ptr ++;
										while(*file_ptr == 0)
											file_ptr ++;
										if(is_cpu_update_cmd)  //rk3399���ز���md5��
										{
											for (i = 0; (i < FILE_MD5_LENGTH);i++) //(*file_ptr != '\0') &&&& (*(pdown_md5+i) != '\0') 
											{
												md5sum_down[i] = *(file_ptr+i);
											//	printf("*(pdown_md5+i) = %#x,*(file_ptr+i)=%#x,i=%d\r\n",*(pdown_md5+i),*(file_ptr+i),i);
												if(*(pdown_md5+i) != *(file_ptr+i))
												{
													md5_same = 1;  //md5��ͬ����������
												//	break;
												}
											//	j++;
											}
											md5sum_down[i] = '\0';
											//printf("j=%d\r\n",j);
											if(!md5_same) //md5һ�£�������
											{										
												/* End session */
												Send_Byte(CA);
												Send_Byte(CA);
												printf("md5 is the same, md5 = %s i = %d\r\n",file_ptr,i);											
											//	printf("md5sum_down = %s\r\n",md5sum_down);
											//	printf("pdown_md5 addr = %p\r\n",pdown_md5);
											//	printf("pdown_md5 = %s\r\n",pdown_md5);
												return -4;
											}
											md5sum_down[i] = '\0';
										}
										/* Test the size of the image to be sent */
										/* Image size is greater than Flash size */
										if (size > (FLASH_IMAGE_SIZE - 1))  //FLASH_SIZE
										{
											printf("size(%d) > (FLASH_IMAGE_SIZE - 1)(%d)\r\n",size , (FLASH_IMAGE_SIZE - 1));
											/* End session */
											Send_Byte(CA);
											Send_Byte(CA);
											return -1;
										}

										/* Erase the needed pages where the user application will be loaded */
										/* Define the number of page to be erased */
										NbrOfPage = FLASH_PagesMask(size);

										/* Erase the FLASH pages */
										for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FMC_READY); EraseCounter++)
										{
										  FLASHStatus = fmc_page_erase(FlashDestination + (PageSize * EraseCounter));
										}
										Send_Byte(ACK);
										Send_Byte(CRC16);
									}
									/* Filename packet is empty, end session */
									else
									{
										Send_Byte(ACK);
										file_done = 1;
										session_done = 1;
										break;
									}
								}
								/* Data packet */
								else
								{
									//memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
									
									if(packets_received == 1)  //�������صĲ���bin�ļ�
									{
										if (((*(__IO uint32_t*)buf_ptr) & 0xfFFE0000 ) != 0x20000000)
										{
											Send_Byte(CA);
											Send_Byte(CA);
											printf("down file error 1,abort\r\n");
											/* End session */
											
											return -2;
										}
										else if(((*(__IO uint32_t*)(buf_ptr+4)) & 0xfFFff000 ) != ApplicationAddress)
										{
											Send_Byte(CA);
											Send_Byte(CA);
											printf("down file error 2,abort\r\n");
											/* End session */
											
											return -2;
										}
									}
									
									RamSource = (uint32_t)buf_ptr;
									for (j = 0;(j < packet_length) && (FlashDestination <  FlashDestination_const + size);j += 4)
									{
										/* Program the data received into STM32F10x Flash */
										fmc_word_program(FlashDestination, *(uint32_t*)RamSource);

										if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
										{
											/* End session */
											Send_Byte(CA);
											Send_Byte(CA);
											return -2;
										}
										FlashDestination += 4;
										RamSource += 4;
									}
									Send_Byte(ACK);
								}
								packets_received ++;
								session_begin = 1;
							}
						}   //end  switch (packet_length)
						break;
					case 1:    //1: abort by user
						printf("1: abort by user\r\n");
					
						Send_Byte(CA);
						Send_Byte(CA);
						return -3;
					default:    //-1: timeout or packet error
						printf("-1: timeout or packet error\r\n");
											
						if (session_begin > 0)
						{
							errors ++;
							if (errors > MAX_ERRORS)
							{
								printf("errors = %d\r\n",errors);
								Send_Byte(CA);
								Send_Byte(CA);
								return 0;
							}
						}
						else   //��û��ʼ���أ��Ǿ�����һ����ʱʱ��
						{
							time_out++;
							if(time_out >= 80)
							{
								printf("download timeout\r\n");
								return 0;
							}
						}
						//���Դ��ڿ�����ֹ
						if (SerialKeyPressed_Uart0((uint8_t*)&key))
						{
							if(key == 'a')  //(key == 'c' || key == 'C')  //ctrl + c
							{
								Send_Byte(CA);
								Send_Byte(CA);
								return -3;
							}
						}
						
						Send_Byte(CRC16);
					break;
			}  // end  switch (Receive_Packet
			if (file_done != 0)
			{
				printf("file_done break\r\n");
				break;
			}
		}    //end    for (packets_received = 0
		if (session_done != 0)
		{
			printf("session_done break\r\n");
			break;
		}
	} //end for (session_done = 0
	return (int32_t)size;
}

/**
  * @brief  check response using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
  * @brief  Prepare the first block
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  
  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
  * @brief  Prepare the data packet
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/**
  * @brief  Update CRC16 for input byte
  * @param  CRC input value 
  * @param  input byte
   * @retval None
  */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
	uint32_t crc = crcIn;
	uint32_t in = byte|0x100;
	do
	{
		crc <<= 1;
		in <<= 1;
		if(in&0x100)
			++crc;
		if(crc&0x10000)
			crc ^= 0x1021;
	}
	while(!(in&0x10000));
	return crc&0xffffu;
}


/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
	uint32_t crc = 0;
	const uint8_t* dataEnd = data+size;
	while(data<dataEnd)
		crc = UpdateCRC16(crc,*data++);

	crc = UpdateCRC16(crc,0);
	crc = UpdateCRC16(crc,0);
	
	return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
	uint32_t sum = 0;
	const uint8_t* dataEnd = data+size;
	
	while(data < dataEnd )
		sum += *data++;
	
	return sum&0xffu;
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
   * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
	uint16_t i;
	i = 0;
	while (i < length)
	{
		Send_Byte(data[i]);
		i++;
	}
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{  
	uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
	uint8_t FileName[FILE_NAME_LENGTH];
	uint8_t *buf_ptr, tempCheckSum ;
	uint16_t tempCRC, blkNumber;
	uint8_t receivedC[2], CRC16_F = 0, i;
	uint32_t errors, ackReceived, size = 0, pktSize;

	errors = 0;
	ackReceived = 0;
	for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
	{
		FileName[i] = sendFileName[i];
	}
	CRC16_F = 1;       

	/* Prepare first block */
	Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);

	do 
	{
		/* Send Packet */
		Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
		/* Send CRC or Check Sum based on CRC16_F */
		if (CRC16_F)
		{
			tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
			Send_Byte(tempCRC >> 8);
			Send_Byte(tempCRC & 0xFF);
		}
		else
		{
			tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
			Send_Byte(tempCheckSum);
		}

		/* Wait for Ack and 'C' */
		if (Receive_Byte(&receivedC[0], 10000) == 0)  
		{
			if (receivedC[0] == ACK)
			{ 
				/* Packet transfered correctly */
				ackReceived = 1;
			}
		}
		else
		{
			errors++;
		}
	}while (!ackReceived && (errors < 0x0A));
	//end do while
	
	
	if (errors >=  0x0A)
	{
	return errors;
	}
	buf_ptr = buf;
	size = sizeFile;
	blkNumber = 0x01;
	/* Here 1024 bytes package is used to send the packets */


	/* Resend packet if NAK  for a count of 10 else end of commuincation */
	while (size)
	{
	/* Prepare next packet */
	Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
	ackReceived = 0;
	receivedC[0]= 0;
	errors = 0;
	do
	{
	/* Send next packet */
	if (size >= PACKET_1K_SIZE)
	{
	pktSize = PACKET_1K_SIZE;

	}
	else
	{
	pktSize = PACKET_SIZE;
	}
	Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
	/* Send CRC or Check Sum based on CRC16_F */
	/* Send CRC or Check Sum based on CRC16_F */
	if (CRC16_F)
	{
	tempCRC = Cal_CRC16(&packet_data[3], pktSize);
	Send_Byte(tempCRC >> 8);
	Send_Byte(tempCRC & 0xFF);
	}
	else
	{
	tempCheckSum = CalChecksum (&packet_data[3], pktSize);
	Send_Byte(tempCheckSum);
	}

	/* Wait for Ack */
	if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
	{
	ackReceived = 1;  
	if (size > pktSize)
	{
	buf_ptr += pktSize;  
	size -= pktSize;
	if (blkNumber == (FLASH_IMAGE_SIZE/1024))
	{
	return 0xFF; /*  error */
	}
	else
	{
	blkNumber++;
	}
	}
	else
	{
	buf_ptr += pktSize;
	size = 0;
	}
	}
	else
	{
	errors++;
	}
	}while(!ackReceived && (errors < 0x0A));
	/* Resend packet if NAK  for a count of 10 else end of commuincation */

	if (errors >=  0x0A)
	{
	return errors;
	}

	}
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;
	do 
	{
	Send_Byte(EOT);
	/* Send (EOT); */
	/* Wait for Ack */
	if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
	{
	ackReceived = 1;  
	}
	else
	{
	errors++;
	}
	}while (!ackReceived && (errors < 0x0A));

	if (errors >=  0x0A)
	{
	return errors;
	}

	/* Last packet preparation */
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;

	packet_data[0] = SOH;
	packet_data[1] = 0;
	packet_data [2] = 0xFF;

	for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
	{
	packet_data [i] = 0x00;
	}

	do 
	{
	/* Send Packet */
	Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
	/* Send CRC or Check Sum based on CRC16_F */
	tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
	Send_Byte(tempCRC >> 8);
	Send_Byte(tempCRC & 0xFF);

	/* Wait for Ack and 'C' */
	if (Receive_Byte(&receivedC[0], 10000) == 0)  
	{
	if (receivedC[0] == ACK)
	{ 
	/* Packet transfered correctly */
	ackReceived = 1;
	}
	}
	else
	{
	errors++;
	}

	}while (!ackReceived && (errors < 0x0A));
	/* Resend packet if NAK  for a count of 10  else end of commuincation */
	if (errors >=  0x0A)
	{
	return errors;
	}  

	do 
	{
	Send_Byte(EOT);
	/* Send (EOT); */
	/* Wait for Ack */
	if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
	{
	ackReceived = 1;  
	}
	else
	{
	errors++;
	}
	}while (!ackReceived && (errors < 0x0A));

	if (errors >=  0x0A)
	{
	return errors;
	}
	return 0; /* file trasmitted successfully */
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/