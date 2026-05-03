#include "touch.h"
#include "delay.h"
#include "debug.h"
#include <string.h>

/*
 * GT9147/GT917S 电容 Touch Controller 驱动
 * 正点原子战舰V3 4.3" TFT LCD v1.6
 * 引脚：PB1(SCL/TCLK), PF9(SDA/TDIN), PF10(PEN/INT), PF11(TCS/RST)
 *
 * 基于正点原子官方 gt9147.c + ctiic.c
 * GT9147 I2C地址：0x28（写），0x29（读）[7位：0x14]
 */

/* ---- I2C Pin 定义 ---- */
#define TOUCH_SCL_PORT   GPIOB
#define TOUCH_SCL_PIN    GPIO_Pin_1    /* PB1：TCLK */
#define TOUCH_SDA_PORT   GPIOF
#define TOUCH_SDA_PIN    GPIO_Pin_9    /* PF9：TDIN */
#define TOUCH_RST_PORT   GPIOF
#define TOUCH_RST_PIN    GPIO_Pin_11   /* PF11：TCS/RST */
#define TOUCH_INT_PORT   GPIOF
#define TOUCH_INT_PIN    GPIO_Pin_10   /* PF10：PEN/INT */

/* I2C位操作宏（直接 Register 操作，提高速度） */
#define SCL_H   GPIO_SetBits(TOUCH_SCL_PORT, TOUCH_SCL_PIN)
#define SCL_L   GPIO_ResetBits(TOUCH_SCL_PORT, TOUCH_SCL_PIN)
#define SDA_H   GPIO_SetBits(TOUCH_SDA_PORT, TOUCH_SDA_PIN)
#define SDA_L   GPIO_ResetBits(TOUCH_SDA_PORT, TOUCH_SDA_PIN)
#define SDA_RD  GPIO_ReadInputDataBit(TOUCH_SDA_PORT, TOUCH_SDA_PIN)

#define RST_H   GPIO_SetBits(TOUCH_RST_PORT, TOUCH_RST_PIN)
#define RST_L   GPIO_ResetBits(TOUCH_RST_PORT, TOUCH_RST_PIN)

/* GT9147 Register 地址 */
#define GT_CMD_WR      0x28
#define GT_CMD_RD      0x29
#define GT_CTRL_REG    0x8040
#define GT_CFGS_REG    0x8047
#define GT_CHECK_REG   0x80FF
#define GT_PID_REG     0x8140
#define GT_GSTID_REG   0x814E
#define GT_TP1_REG     0x8150
#define GT_TP2_REG     0x8158
#define GT_TP3_REG     0x8160
#define GT_TP4_REG     0x8168
#define GT_TP5_REG     0x8170

static const uint16_t GT_TP_REG[5] = {GT_TP1_REG, GT_TP2_REG, GT_TP3_REG, GT_TP4_REG, GT_TP5_REG};

/* ---- 软件I2C层 ---- */

static GPIO_InitTypeDef _sda_gpio;

static void SDA_OUT(void)
{
    _sda_gpio.GPIO_Pin   = TOUCH_SDA_PIN;
    _sda_gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    _sda_gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TOUCH_SDA_PORT, &_sda_gpio);
}

static void SDA_IN(void)
{
    _sda_gpio.GPIO_Pin   = TOUCH_SDA_PIN;
    _sda_gpio.GPIO_Mode  = GPIO_Mode_IPU;
    _sda_gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(TOUCH_SDA_PORT, &_sda_gpio);
}

static void CT_Delay(void)
{
    delay_us(5);
}

static void I2C_Start(void)
{
    SDA_OUT();
    SDA_H; SCL_H;
    delay_us(30);
    SDA_L;
    CT_Delay();
    SCL_L;
}

static void I2C_Stop(void)
{
    SDA_OUT();
    SCL_H;
    delay_us(30);
    SDA_L;
    CT_Delay();
    SDA_H;
}

static uint8_t I2C_WaitAck(void)
{
    uint8_t timeout = 0;
    SDA_IN();
    SDA_H;
    SCL_H;
    CT_Delay();
    while (SDA_RD) {
        timeout++;
        if (timeout > 250) {
            I2C_Stop();
            return 1;
        }
        CT_Delay();
    }
    SCL_L;
    return 0;
}

static void I2C_SendByte(uint8_t data)
{
    uint8_t i;
    SDA_OUT();
    SCL_L;
    CT_Delay();
    for (i = 0; i < 8; i++) {
        if (data & 0x80) SDA_H; else SDA_L;
        data <<= 1;
        SCL_H;
        CT_Delay();
        SCL_L;
        CT_Delay();
    }
}

static uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t i, data = 0;
    SDA_IN();
    delay_us(30);
    for (i = 0; i < 8; i++) {
        SCL_L;
        CT_Delay();
        SCL_H;
        data <<= 1;
        if (SDA_RD) data |= 0x01;
    }
    if (!ack) {
        /* NACK */
        SCL_L; SDA_OUT();
        CT_Delay();
        SDA_H;
        CT_Delay();
        SCL_H;
        CT_Delay();
        SCL_L;
    } else {
        /* ACK */
        SCL_L; SDA_OUT();
        CT_Delay();
        SDA_L;
        CT_Delay();
        SCL_H;
        CT_Delay();
        SCL_L;
    }
    return data;
}

/* ---- GT9147 Register 访问 ---- */

static uint8_t GT9147_WR_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i, ret = 0;
    I2C_Start();
    I2C_SendByte(GT_CMD_WR);
    I2C_WaitAck();
    I2C_SendByte(reg >> 8);
    I2C_WaitAck();
    I2C_SendByte(reg & 0xFF);
    I2C_WaitAck();
    for (i = 0; i < len; i++) {
        I2C_SendByte(buf[i]);
        ret = I2C_WaitAck();
        if (ret) break;
    }
    I2C_Stop();
    return ret;
}

static void GT9147_RD_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    I2C_Start();
    I2C_SendByte(GT_CMD_WR);
    I2C_WaitAck();
    I2C_SendByte(reg >> 8);
    I2C_WaitAck();
    I2C_SendByte(reg & 0xFF);
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte(GT_CMD_RD);
    I2C_WaitAck();
    for (i = 0; i < len; i++) {
        buf[i] = I2C_ReadByte(i == (len - 1) ? 0 : 1);
    }
    I2C_Stop();
}

/* ---- GT9147 Config ---- */

static const uint8_t GT9147_CFG[] = {
    0x60, 0xE0, 0x01, 0x20, 0x03, 0x05, 0x35, 0x00, 0x02, 0x08,
    0x1E, 0x08, 0x50, 0x3C, 0x0F, 0x05, 0x00, 0x00, 0xFF, 0x67,
    0x50, 0x00, 0x00, 0x18, 0x1A, 0x1E, 0x14, 0x89, 0x28, 0x0A,
    0x30, 0x2E, 0xBB, 0x0A, 0x03, 0x00, 0x00, 0x02, 0x33, 0x1D,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
    0x2A, 0x1C, 0x5A, 0x94, 0xC5, 0x02, 0x07, 0x00, 0x00, 0x00,
    0xB5, 0x1F, 0x00, 0x90, 0x28, 0x00, 0x77, 0x32, 0x00, 0x62,
    0x3F, 0x00, 0x52, 0x50, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,
    0x0F, 0x03, 0x06, 0x10, 0x42, 0xF8, 0x0F, 0x14, 0x00, 0x00,
    0x00, 0x00, 0x1A, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0E, 0x0C,
    0x0A, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x29, 0x28, 0x24, 0x22, 0x20, 0x1F, 0x1E, 0x1D,
    0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x05, 0x04, 0x02, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
};

static void GT9147_SendCfg(uint8_t mode)
{
    uint8_t buf[2];
    uint8_t i;
    uint16_t checksum = 0;

    for (i = 0; i < sizeof(GT9147_CFG); i++) {
        checksum += GT9147_CFG[i];
    }
    buf[0] = (uint8_t)((~checksum) + 1);
    buf[1] = mode;

    GT9147_WR_Reg(GT_CFGS_REG, (uint8_t *)GT9147_CFG, sizeof(GT9147_CFG));
    GT9147_WR_Reg(GT_CHECK_REG, buf, 2);
}

/* ---- 公共接口 ---- */

void Touch_Init(void)
{
    GPIO_InitTypeDef gpio;
    uint8_t id[5];

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOF, ENABLE);

    /* SCL（PB1）：Push-pull output */
    gpio.GPIO_Pin   = TOUCH_SCL_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TOUCH_SCL_PORT, &gpio);
    GPIO_SetBits(TOUCH_SCL_PORT, TOUCH_SCL_PIN);

    /* SDA（PF9）：Push-pull output */
    gpio.GPIO_Pin   = TOUCH_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TOUCH_SDA_PORT, &gpio);
    GPIO_SetBits(TOUCH_SDA_PORT, TOUCH_SDA_PIN);

    /* RST（PF11）：Push-pull output */
    gpio.GPIO_Pin   = TOUCH_RST_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TOUCH_RST_PORT, &gpio);
    GPIO_SetBits(TOUCH_RST_PORT, TOUCH_RST_PIN);

    /* INT（PF10）：先 Pull-up input，Reset 后 Pull-down */
    gpio.GPIO_Pin   = TOUCH_INT_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(TOUCH_INT_PORT, &gpio);
    GPIO_SetBits(TOUCH_INT_PORT, TOUCH_INT_PIN);

    /* Reset 时序 */
    RST_L;
    delay_us(10000);   /* 10ms */
    RST_H;
    delay_us(10000);   /* 10ms */

    /* INT Pin 设为 Pull-down input（官方时序） */
    gpio.GPIO_Pin   = TOUCH_INT_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_Init(TOUCH_INT_PORT, &gpio);
    GPIO_ResetBits(TOUCH_INT_PORT, TOUCH_INT_PIN);

    delay_us(100000);  /* 100ms */

    /* 读取产品ID */
    GT9147_RD_Reg(GT_PID_REG, id, 4);
    id[4] = 0;
    Debug_Printf("CTP ID:%s\r\n", id);

    /* Software Reset */
    id[0] = 0x02;
    GT9147_WR_Reg(GT_CTRL_REG, id, 1);

    /* 检查配置版本，必要时更新 */
    GT9147_RD_Reg(GT_CFGS_REG, id, 1);
    if (id[0] < 0x60) {
        Debug_Printf("CFG ver:%d, updating\r\n", id[0]);
        GT9147_SendCfg(1);
    }

    delay_us(10000);
    id[0] = 0x00;
    GT9147_WR_Reg(GT_CTRL_REG, id, 1);  /* 结束复位 */
}

uint8_t Touch_Scan(Touch_Data_t *data)
{
    uint8_t buf[4];
    uint8_t status, num, i;
    uint8_t res = 0;

    data->num = 0;

    GT9147_RD_Reg(GT_GSTID_REG, &status, 1);

    if ((status & 0x80) && ((status & 0x0F) < 6)) {
        /* 清除 Status Register */
        uint8_t zero = 0;
        GT9147_WR_Reg(GT_GSTID_REG, &zero, 1);
    }

    num = status & 0x0F;
    if (num > 0 && num < 6) {
        for (i = 0; i < num && i < 5; i++) {
            GT9147_RD_Reg(GT_TP_REG[i], buf, 4);

            /* 横屏坐标变换（匹配官方 gt9147.c touchtype=1） */
            data->y[i] = ((uint16_t)buf[1] << 8) | buf[0];
            data->x[i] = 800 - (((uint16_t)buf[3] << 8) | buf[2]);
        }
        data->num = num;
        res = 1;

        /* 验证主触摸点 */
        if (data->x[0] > 800 || data->y[0] > 480) {
            if (num > 1) {
                data->x[0] = data->x[1];
                data->y[0] = data->y[1];
            } else {
                data->x[0] = 0xFFFF;
                data->y[0] = 0xFFFF;
                res = 0;
            }
        }

        if (res) {
            Debug_Printf("Touch: %d pts X=%d Y=%d\r\n", num, data->x[0], data->y[0]);
        }
    }

    return res;
}

uint8_t Touch_IsTouched(void)
{
    uint8_t status;
    GT9147_RD_Reg(GT_GSTID_REG, &status, 1);
    return ((status & 0x80) && ((status & 0x0F) > 0)) ? 1 : 0;
}
