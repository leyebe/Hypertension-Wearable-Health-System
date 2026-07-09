#include "DS18B20.h"
#include "delay.h"
void DS18B20_Reset(void)
{
	DS18B20_W_DQ(0);
	Delay_us(750);
	DS18B20_W_DQ(1);
	Delay_us(15);
}


uint8_t DS18B20_Check(void)
{
	uint16_t time = 0;
	DS18B20_W_DQ(1);
	while(DS18B20_R_DQ() && time < 200)
	{
		time++;
		Delay_us(1);
	}
	
	if (time >= 200)
		return 1;
	else
		time = 0;
	
	while(!DS18B20_R_DQ() && time < 240)
	{
		time++;
		Delay_us(1);
	}
	
	if (time >= 240)
		return 1;
	return 0;
}




uint8_t DS18B20_Read_Bit(void)
{
	DS18B20_W_DQ(0);
	Delay_us(1);
	DS18B20_W_DQ(1);
	Delay_us(15);
	if (DS18B20_R_DQ())
	{
		Delay_us(45);
		return 1;
	}
	else
	{
		Delay_us(45);
		return 0;
	}
}



uint8_t DS18B20_Read_Byte(void)   
{        
    uint8_t i,Bit,Byte = 0x00;
	for (i = 0;i < 8;i++) 
	{
        Bit = DS18B20_Read_Bit();
        Byte=(Bit << 7) | (Byte >> 1);		 /* 高位先行 */
    }						    
    return Byte;
}

void DS18B20_Write_Byte(uint8_t data)
{
	uint8_t i = 0;
	for (i = 0; i < 8; i++)
	{
		if (data & 0x01)
		{
			DS18B20_W_DQ(0);
			Delay_us(2);
			DS18B20_W_DQ(1);
			Delay_us(60);
		}
		else
		{
			DS18B20_W_DQ(0);
			Delay_us(60);
			DS18B20_W_DQ(1);
			Delay_us(2);
		}
		data = (data >> 1);
	}
}



void DS18B20_Start(void)
{
	DS18B20_Reset();
	DS18B20_Check();
	
	DS18B20_Write_Byte(0xcc);
	DS18B20_Write_Byte(0x44);
}

float DS18B20_GetTemp(void)
{
    uint16_t temp;
	uint8_t a,b;
	float value;
    DS18B20_Start();                    // ds1820 start convert
    DS18B20_Reset();
    DS18B20_Check();	 
    DS18B20_Write_Byte(0xcc);// skip rom
    DS18B20_Write_Byte(0xbe);// convert	    
    a=DS18B20_Read_Byte(); // LSB   
    b=DS18B20_Read_Byte(); // MSB   
	temp=b;
	temp=(temp<<8)+a;
    if((temp&0xf800)==0xf800)
	{
		temp=(~temp)+1;
		value=temp*(-0.0625);
	}
	else
	{
		value=temp*0.0625;	
	}
	return value;    
}




