#ifndef __GPS_H
#define __GPS_H
#include "stm32f1xx_hal.h"

void GPS_Init(void);
void GPS_Read(float *jd, float *wd);

#endif  /* __GPS_H */





