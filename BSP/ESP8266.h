#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f1xx_hal.h"
#include "delay.h"
#include "OLED.h"

#define REV_WAIT 1
#define REV_OK 0
#define RXERROR -1
void ESP8266_Init(void);
unsigned char *ESP8266_GetIPD(unsigned short timeOut);
void ESP8266_Connect(void);
void ESP8266_SendString(char *s);
void ESP8266_SendData(char *str);

#endif
