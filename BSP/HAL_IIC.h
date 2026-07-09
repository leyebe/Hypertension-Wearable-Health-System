#ifndef __MYIIC_H
#define __MYIIC_H

#include "system.h"
#include "delay.h"
#include "main.h"

#define IIC_SDA_GPIO_Port GPIOB
#define IIC_SDA_Pin   GPIO_PIN_6
#define IIC_SCL_Pin   GPIO_PIN_5



//IO操作函数	 
#define IIC_SCL    PBout(5) //SCL
#define IIC_SDA    PBout(6) //输出SDA	 
#define READ_SDA   PBin(6)  //输入SDA

// IIC操作函数
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(unsigned char ack);
uint8_t IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Write_One_Byte(uint8_t daddr, uint8_t addr, uint8_t data);
void IIC_Read_One_Byte(uint8_t daddr, uint8_t addr, uint8_t *data);
void IIC_WriteBytes(uint8_t WriteAddr, uint8_t *data, uint8_t dataLength);
void IIC_ReadBytes(uint8_t deviceAddr, uint8_t writeAddr, uint8_t *data, uint8_t dataLength);

#endif
