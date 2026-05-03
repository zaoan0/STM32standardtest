#include "mpu6050.h"
#include "delay.h"

/* ---------- 引脚定义（PB1-SCL, PB2-SDA，无JTAG冲突） ---------- */
#define SCL_H   GPIO_SetBits(GPIOB, GPIO_Pin_1)
#define SCL_L   GPIO_ResetBits(GPIOB, GPIO_Pin_1)
#define SDA_H   GPIO_SetBits(GPIOB, GPIO_Pin_2)
#define SDA_L   GPIO_ResetBits(GPIOB, GPIO_Pin_2)
#define SDA_RD  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_2)

#define MPU6050_ADDR  0xD0  // 0x68 << 1

/* ---------- SDA方向切换 ---------- */
static GPIO_InitTypeDef sda_gpio;

static void SDA_OUT(void)
{
    sda_gpio.GPIO_Pin   = GPIO_Pin_2;
    sda_gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    sda_gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &sda_gpio);
}

static void SDA_IN(void)
{
    sda_gpio.GPIO_Pin   = GPIO_Pin_2;
    sda_gpio.GPIO_Mode  = GPIO_Mode_IPU;
    sda_gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &sda_gpio);
}

/* ---------- I2C位级原语 ---------- */
static void I2C_Start(void)
{
    SDA_OUT();
    SDA_H; SCL_H; delay_us(3);
    SDA_L; delay_us(3);
    SCL_L; delay_us(3);
}

static void I2C_Stop(void)
{
    SDA_OUT();
    SDA_L; SCL_H; delay_us(3);
    SDA_H; delay_us(3);
}

static uint8_t I2C_WriteByte(uint8_t data)
{
    uint8_t i, ack;
    SDA_OUT();
    for (i = 0; i < 8; i++) {
        if (data & 0x80) SDA_H; else SDA_L;
        data <<= 1;
        delay_us(1);
        SCL_H; delay_us(3);
        SCL_L; delay_us(2);
    }
    SDA_IN();
    delay_us(1);
    SCL_H; delay_us(3);
    ack = (SDA_RD == 0) ? 1 : 0;  // SDA低 = ACK
    SCL_L; delay_us(2);
    return ack;
}

static uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t i, data = 0;
    SDA_IN();
    for (i = 0; i < 8; i++) {
        data <<= 1;
        delay_us(1);
        SCL_H; delay_us(3);
        if (SDA_RD) data |= 0x01;
        SCL_L; delay_us(2);
    }
    SDA_OUT();
    if (ack) SDA_L; else SDA_H;  // ACK：拉低，NACK：保持高
    delay_us(1);
    SCL_H; delay_us(3);
    SCL_L; delay_us(2);
    return data;
}

/* ---------- Register 访问层 ---------- */
static void I2C_WriteReg(uint8_t reg, uint8_t val)
{
    I2C_Start();
    I2C_WriteByte(MPU6050_ADDR);     // 写
    I2C_WriteByte(reg);
    I2C_WriteByte(val);
    I2C_Stop();
}

static uint8_t I2C_ReadReg(uint8_t reg)
{
    uint8_t val;
    I2C_Start();
    I2C_WriteByte(MPU6050_ADDR);     // 写
    I2C_WriteByte(reg);
    I2C_Stop();

    I2C_Start();
    I2C_WriteByte(MPU6050_ADDR | 1); // 读
    val = I2C_ReadByte(0);           // NACK
    I2C_Stop();
    return val;
}

static void I2C_ReadBytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    I2C_Start();
    I2C_WriteByte(MPU6050_ADDR);
    I2C_WriteByte(reg);
    I2C_Stop();

    I2C_Start();
    I2C_WriteByte(MPU6050_ADDR | 1);
    for (i = 0; i < len; i++) {
        buf[i] = I2C_ReadByte(i < (len - 1) ? 1 : 0);  // 除最后一个外都发ACK
    }
    I2C_Stop();
}

/* ---------- Bus 恢复（SCL翻转9次） ---------- */
static void I2C_Recovery(void)
{
    uint8_t i;
    SDA_IN();
    for (i = 0; i < 9; i++) {
        SCL_L; delay_us(3);
        SCL_H; delay_us(3);
    }
    I2C_Stop();
}

/* ========== 公共接口 ========== */

void MPU6050_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // SCL：Open-drain output（PB1）
    gpio.GPIO_Pin   = GPIO_Pin_1;
    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);

    // SDA：Open-drain output（PB2）
    gpio.GPIO_Pin   = GPIO_Pin_2;
    GPIO_Init(GPIOB, &gpio);

    SCL_H; SDA_H;

    // Bus 恢复，防止之前锁定
    I2C_Recovery();

    // MPU6050上电延时（100ms）
    delay_us(100000);

    // 唤醒（清除睡眠位）
    I2C_WriteReg(0x6B, 0x00);

    // Gyro：±500 dps（FS_SEL=1）
    I2C_WriteReg(0x1B, 0x08);

    // Accelerometer：±4g（AFS_SEL=2）
    I2C_WriteReg(0x1C, 0x10);

    // DLPF：44Hz带宽（滤除电机振动）
    I2C_WriteReg(0x1A, 0x03);
}

uint8_t MPU6050_Check(void)
{
    return (I2C_ReadReg(0x75) == 0x68) ? 1 : 0;
}

int16_t MPU6050_ReadGyroZ(void)
{
    uint8_t buf[2];
    I2C_ReadBytes(0x47, buf, 2);  // Gyro Z轴高/低字节
    return (int16_t)((buf[0] << 8) | buf[1]);
}
