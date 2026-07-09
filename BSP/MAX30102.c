#include "max30102.h"
#include "HAL_IIC.h"
#include "Delay.h"
#include "algorithm.h"
#include "stdio.h"
#include "string.h"
#include "OLED.h"
#define MAX_BRIGHTNESS 255
#define BUFFER_LENTH 500

int humi;
const char *sub_topic[] = {"/ptxz/sub"};
const char pub_topic[] = "/ptxz/pub";
uint32_t aun_ir_buffer[BUFFER_LENTH]; 		// IR LED sensor data   红外数据，用于计算血氿
uint32_t aun_red_buffer[BUFFER_LENTH]; 	// Red LED sensor data  红光数据，用于计算心率曲线以及计算心玿
int32_t n_sp02; 							// SPO2 value
int8_t ch_spo2_valid; 					// indicator to show if the SP02 calculation is valid
int32_t n_heart_rate; 					// heart rate value
int8_t ch_hr_valid; 						// indicator to show if the heart rate calculation is valid
uint8_t heart_rate_buff[16] = {0}, spo2_buff[16] = {0};
uint32_t un_min = 0x3FFFF, un_max = 0, un_prev_data;
int32_t n_brightness = 0;
float    f_temp;
uint8_t temp[6];

uint8_t max30102_Bus_Write(uint8_t Register_Address, uint8_t Word_Data)
{
    /* 采用串行EEPROM随即读取指令序列，连续读取若干字节 */

    /* 第1步：发起I2C总线启动信号 */
    IIC_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

    /* 第3步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址 */
    IIC_Send_Byte(Register_Address);
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第5步：开始写入数据 */
    IIC_Send_Byte(Word_Data);

    /* 第6步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 发送I2C总线停止信号 */
    IIC_Stop();
    return 1; /* 执行成功 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    IIC_Stop();
    return 0;
}

uint8_t max30102_Bus_Read(uint8_t Register_Address)
{
    uint8_t data;

    /* 第1步：发起I2C总线启动信号 */
    IIC_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

    /* 第3步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    IIC_Send_Byte((uint8_t)Register_Address);
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    IIC_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_RD); /* 此处是读指令 */

    /* 第8步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第9步：读取数据 */
    {
        data = IIC_Read_Byte(0); /* 读1个字节 */

        IIC_NAck(); /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    }
    /* 发送I2C总线停止信号 */
    IIC_Stop();
    return data; /* 执行成功 返回data值 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    IIC_Stop();
    return 0;
}

void max30102_FIFO_ReadWords(uint8_t Register_Address, uint16_t Word_Data[][2], uint8_t count)
{
    uint8_t i = 0;
    uint8_t no = count;
    uint8_t data1, data2;
    /* 第1步：发起I2C总线启动信号 */
    IIC_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

    /* 第3步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    IIC_Send_Byte((uint8_t)Register_Address);
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    IIC_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_RD); /* 此处是读指令 */

    /* 第8步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第9步：读取数据 */
    while(no)
    {
        data1 = IIC_Read_Byte(0);
        IIC_Ack();
        data2 = IIC_Read_Byte(0);
        IIC_Ack();
        Word_Data[i][0] = (((uint16_t)data1 << 8) | data2); //

        data1 = IIC_Read_Byte(0);
        IIC_Ack();
        data2 = IIC_Read_Byte(0);
        if(1 == no)
            IIC_NAck(); /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
        else
            IIC_Ack();
        Word_Data[i][1] = (((uint16_t)data1 << 8) | data2);

        no--;
        i++;
    }
    /* 发送I2C总线停止信号 */
    IIC_Stop();

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    IIC_Stop();
}

void max30102_FIFO_ReadBytes(uint8_t Register_Address, uint8_t *Data)
{
    max30102_Bus_Read(REG_INTR_STATUS_1);
    max30102_Bus_Read(REG_INTR_STATUS_2);

    /* 第1步：发起I2C总线启动信号 */
    IIC_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_WR); /* 此处是写指令 */

    /* 第3步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    IIC_Send_Byte((uint8_t)Register_Address);
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    IIC_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    IIC_Send_Byte(max30102_WR_address | I2C_RD); /* 此处是读指令 */

    /* 第8步：发送ACK */
    if(IIC_Wait_Ack() != 0)
    {
        goto cmd_fail; /* EEPROM器件无应答 */
    }

    /* 第9步：读取数据 */
    Data[0] = IIC_Read_Byte(1);
    Data[1] = IIC_Read_Byte(1);
    Data[2] = IIC_Read_Byte(1);
    Data[3] = IIC_Read_Byte(1);
    Data[4] = IIC_Read_Byte(1);
    Data[5] = IIC_Read_Byte(0);
    /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    /* 发送I2C总线停止信号 */
    IIC_Stop();

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    IIC_Stop();
}

void max30102_init(void)
{
	int i;
    IIC_Init();
    max30102_reset();
    max30102_Bus_Write(REG_INTR_ENABLE_1, 0xc0); // INTR setting
    max30102_Bus_Write(REG_INTR_ENABLE_2, 0x00);
    max30102_Bus_Write(REG_FIFO_WR_PTR, 0x00); // FIFO_WR_PTR[4:0]
    max30102_Bus_Write(REG_OVF_COUNTER, 0x00); // OVF_COUNTER[4:0]
    max30102_Bus_Write(REG_FIFO_RD_PTR, 0x00); // FIFO_RD_PTR[4:0]
    max30102_Bus_Write(REG_FIFO_CONFIG, 0x0f); // sample avg = 1, fifo rollover=false, fifo almost full = 17
    max30102_Bus_Write(REG_MODE_CONFIG, 0x03); // 0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
    max30102_Bus_Write(REG_SPO2_CONFIG, 0x27); // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS)
    max30102_Bus_Write(REG_LED1_PA, 0x24); // Choose value for ~ 7mA for LED1
    max30102_Bus_Write(REG_LED2_PA, 0x24); // Choose value for ~ 7mA for LED2
    max30102_Bus_Write(REG_PILOT_PA, 0x7f); // Choose value for ~ 25mA for Pilot LED
	
	
	
	for(i = 0; i < BUFFER_LENTH; i++)
    {
        while(MAX30102_INT == 1) // 等待中断引脚
            ;

        max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp); // 读取传感器数据，赋忼到temp丿
        aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2]; // 将忼合并得到实际数孿
        aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5]; // 将忼合并得到实际数孿

        if(un_min > aun_red_buffer[i])
            un_min = aun_red_buffer[i]; // 更新信号朿小忿
        if(un_max < aun_red_buffer[i])
            un_max = aun_red_buffer[i]; // 更新信号朿大忿
    }
    un_prev_data = aun_red_buffer[i];

    // 计算剿500个样本后的心率和SpO2（样本的剿5秒）
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_LENTH, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

}

void max30102_reset(void)
{
    max30102_Bus_Write(REG_MODE_CONFIG, 0x40);
    max30102_Bus_Write(REG_MODE_CONFIG, 0x40);
}

void maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
{
    IIC_Write_One_Byte(I2C_WRITE_ADDR, uch_addr, uch_data);
}

void maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
{
    IIC_Read_One_Byte(I2C_WRITE_ADDR, uch_addr, puch_data);
}

void maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)
{
    uint32_t un_temp;
    unsigned char uch_temp;
    char ach_i2c_data[6];
    *pun_red_led = 0;
    *pun_ir_led = 0;

    // read and clear status register
    maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_temp);
    maxim_max30102_read_reg(REG_INTR_STATUS_2, &uch_temp);

    IIC_ReadBytes(I2C_WRITE_ADDR, REG_FIFO_DATA, (uint8_t *)ach_i2c_data, 6);

    un_temp = (unsigned char)ach_i2c_data[0];
    un_temp <<= 16;
    *pun_red_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[1];
    un_temp <<= 8;
    *pun_red_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[2];
    *pun_red_led += un_temp;

    un_temp = (unsigned char)ach_i2c_data[3];
    un_temp <<= 16;
    *pun_ir_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[4];
    un_temp <<= 8;
    *pun_ir_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[5];
    *pun_ir_led += un_temp;
    *pun_red_led &= 0x03FFFF; // Mask MSB [23:18]
    *pun_ir_led &= 0x03FFFF; // Mask MSB [23:18]
}
uint8_t flag = 0;
uint32_t old_un_max, old_un_min;
void MAX30102_GetData(uint8_t *heart, uint8_t *spo2)
{
	
	int i;
// 读取和计算max30102数据，濻体用缓存的500组数据分析，实际每读叿100组新数据分析丿欿
        un_min = 0x3FFFF;
        un_max = 0;
        // 将前100组样本转储到内存中（实际没有），并将吿400组样本移到顶部，尿100-500缓存数据移位刿0-400
        for(i = 100; i < 500; i++)
        {
            aun_red_buffer[i - 100] = aun_red_buffer[i]; // 尿100-500缓存数据移位刿0-400
            aun_ir_buffer[i - 100] = aun_ir_buffer[i]; // 尿100-500缓存数据移位刿0-400
            // 更新信号的最小忼和朿大忿
            if(un_min > aun_red_buffer[i]) // 寻找移位吿0-400中的朿小忿
                un_min = aun_red_buffer[i];
            if(un_max < aun_red_buffer[i]) // 寻找移位吿0-400中的朿大忿
                un_max = aun_red_buffer[i];
        }

        // 在计算心率前叿100组样本，取的数据放在400-500缓存数组丿
				
        for(i = 400; i < 500; i++)
        {
            un_prev_data = aun_red_buffer[i - 1]; // 临时记录上一次读取数捿

//             // 等待中断引脚
                
						
					while(MAX30102_INT == 1);
            max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp); // 读取传感器数据，赋忼到temp丿
            aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2]; // 将忼合并得到实际数字，数组400-500为新读取数据
            aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5]; // 将忼合并得到实际数字，数组400-500为新读取数据

            if(aun_red_buffer[i] > un_prev_data)
            { // 用新获取的一个数值与上一个数值对毿
                f_temp = aun_red_buffer[i] - un_prev_data;
                f_temp /= (un_max - un_min);
                f_temp *= MAX_BRIGHTNESS; // 公式（心率曲线）=（新数忿-旧数值）/（最大忿-朿小忼）*255
                n_brightness -= (int)f_temp;
                if(n_brightness < 0)
                    n_brightness = 0;
            }
            else
            {
                f_temp = un_prev_data - aun_red_buffer[i];
                f_temp /= (un_max - un_min);
                f_temp *= MAX_BRIGHTNESS; // 公式（心率曲线）=（旧数忿-新数值）/（最大忿-朿小忼）*255
                n_brightness += (int)f_temp;
                if(n_brightness > MAX_BRIGHTNESS)
                    n_brightness = MAX_BRIGHTNESS;
            }
        }
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_LENTH, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); // 传入500个心率和衿氧数据计算传感器棿测结论，反馈心率和血氧测试结枿
        if((1 == ch_hr_valid) && (1 == ch_spo2_valid) && (n_heart_rate < 120) && (n_sp02 < 101))
        {
					if (n_heart_rate < 60)
						*heart = 60;
					else
						*heart = n_heart_rate;
					*spo2 = n_sp02 - 1;
        }
		else
		{
//			*heart = 0;
//			*spo2  = 0;
		}
}
