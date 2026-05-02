# STM32 触摸屏循迹小车 (战舰V3)

基于 **STM32F103ZET6** (正点原子战舰V3) 的循迹小车，配备 4.3" TFT 触摸屏，支持实时调参、状态显示和陀螺仪辅助脱线恢复。

本项目从 [STM32standardtest](https://github.com/zaoan0/STM32standardtest) (F103C8 小板) 演进而来，针对战舰V3 的引脚布局和外设资源做了全面重构。

---

## 硬件配置

| 组件 | 说明 |
|------|------|
| MCU | STM32F103ZET6（战舰V3 核心板，144pin，512KB Flash） |
| LCD | 4.3" 800x480 TFT LCD（NT35510，FSMC 16-bit 接口） |
| 触摸 | GT9147 电容触摸（软件I2C: PB1-SCL, PF9-SDA） |
| 陀螺仪 | MPU6050（软件I2C: **PB4-SCL, PB5-SDA**） |
| 传感器 | 5路灰度传感器（PC4/5/6/7, PD2） |
| 电机驱动 | 双电机 PWM 控制（TIM3, PA6/PA7, 1kHz） |
| 编码器 | 增量式，TIM2 左轮(PA0/PA1)，TIM4 右轮(PB6/PB7)（预留） |
| 串口 | USART1（PA9-TX, PA10-RX）115200bps |

---

## 项目结构

```
stm32zhanjianstandard/
├── CORE/                        ARM CMSIS + 启动文件
│   ├── core_cm3.c/h
│   ├── startup_stm32f10x_hd.s
│   ├── stm32f10x.h
│   └── system_stm32f10x.c/h
├── FWLIB/                       STM32 标准外设库 V3.5.0
│   ├── inc/
│   └── src/
├── USER/
│   ├── main.c                   主循环（~75 行，仅调度）
│   ├── tracking.c/h             循迹控制模块（加权平均 + 脱线恢复）
│   ├── ui.c/h                   触摸屏 UI 模块（界面 + 触摸事件）
│   ├── motor.c/h                电机 PWM + 方向控制
│   ├── grayscale.c/h            灰度传感器（含 SR_BROKEN 配置）
│   ├── mpu6050.c/h              MPU6050 软件 I2C 驱动
│   ├── lcd.c/h                  NT35510 LCD 驱动（FSMC）
│   ├── touch.c/h                GT9147 电容触摸驱动
│   ├── pid.c/h                  PID 算法（预留）
│   ├── encoder.c/h              编码器读取（预留）
│   ├── debug.c/h                USART1 调试 printf
│   ├── delay.c/h                DWT 微秒 + SysTick 毫秒延时
│   └── stm32f10x_it.c/h         中断处理（默认空）
└── stm32zhanjianstandard.uvprojx
```

---

## 引脚分配

### 电机

| 引脚 | 功能 | 说明 |
|------|------|------|
| PA6 | 左轮 PWM | TIM3_CH1 |
| PA7 | 右轮 PWM | TIM3_CH2 |
| PE2 | 左轮 IN1 | 方向控制 |
| PE3 | 左轮 IN2 | 方向控制 |
| PE4 | 右轮 IN3 | 方向控制 |
| PC0 | 右轮 IN4 | 方向控制 |
| PC3 | 电机 STBY | 使能，常高 |

### 灰度传感器

| 引脚 | 传感器 | 位置 |
|------|--------|------|
| PC4 | OUT1 (SR) | 最右（已损坏，`SR_BROKEN` 宏禁用） |
| PC5 | OUT2 (RM) | 右中 |
| PC6 | OUT3 (M) | 中间 |
| PC7 | OUT4 (LM) | 左中 |
| PD2 | OUT5 (SL) | 最左 |

### 通信接口

| 接口 | 引脚 | 用途 |
|------|------|------|
| MPU6050 I2C | PB4 (SCL), PB5 (SDA) | 软件 I2C |
| GT9147 触摸 I2C | PB1 (SCL), PF9 (SDA) | 软件 I2C |
| USART1 | PA9 (TX), PA10 (RX) | 调试串口 |

### LCD (FSMC，底板直连)

| 引脚 | 功能 |
|------|------|
| PD0,1,4,5,8-10,14,15 | FSMC 数据/控制 |
| PE7-15 | FSMC 数据 |
| PG0 | FSMC A10 (RS) |
| PG12 | FSMC NE4 (片选) |
| PB0 | LCD 背光 |

---

## 软件功能

### 循迹算法（加权平均）

5 路传感器计算加权偏差，实现比例转向：

```
error = -2*SL - 1*LM + 0*M + 1*RM + 2*SR

pwm_left  = SPD_BASE + error * SPD_STRONG
pwm_right = SPD_BASE - error * SPD_STRONG
```

| error | 含义 | 效果 |
|-------|------|------|
| 0 | 线在正中 | 直行 |
| +/-1 | 线偏一侧 | 小幅转弯 |
| +/-2 | 线在远端 | 大幅转弯 |

多传感器同时触发时自动产生中间偏差，过渡平滑。

### 脱线恢复（陀螺仪辅助）

1. 脱线瞬间记录 yaw 角度
2. 根据线消失方向设定目标旋转角 (+/-30 度)
3. P 控制 (Kp=4.0) 旋转到目标角度
4. 找到线后恢复正常循迹

### 触摸屏 UI

- **模式切换**: STOP / TRACK 按钮
- **参数调节**: Base / Slight / Strong，+/- 步进 10
- **状态显示**: 模式、yaw、传感器值、脱线状态、速度，每 200ms 刷新

---

## 与旧版 (STM32standardtest) 对比

| 对比项 | 旧版 (F103C8) | 新版 (F103ZE 战舰V3) |
|--------|---------------|---------------------|
| MCU | STM32F103C8 (48pin, 64KB) | STM32F103ZE (144pin, 512KB) |
| 显示 | 无 | 4.3" 800x480 TFT + 电容触摸 |
| 交互方式 | 改代码重新烧录 | 触摸屏实时调参 |
| 循迹算法 | 离散 if-else 单传感器判断 | 加权平均，多传感器平滑过渡 |
| 灰度引脚 | PB3/4/5/12/13 (需禁用JTAG) | PC4-7 + PD2 (无冲突) |
| MPU6050 引脚 | PB8/PB9 | PB4/PB5 (释放 PB6/PB7 给编码器) |
| I2C 速度 | 每 bit 5us 延时 | 每 bit 3us 延时 (快约 40%) |
| 电机方向引脚 | PB0/1/10/11, STBY=PB14 | PE2/3/4 + PC0, STBY=PC3 |
| 代码架构 | 全部在 main.c (~140 行) | 模块化拆分 main/tracking/ui |
| main.c 行数 | ~140 行 | ~75 行 |
| 编码器/PID | 无 | 模块已写好，预留接口 |
| 传感器故障处理 | 无 | `SR_BROKEN` 宏配置 |
| 串口调试 | 无 | USART1 115200 Debug_Printf |

### 代码风格变化

**旧版** (单文件风格)：
```c
// 全部逻辑堆在 main.c
#define SPD_BASE 200        // 常量宏
// 离散判断
if (M == 1) { pwm_l = SPD_BASE; }
else if (LM == 1) { pwm_l = SPD_BASE - SPD_SLIGHT; }
```

**新版** (模块化风格)：
```c
// tracking.c 封装控制逻辑
int8_t error = -2*SL - 1*LM + 1*RM + 2*SR;  // 加权平均
pwm_l = SPD_BASE + error * SPD_STRONG;
// getter/setter 接口
void Tracking_SetBaseSpeed(int16_t v);
int16_t Tracking_GetBaseSpeed(void);
```

---

## 编译环境

- **IDE**: Keil uVision5
- **编译器**: ARMCC V5.06 update 5
- **C 标准**: C99
- **优化等级**: -O1
- **预定义宏**: `STM32F10X_HD, USE_STDPERIPH_DRIVER`
- **包含路径**: `.\USER; .\CORE; .\FWLIB\inc`

---

## 使用方法

1. 用 Keil 打开 `stm32zhanjianstandard.uvprojx`
2. 编译并烧录到战舰V3
3. 上电后触摸屏显示主界面
4. 点击 **TRACK** 开始循迹
5. 点击 **STOP** 停止
6. 通过 **[-]/[+]** 调节速度参数

---

## MPU6050 接线

| MPU6050 | STM32 | 备注 |
|---------|-------|------|
| VCC | 3.3V | 不可接 5V |
| GND | GND | 共地 |
| SCL | **PB4** | 需 4.7k 上拉到 3.3V |
| SDA | **PB5** | 需 4.7k 上拉到 3.3V |
| AD0 | GND | I2C 地址 0x68 |

> 注：MPU6050 引脚从 PB8/PB9 (旧版) 改为 PB4/PB5，释放 PB6/PB7 给 TIM4 编码器使用。

---

## 待完善

- [ ] 启用编码器 + PID 闭环速度控制
- [ ] 添加脱线恢复超时保护（自动停车）
- [ ] 陀螺仪互补滤波（抑制长时间漂移）
- [ ] 参数 Flash 持久化存储
- [ ] 串口数据上位机

---

*STM32 学习记录，代码仅供参考。*
