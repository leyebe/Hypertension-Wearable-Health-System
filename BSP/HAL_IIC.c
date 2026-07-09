#include "HAL_IIC.h"

// 初始化IIC
void IIC_Init(void)
{
    HAL_GPIO_WritePin(IIC_SDA_GPIO_Port,IIC_SCL_Pin|IIC_SDA_Pin,GPIO_PIN_SET);
}
/**
 * @brief SDA引脚设置输出模式
 * @param  无
 * @return 无
 */
static void Soft_IIC_Output(void)
{
    GPIO_InitTypeDef SOFT_IIC_GPIO_STRUCT;
    SOFT_IIC_GPIO_STRUCT.Mode = GPIO_MODE_OUTPUT_PP;
    SOFT_IIC_GPIO_STRUCT.Pin = IIC_SDA_Pin;
    SOFT_IIC_GPIO_STRUCT.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SDA_GPIO_Port, &SOFT_IIC_GPIO_STRUCT);
}
/**
 * @brief SDA引脚设置输入模式
 * @param  无
 * @return 无
 */
static void Soft_IIC_Input(void)
{
    GPIO_InitTypeDef SOFT_IIC_GPIO_STRUCT;
    SOFT_IIC_GPIO_STRUCT.Mode = GPIO_MODE_INPUT;
    SOFT_IIC_GPIO_STRUCT.Pin = IIC_SDA_Pin;
    SOFT_IIC_GPIO_STRUCT.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SDA_GPIO_Port, &SOFT_IIC_GPIO_STRUCT);
}

// 产生IIC起始信号
void IIC_Start(void)
{
    Soft_IIC_Output(); // sda线输出
    IIC_SDA = 1;//SDA高
    IIC_SCL = 1;
    Delay_us(4);
    IIC_SDA = 0; // START:when CLK is high,DATA change form high to low
    Delay_us(4);
    IIC_SCL = 0; // 钳住I2C总线，准备发送或接收数据
}


// 产生IIC停止信号
void IIC_Stop(void)
{
    Soft_IIC_Output(); // sda线输出
    IIC_SCL = 0;
    IIC_SDA = 0; // STOP:when CLK is high DATA change form low to high
    Delay_us(4);
    IIC_SCL = 1;
    IIC_SDA = 1; // 发送I2C总线结束信号
    Delay_us(4);
}

// 等待应答信号到来
// 返回值：1，接收应答失败
//         0，接收应答成功
uint8_t IIC_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;
    Soft_IIC_Input(); // SDA设置为输入
    IIC_SDA = 1;
    Delay_us(1);
    IIC_SCL = 1;
    Delay_us(1);
    while(READ_SDA)
    {
        ucErrTime++;
        if(ucErrTime > 250)
        {
            IIC_Stop();
            return 1;
        }
    }
    IIC_SCL = 0; // 时钟输出0
    return 0;
}

// 产生ACK应答
void IIC_Ack(void)
{
    IIC_SCL = 0;
    Soft_IIC_Output();
    IIC_SDA = 0;
    Delay_us(2);
    IIC_SCL = 1;
    Delay_us(2);
    IIC_SCL = 0;
}

// 不产生ACK应答
void IIC_NAck(void)
{
    IIC_SCL = 0;
    Soft_IIC_Output();
    IIC_SDA = 1;
    Delay_us(2);
    IIC_SCL = 1;
    Delay_us(2);
    IIC_SCL = 0;
}

// IIC发送一个字节
// 返回从机有无应答
// 1，有应答
// 0，无应答
void IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;
    Soft_IIC_Output();
    IIC_SCL = 0; // 拉低时钟开始数据传输
    for(t = 0; t < 8; t++)
    {
        IIC_SDA = (txd & 0x80) >> 7;
        txd <<= 1;
        Delay_us(2);
        IIC_SCL = 1;
        Delay_us(2);
        IIC_SCL = 0;
        Delay_us(2);
    }
}

// 诿1个字节，ack=1时，发送ACK，ack=0，发送nACK
uint8_t IIC_Read_Byte(unsigned char ack)
{
    unsigned char i, receive = 0;
    Soft_IIC_Input(); // SDA设置为输入
    for(i = 0; i < 8; i++)
    {
        IIC_SCL = 0;
        Delay_us(2);
        IIC_SCL = 1;
        receive <<= 1;
        if(READ_SDA)
            receive++;
        Delay_us(1);
    }
    if(!ack)
        IIC_NAck(); // 发送nACK
    else
        IIC_Ack(); // 发送ACK
    return receive;
}

// 写多个字节数据
void IIC_WriteBytes(uint8_t WriteAddr, uint8_t *data, uint8_t dataLength)
{
    uint8_t i;
    IIC_Start();

    IIC_Send_Byte(WriteAddr); // 发送写命令
    IIC_Wait_Ack();

    for(i = 0; i < dataLength; i++)
    {
        IIC_Send_Byte(data[i]);
        IIC_Wait_Ack();
    }
    IIC_Stop(); // 产生一个停止信号
    HAL_Delay(10);
}

// 读多个字节数据
void IIC_ReadBytes(uint8_t deviceAddr, uint8_t writeAddr, uint8_t *data, uint8_t dataLength)
{
    uint8_t i;
    IIC_Start();

    IIC_Send_Byte(deviceAddr); // 发送写命令
    IIC_Wait_Ack();
    IIC_Send_Byte(writeAddr);
    IIC_Wait_Ack();
    IIC_Send_Byte(deviceAddr | 0X01); // 进入接收模式
    IIC_Wait_Ack();

    for(i = 0; i < dataLength - 1; i++)
    {
        data[i] = IIC_Read_Byte(1);
    }
    data[dataLength - 1] = IIC_Read_Byte(0);
    IIC_Stop(); // 产生一个停止信号
    HAL_Delay(10);
}

// 读一个字节数据
void IIC_Read_One_Byte(uint8_t daddr, uint8_t addr, uint8_t *data)
{
    IIC_Start();

    IIC_Send_Byte(daddr); // 发送写命令
    IIC_Wait_Ack();
    IIC_Send_Byte(addr); // 发送地址
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte(daddr | 0X01); // 进入接收模式
    IIC_Wait_Ack();
    *data = IIC_Read_Byte(0);
    IIC_Stop(); // 产生一个停止信号
}

// 写一个字节数据
void IIC_Write_One_Byte(uint8_t daddr, uint8_t addr, uint8_t data)
{
    IIC_Start();

    IIC_Send_Byte(daddr); // 发送写命令
    IIC_Wait_Ack();
    IIC_Send_Byte(addr); // 发送地址
    IIC_Wait_Ack();
    IIC_Send_Byte(data); // 发送字节
    IIC_Wait_Ack();
    IIC_Stop(); // 产生一个停止信号
    HAL_Delay(10);
}
