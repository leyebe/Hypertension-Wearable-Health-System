/*
如何使用：
1.根据所使用的控制器替换头文件 "stm32f3xx_hal.h"。我使用的是STM32F103VC。
2.重新分配与芯片连接的端口和引脚（DS1302_GPIO、DS1302_SCLK、DS1302_SDA、DS1302_RST）。
3.在主函数中执行 DS1302_Init();。
  现在就可以使用了。
 */

#ifndef _DS1302_H
#define _DS1302_H

#include "stm32f1xx_hal.h"
#include "delay.h"


//----------------------------------------------------------------------------------
//将下面列出的端口和引脚根据自己的需求进行重新分配
//----------------------------------------------------------------------------------
#define DS1302_CLK_Pin 	GPIO_PIN_5
#define DS1302_IO_Pin	GPIO_PIN_6
#define DS1302_RST_Pin	GPIO_PIN_7

#define DS1302_GPIO				GPIOA
#define DS1302_SCLK				DS1302_CLK_Pin
#define DS1302_SDA				DS1302_IO_Pin
#define DS1302_RST				DS1302_RST_Pin
//存放时间
typedef struct _time
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t date;
    uint8_t month;
    uint8_t week;
    uint8_t year;

} DS1302_Time_t;

/* 初始化 */
/* 配置 STM32 端口*/
void DS1302_Init(void);

/* Reads time byte by byte to 'buf' */
void DS1302_ReadTime(DS1302_Time_t* time);

/* Writes time byte by byte from 'buf' */
void DS1302_WriteTime(uint8_t *buf); 

/* Writes 'val' to ram address 'addr' */
/* Ram addresses range from 0 to 30 */
void DS1302_WriteRam(uint8_t addr, uint8_t val);

/* Чтение из RAM по адресу 'addr' */
uint8_t DS1302_ReadRam(uint8_t addr);

/* Очистка RAM памяти */
void DS1302_ClearRam(void);

/* Reads time in burst mode, includes control byte */
void DS1302_ReadTimeBurst(uint8_t * temp);

/* Writes time in burst mode, includes control byte */
void DS1302_WriteTimeBurst(uint8_t * buf);

/* Reads ram in burst mode 'len' bytes into 'buf' */
void DS1302_ReadRamBurst(uint8_t len, uint8_t * buf);

/* Writes ram in burst mode 'len' bytes from 'buf' */
void DS1302_WriteRamBurst(uint8_t len, uint8_t * buf);

//Запуск часов.
void DS1302_ClockStart(void);

//Остановка часов.
void DS1302_ClockStop(void);

//Сброс часов
void DS1302_ClockClear(void);

void DS1302_Write_Time(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t week);

#endif //_DS1302_H

