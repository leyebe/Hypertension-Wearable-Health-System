#include "MPU6050.h"
#include "delay.h"
#include "OLED.h"
#define MAX(a,b) (a > b) ? a : b
#define MIN(a,b) (a > b) ? b : a
#define ACC_2_G(acc_val)  ((float)acc_val / 16384.0f)
#define ABS(a) (0 - (a)) > 0 ? (-(a)) : (a)   //取a的绝对值
#define SAMPLE_NUM                10          //采样10次取平均值
#define MIN_RELIABLE_VARIATION    500         //最小可信赖变化量
#define MAX_RELIABLE_VARIATION    5000        //最大可信赖变化量



#define ACTIVE_NUM          30            //最活跃轴更新周期
#define ACTIVE_NULL         0             //最活跃轴未知
#define ACTIVE_X            1             //最活跃轴是X
#define ACTIVE_Y            2             //最活跃轴是Y
#define ACTIVE_Z            3             //最活跃轴是Z
uint8_t most_active_axis = ACTIVE_NULL;   //记录最活跃轴

MPU6050_t old_ave_GyroValue;
typedef struct
{
	MPU6050_t max;
	MPU6050_t min;
}peak_value_t;
peak_value_t peak_value;

/* 写SCL操作函数*/
static void MPU6050_W_SCL(uint8_t value)
{
	HAL_GPIO_WritePin(MPU6050_GPIOx, MPU6050_SCL_Pin, (GPIO_PinState)value);
	Delay_us(10);
}
/* 写SDA操作函数 */
static void MPU6050_W_SDA(uint8_t value)
{
	HAL_GPIO_WritePin(MPU6050_GPIOx, MPU6050_SDA_Pin, (GPIO_PinState)value);
	Delay_us(10);
}
/* 读SDA操作函数 */
static uint8_t MPU6050_R_SDA(void)
{
	Delay_us(10);
	return HAL_GPIO_ReadPin(MPU6050_GPIOx, MPU6050_SDA_Pin);
}
/* I2C起始函数 */
static void I2C_Start(void)
{
	MPU6050_W_SDA(1);
	MPU6050_W_SCL(1);
	MPU6050_W_SDA(0);
	MPU6050_W_SCL(0);
}
/* I2C停止函数 */
static void I2C_Stop(void)
{
	MPU6050_W_SDA(0);
	MPU6050_W_SCL(1);
	MPU6050_W_SDA(1);
}
/* 发送一个字节 */
static void I2C_SendByte(uint8_t SendByte)
{
	for (int i = 0; i < 8; i++)
	{
		MPU6050_W_SDA(SendByte & (0x80 >> i));
		MPU6050_W_SCL(1);
		MPU6050_W_SCL(0);
	}
}

/* 接收一个字节 */
static uint8_t I2C_RecviceByte(void)
{
	uint8_t RecviceByte = 0x00;
	MPU6050_W_SDA(1);
	for (int i = 0; i < 8; i++)
	{
		MPU6050_W_SCL(1);
		if (MPU6050_R_SDA() == 1)
		{
			RecviceByte |= (0x80 >> i);
		}
		MPU6050_W_SCL(0);
	}
	return RecviceByte;
}

/* 发送ACK */
static void I2C_SendACK(uint8_t Ack)
{
	MPU6050_W_SDA(Ack);
	MPU6050_W_SCL(1);
	MPU6050_W_SCL(0);
}
/* 等待ACK */
static uint8_t I2C_WaitAck(void)
{
	uint8_t RecviceAck = 0;
	MPU6050_W_SDA(1);
	MPU6050_W_SCL(1);
	RecviceAck = MPU6050_R_SDA();
	MPU6050_W_SCL(0);
	return RecviceAck;
}
/* 读取一个寄存器的值 */
static uint8_t I2C_ReadReg(uint8_t Reg)
{
	uint8_t RecviceData;
	I2C_Start();
	I2C_SendByte(MPU6050_ADDRESS);
	I2C_WaitAck();
	I2C_SendByte(Reg);
	I2C_WaitAck();
	I2C_Start();
	I2C_SendByte(MPU6050_ADDRESS | 0x01);
	I2C_WaitAck();
	RecviceData = I2C_RecviceByte();
	I2C_SendACK(1);
	I2C_Stop();
	return RecviceData;
}
/* 写入一个寄存器 */
static void I2C_WriteReg(uint8_t Reg, uint8_t Data)
{
	I2C_Start();
	I2C_SendByte(MPU6050_ADDRESS);
	I2C_WaitAck();
	I2C_SendByte(Reg);
	I2C_WaitAck();
	I2C_SendByte(Data);
	I2C_WaitAck();
	I2C_Stop();
}
#if 0      /* 用到时在打开 */
/* 连续写多个寄存器 */
static void I2C_Write_Regs(uint8_t Reg, uint8_t *DataBuff, uint8_t Lenght)
{
	uint8_t i = 0;
	I2C_Start();
	I2C_SendByte(MPU6050_ADDRESS);
	I2C_WaitAck();
	I2C_SendByte(Reg);
	I2C_WaitAck();
	for (i = 0; i < Lenght; i++)
	{
		I2C_SendByte(DataBuff[i]);
		I2C_WaitAck();
	}
	I2C_Stop();
}
#endif
/* 连续读多个寄存器 */
static void I2C_Read_Regs(uint8_t Reg, uint8_t *DataBuff, uint8_t Lenght)
{
	uint8_t i = 0;
	I2C_Start();
	I2C_SendByte(MPU6050_ADDRESS);
	I2C_WaitAck();
	I2C_SendByte(Reg);
	I2C_WaitAck();
	I2C_Start();
	I2C_SendByte(MPU6050_ADDRESS | 0x01);
	I2C_WaitAck();
	for (i = 0; i < Lenght; i++)
	{
		DataBuff[i] = I2C_RecviceByte();
		if (i == Lenght - 1)
			I2C_SendACK(1);
		else
			I2C_SendACK(0);
	}
	
	I2C_Stop();
}
 

//设置MPU6050陀螺仪传感器满量程范围
//fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
//返回值:0,设置成功
//    其他,设置失败 
void MPU_Set_Gyro_Fsr(uint8_t fsr)
{
    I2C_WriteReg(MPU_GYRO_CFG_REG,fsr<<3);//设置陀螺仪满量程范围
}
 
//设置MPU6050加速度传感器满量程范围
//fsr:0,±2g;1,±4g;2,±8g;3,±16g
//返回值:0,设置成功
//    其他,设置失败 
void MPU_Set_Accel_Fsr(uint8_t fsr)
{
    I2C_WriteReg(MPU_ACCEL_CFG_REG,fsr<<3);//设置加速度传感器满量程范围
}
 
//设置MPU6050的数字低通滤波器
//lpf:数字低通滤波频率(Hz)
//返回值:0,设置成功
//    其他,设置失败 
void MPU_Set_LPF(uint16_t lpf)
{
    uint8_t data=0;
    if(lpf>=188) data=1;
    else if(lpf>=98) data=2;
    else if(lpf>=42) data=2;
    else if(lpf>=42) data=3;
    else if(lpf>=20) data=4;
    else if(lpf>=10) data=5;
    else data=6; 
    I2C_WriteReg(MPU_CFG_REG,data);//设置数字低通滤波器  
}
 
//设置MPU6050的采样率(假定Fs=1KHz)
//rate:4~1000(Hz)
//返回值:0,设置成功
//    其他,设置失败 
void MPU_Set_Rate(uint16_t rate)
{
    uint8_t data;
    if(rate>1000)rate=1000;
    if(rate<4)rate=4;
    data=1000/rate-1;
    I2C_WriteReg(MPU_SAMPLE_RATE_REG,data);  //设置数字低通滤波器
    MPU_Set_LPF(rate/2); //自动设置LPF为采样率的一半
}

uint8_t MPU6050_Read_ID()
{
	uint8_t ID;
	ID = I2C_ReadReg(MPU_DEVICE_ID_REG);
	return ID;
}

/* 初始化MPU6050 */
void MPU6050_Init()
{
	I2C_WriteReg(MPU_PWR_MGMT1_REG, 0x80);
	Delay_ms(100);
	I2C_WriteReg(MPU_PWR_MGMT1_REG, 0x00);
	MPU_Set_Gyro_Fsr(3);					
	MPU_Set_Accel_Fsr(0);					
	MPU_Set_Rate(200);			
	
	I2C_WriteReg(MPU_INT_EN_REG,0X00);
	I2C_WriteReg(MPU_USER_CTRL_REG,0X00);
	I2C_WriteReg(MPU_FIFO_EN_REG,0X00);
	I2C_WriteReg(MPU_INTBP_CFG_REG,0X80);
	if (MPU6050_Read_ID() == 0x70)
	{
		I2C_WriteReg(MPU_PWR_MGMT1_REG,0X01);
		I2C_WriteReg(MPU_PWR_MGMT2_REG,0X00);
		MPU_Set_Rate(200);
	}
	
}

/* 数据获取 */
static void MPU6050_Get_Value(MPU6050_t *data)
{
	uint8_t Buff[6];
	int sum[6] = {0};
	MPU6050_t change;

	if (MPU6050_Read_ID() == 0x70)
	{
		old_ave_GyroValue.gx = data->gx;
		old_ave_GyroValue.gy = data->gy;
		old_ave_GyroValue.gz = data->gz;
		for (uint8_t i = 0; i < SAMPLE_NUM; i++)
		{
			I2C_Read_Regs(MPU_GYRO_XOUTH_REG, Buff, 6);
			sum[0] += ((uint16_t)(Buff[0] << 8) | Buff[1]);
			sum[1] += ((uint16_t)(Buff[2] << 8) | Buff[3]);
			sum[2] += ((uint16_t)(Buff[4] << 8) | Buff[5]);
			I2C_Read_Regs(MPU_ACCEL_XOUTH_REG, Buff, 6);
			sum[3] += ((uint16_t)(Buff[0] << 8) | Buff[1]);
			sum[4] += ((uint16_t)(Buff[2] << 8) | Buff[3]);
			sum[5] += ((uint16_t)(Buff[4] << 8) | Buff[5]);
		}
		
		data->gx = sum[0] / SAMPLE_NUM;
		data->gy = sum[1] / SAMPLE_NUM;
		data->gz = sum[2] / SAMPLE_NUM;
		    
		data->ax = sum[3] / SAMPLE_NUM;
		data->ay = sum[4] / SAMPLE_NUM;
		data->az = sum[5] / SAMPLE_NUM;
		
		
		change.gx = ABS(data->gx - old_ave_GyroValue.gx);
		change.gy = ABS(data->gy - old_ave_GyroValue.gy);
		change.gz = ABS(data->gz - old_ave_GyroValue.gz);
		
		
		if(change.gx < MIN_RELIABLE_VARIATION || change.gx > MAX_RELIABLE_VARIATION)
		{
			data->gx = old_ave_GyroValue.gx;
		}
		if(change.gy < MIN_RELIABLE_VARIATION || change.gy > MAX_RELIABLE_VARIATION)
		{
			data->gy = old_ave_GyroValue.gy;
		}	
		if(change.gz < MIN_RELIABLE_VARIATION || change.gz > MAX_RELIABLE_VARIATION)
		{
			data->gz = old_ave_GyroValue.gz;
		}
		
		peak_value.max.gx = MAX(peak_value.max.gx , data->gx);
		peak_value.min.gx = MIN(peak_value.min.gx , data->gx);
		peak_value.max.gy = MAX(peak_value.max.gy , data->gy);
		peak_value.min.gy = MIN(peak_value.min.gy , data->gy);
		peak_value.max.gz = MAX(peak_value.max.gz , data->gz);
		peak_value.min.gz = MIN(peak_value.min.gz , data->gz);
	}
}

void MPU6050_is_Active(MPU6050_t *data)
{
	MPU6050_t change;
	static MPU6050_t active;        //三个轴的活跃度权重
	static uint8_t active_sample_num; 
	MPU6050_Get_Value(data);
	active_sample_num ++;
 
	//每隔一段时间，比较一次权重大小，判断最活跃轴
	if(active_sample_num >= ACTIVE_NUM)
	{
		if(active.gx > active.gy && active.gy > active.gz)
		{
			most_active_axis = ACTIVE_X;
		}
		else if(active.gy > active.gx && active.gy > active.gz)
		{
			most_active_axis = ACTIVE_Y;
		}
		else if(active.gz > active.gx && active.gz > active.gy)
		{
			most_active_axis = ACTIVE_Z;
		}
		else
		{
			most_active_axis = ACTIVE_NULL;
		}
		active_sample_num = 0;
		active.gx = 0;
		active.gy = 0;
		active.gz = 0;
	}
 
	//原始数据变化量
	change.gx = ABS(data->gx - old_ave_GyroValue.gx);
	change.gy = ABS(data->gy - old_ave_GyroValue.gy);
	change.gz = ABS(data->gz - old_ave_GyroValue.gz);
 
	//增加三轴活跃度权重
	if(change.gx > change.gy && change.gx > change.gz)
	{
		active.gx++;
	}
	else if(change.gy > change.gx && change.gy > change.gz)
	{
		active.gy++;
	}
	else if(change.gz > change.gx && change.gz > change.gy)
	{
		active.gz++;
	}
}

void MPU6050_Get_Step(MPU6050_t *data)
{
	int16_t mid;
	MPU6050_is_Active(data);
	static uint32_t step = 0, count = 0, x_state, y_state, z_state;
	switch(most_active_axis)
	{
		case ACTIVE_NULL:
			break;
		//捕捉原始数据骤增和骤减现象
		case ACTIVE_X:
			mid = (peak_value.max.gx + peak_value.min.gx) / 2;
			switch(x_state)
			{
				case 0:
					if(old_ave_GyroValue.gx < mid && data->gx > mid)
					{
						x_state++;
					}
					break;
				case 1:
					if(old_ave_GyroValue.gx > mid && data->gx < mid)
					{
						data->step_count++;
						x_state = 0;
					}
					break;
			}
			break;
		case ACTIVE_Y:
			mid = (peak_value.max.gy + peak_value.min.gy) / 2;
			switch(y_state)
			{
				case 0:
					if(old_ave_GyroValue.gy < mid && data->gy > mid)
					{
						y_state = 1;
					}
					break;
				case 1:
					if(old_ave_GyroValue.gy > mid && data->gy < mid)
					{
						data->step_count++;
						y_state = 0;
					}
					break;
			}
			break;	
		case ACTIVE_Z:
			mid = (peak_value.max.gz + peak_value.min.gz) / 2;
			if(((z_state == 0)) && (old_ave_GyroValue.gz < mid && data->gz > mid))
			{
				z_state = 1;
			}
			else if(old_ave_GyroValue.gz > mid && data->gz < mid)
			{
				if (z_state == 1)
				{
					data->step_count++;
					z_state = 0;
				}
			}
			break;
		default:
			break;
	}
	
	
	
//	if (count == 5)
//	{
//		count = 0;
//		if (step != 0)
//		{
//			step = 0;
//			data->step_count++;
//		}
//	}
//	else
//		count++;
}
void MPU6050_is_Tumble(MPU6050_t *data)
{
//	MPU6050_Get_Value(data);
	static uint8_t tumble_state = 0, tumble_time = 0;
	static int16_t old_gz_temp = 0;
	float ax_g = ACC_2_G(data->ax);
	float ay_g = ACC_2_G(data->ay);
	float az_g = ACC_2_G(data->az);
	switch(tumble_state)
	{
		case 0:
			if (az_g > 0.1f)
			{
				tumble_state 	= 1;
				data->tumble_state = 0;
			}
			break;
		case 1:
			if (az_g < 0)
			{
				tumble_state 	= 0;
				tumble_time 	= 0;
				data->tumble_state = 1;
			}
			break;
		case 2:
			if (tumble_time < 10)
				old_gz_temp = data->gz;
			else if (tumble_time >= 50)
			{
				data->tumble_state = 1;
				tumble_state = 0;
				tumble_time = 0;
			}
			else if (old_gz_temp != data->gz)
			{
				tumble_state = 0;
				tumble_time = 0;
				data->tumble_state = 0;
			}
			tumble_time++;
			break;
	}
//	
//	
//	OLED_ShowFloatNum(1, 32, ax_g, 1, 2, 8);
//	OLED_ShowFloatNum(50, 32, ay_g, 1, 2, 8);
//	OLED_ShowFloatNum(100, 32, az_g, 1, 2, 8);
}
