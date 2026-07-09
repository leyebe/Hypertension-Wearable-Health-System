#ifndef __DS18B20_H
#define __DS18B20_H
#include "stm32f1xx_hal.h"

#define DS18B20_GPIO					GPIOB
#define DS18B20_GPIO_PIN			GPIO_PIN_7

#define DS18B20_W_DQ(x)			HAL_GPIO_WritePin(DS18B20_GPIO, DS18B20_GPIO_PIN, (GPIO_PinState)x)
#define DS18B20_R_DQ()				HAL_GPIO_ReadPin(DS18B20_GPIO, DS18B20_GPIO_PIN)



float DS18B20_GetTemp(void);
#endif

