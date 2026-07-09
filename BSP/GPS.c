#include "GPS.h"
#include "UART.h"
#include "OLED.h"
#include "Delay.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
extern UART_HandleTypeDef huart3;
extern uint8_t rxdata3;
extern uint8_t uart3_Buff[RXBUFF_MAX_SIZEOF];
extern uint16_t uart3_i;
extern uint8_t rx_state;

float longitude_value = 0;
float latitude_value = 0;
uint16_t gps_i;
void GPS_Init(void)
{
  HAL_UART_Receive_IT(&huart3, &rxdata3, 1);
}

void Uart3_Clear_Buff(void)
{
	memset(uart3_Buff, 0, sizeof(uart3_Buff));
}

uint8_t gps_time;
uint8_t GPS_Wait(void)
{
	if (gps_time < 2)
	{
		gps_time++;
		return 0;
	}
	gps_time = 0;
	if (uart3_i == 0)
		return 0;
	if (gps_i == uart3_i)
	{
		uart3_i = 0;
		return 1;
	}
	gps_i = uart3_i;
	return 0;
}


void GPS_Read(float *jd, float *wd)
{
  char *p = NULL;
  uint8_t dec;
	char gps_data[RXBUFF_MAX_SIZEOF];
  char buff1[100] = {0};
  char buff2[100] = {0};
  uint8_t i = 0;
	uint8_t time_out = 100;
	while(time_out-- )
	{
		if (rx_state == 1)
		{
			uart3_Buff[RXBUFF_MAX_SIZEOF - 1] = '\0';
			strcpy(gps_data, (char *)uart3_Buff);
			gps_data[RXBUFF_MAX_SIZEOF - 1] = '\0';
				p = strstr((char *)gps_data, "A");                      /* 取出纬度 */
				if (p == NULL)
					return ;
				p+=2;
				while(*p != ',')
				{
					buff1[i++] = *p;
					p++;        
				}
				p = strstr((char *)gps_data, "N");                     /* 取出经度 */
				if (p == NULL)
					return ;
				p += 2;
				i = 0;
				while(*p != ',')
				{
					if (i == 10)
						break;
					buff2[i++] = *p;
					p++;        
				}
				p = strstr(buff1, ".");
				p += 1;
				char temp_buff[5];
				temp_buff[0] = p[0];
				temp_buff[1] = p[1];
				temp_buff[2] = p[2];
				temp_buff[3] = p[3];
				temp_buff[4] = '\0';
				if (p != NULL)
					dec = atoi(temp_buff);
				latitude_value = ((buff1[0] - '0') * 10 + (buff1[1] - '0')) + ((buff1[2] - '0') * 10 + (buff1[3] - '0')) / 60.0 + (dec / 600000.0);
				if (latitude_value > 3 && latitude_value < 53)
					*wd = latitude_value;
				p = strstr(buff2, ".");
				p += 1;
				memset(temp_buff, 0, sizeof(temp_buff));
				temp_buff[0] = p[0];
				temp_buff[1] = p[1];
				temp_buff[2] = p[2];
				temp_buff[3] = p[3];
				temp_buff[4] = '\0';
				if (p != NULL)
					dec = atoi(temp_buff);
				longitude_value = ((buff2[0] - '0') * 100 + (buff2[1] - '0') * 10 + (buff2[2] - '0')) + ((buff2[3] - '0') * 10 + (buff2[4] - '0')) / 60.0 + (dec / 600000.0);
				if (longitude_value > 73 && longitude_value < 135)
					*jd = longitude_value;
				Uart3_Clear_Buff();
				rx_state = 0;
				return;
		}
		Delay_ms(1);
  }
}





















