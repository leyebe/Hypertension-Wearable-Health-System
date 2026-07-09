#ifndef __ESP8266_H
#define __ESP8266_H
#include "main.h"

struct Info
{
  uint8_t flag;
  char str[20];
};




#define RXERROR		-1


#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志


void ESP8266_Init(void);
void ESP8266_SendData(char *data, unsigned short len);
uint8_t ESP8266_SendCmd(char *cmd, char *res, uint16_t time);

uint8_t ESP8266_GetIPD(void);
#endif
