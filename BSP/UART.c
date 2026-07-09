#include "UART.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;


uint8_t uart1_Buff[RXBUFF_MAX_SIZEOF];
uint16_t uart1_i;
uint8_t rxdata1;

uint8_t uart3_Buff[RXBUFF_MAX_SIZEOF];
uint16_t uart3_i;
uint8_t rxdata3;

uint8_t rx_flag, rx_state;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart1_Buff[uart1_i++] = rxdata1;
    HAL_UART_Receive_IT(&huart1, &rxdata1, 1);
    if (uart1_i >= RXBUFF_MAX_SIZEOF)
    {
      uart1_i = 0;
    }
  }
	else if (huart->Instance == USART3)
  {
    HAL_UART_Receive_IT(&huart3, &rxdata3, 1);
		switch(rx_flag)
		{
			case 0:
				if(rxdata3 == '$')
					rx_flag = 1;
				break;
			case 1:
				if(rxdata3 == 'G')
					rx_flag = 2;
				else
					rx_flag = 0;
				break;
			case 2:
				if(rxdata3 == 'P')
					rx_flag = 3;
				else
					rx_flag = 0;
				break;
			case 3:
				if(rxdata3 == 'R')
					rx_flag = 4;
				else
					rx_flag = 0;
				break;
			case 4:
				if(rxdata3 == 'M')
					rx_flag = 5;
				else
					rx_flag = 0;
				break;
				case 5:
				if (rxdata3 == '\n')
				{
					rx_flag = 0;
					rx_state = 1;
					uart3_i = 0;
				}
				uart3_Buff[uart3_i++] = rxdata3;
				break;
		}
  }
}


