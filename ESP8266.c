#include "ESP8266.h"
#include "delay.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "UART.h"
#include "OLED.h"

extern UART_HandleTypeDef huart1;
extern uint8_t uart1_Buff[RXBUFF_MAX_SIZEOF];
extern uint16_t uart1_i;
extern uint8_t rxdata1;
extern uint16_t temp_max, heart_max, spo2_max;
extern uint8_t mode;

#define  ESP8266_BUF         uart1_Buff 
#define  ESP8266_CNT         uart1_i
#define  STM32_RX1BUFF_SIZE  RXBUFF_MAX_SIZEOF
uint16_t esp8266_cntPre;

void Clear_RxBuff(void)
{
  memset(ESP8266_BUF, 0, sizeof(ESP8266_BUF));
  ESP8266_CNT = 0;
}

void ESP8266_Init(void)
{
	HAL_UART_Receive_IT(&huart1, &rxdata1, 1);
  Delay_ms(500);
	while(ESP8266_SendCmd("AT\r\n","OK", 100) == 1);
	while(ESP8266_SendCmd("AT+CWMODE=2\r\n", "OK", 100) == 1);										/* 设置为AP模式 */
	ESP8266_SendCmd("AT+RST\r\n", "OK", 100);
	Delay_ms(500);
	while(ESP8266_SendCmd("AT+CWSAP=\"CCDX001\",\"12345678\",5,3\r\n", "OK", 100) == 1);		/* 建立服务器 */
	Delay_ms(500);

	while(ESP8266_SendCmd("AT+CIPMUX=1\r\n", "OK", 100) == 1);										/* 启动多连接 */
	Delay_ms(500);

	while(ESP8266_SendCmd("AT+CIPSERVER=1,8080\r\n", "OK", 100) == 1);										/* 建立服务器设置端口号 */
	Delay_ms(500);
}
_Bool ESP8266_WaitRecive(void)
{
	if(ESP8266_CNT == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(ESP8266_CNT == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		ESP8266_CNT = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = ESP8266_CNT;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志
}


void uart1_send(char *bufs,unsigned char len)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)bufs, len, 0xffff);
}


uint8_t ESP8266_SendCmd(char *cmd, char *res, uint16_t time)
{	
  uart1_send((char *)cmd,strlen((const char *)cmd));
	
	while(time--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							       //如果收到数据
		{
			if(strstr((const char *)ESP8266_BUF, res) != NULL)		//如果检索到关键词
			{
				Clear_RxBuff();									                //清空缓存
				return 0;
			}
		}
		
		Delay_ms(1);
	}
	
	return 1;

}


void ESP8266_SendData(char *data, unsigned short len)
{

	char cmdBuf[32];
	sprintf(cmdBuf, "AT+CIPSEND=0,%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">", 100))				//收到‘>’时可以发送数据
	{
			uart1_send(data , len);                   //发送设备连接请求数据
	}
}

int8_t Received_Data_Processing(const char *str, uint8_t len)						/* 处理接收的数据 */
{
	char temp[10];
	uint8_t temp_i = 0;
	char *p1 = NULL;
	if ((p1 = strstr((char *)ESP8266_BUF, str)) != NULL)													/* 通过关键字筛选有用的数据 */
	{
		p1 += strlen(str) + 1;
		while(*p1 != ',')
		{
			temp[temp_i++] = *p1;
			p1++;
		}
		return atoi(temp);
	}
	return RXERROR;
}


uint8_t ESP8266_GetIPD(void)
{
  int8_t value;
  uint8_t timeout = 100;
  while(timeout--)
  {
		if((ESP8266_WaitRecive() == REV_OK) && (strstr((char *)ESP8266_BUF, "IPD") != NULL))										//如果接收完成
		{
		OLED_Clear();
		value = Received_Data_Processing("mode", strlen("mode"));
		if (value != RXERROR)
			mode = value;
			if (mode == 2)
			{
				value = Received_Data_Processing("temp_max", strlen("temp_max"));
				if (value != RXERROR)
					temp_max = value;
				value = Received_Data_Processing("heart_max", strlen("heart_max"));
				if (value != RXERROR)
					heart_max = value;
				value = Received_Data_Processing("spo2_max", strlen("spo2_max"));
				if (value != RXERROR)
					spo2_max = value;
			}
      
		}
    Delay_ms(1);
  }
	return 0;														//超时还未找到，返回空指针
}




