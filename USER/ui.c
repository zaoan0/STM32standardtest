#include "ui.h"
#include "tracking.h"
#include "lcd.h"
#include "touch.h"
#include "motor.h"

/* ---- 颜色方案 ---- */
#define BG_COLOR       BLACK
#define TITLE_COLOR    CYAN
#define BTN_ACTIVE     GREEN
#define BTN_INACTIVE   DARKGRAY
#define BTN_TEXT       WHITE
#define PARAM_LABEL    LIGHTGRAY
#define STATUS_COLOR   YELLOW

/* ---- UI 布局 (800x480 landscape) ---- */
#define TITLE_Y        15
#define BTN_Y          60
#define BTN_H          70
#define BTN_W          200
#define BTN_GAP        40
#define PARAM_LABEL_Y  145
#define PARAM_Y        180
#define PARAM_ROW_GAP  75
#define PARAM_BTN_W    50
#define PARAM_BTN_H    36
#define PARAM_VAL_W    70
#define PARAM_LX       180
#define STATUS_Y       380
#define PARAM_NUM      3

/* ---- 按钮区域 ---- */
typedef struct {
    uint16_t x, y, w, h;
} Rect;

static Rect mode_btns[2];
static Rect param_minus[PARAM_NUM];
static Rect param_plus[PARAM_NUM];

static const char *mode_names[] = { "STOP", "TRACK" };
static const char *param_names[] = { "Base  :", "Slight:", "Strong:" };

/* ---- 触摸状态 ---- */
static uint32_t touch_tick = 0;
static Touch_Data_t touch;

/* ---- 工具函数 ---- */
static uint8_t PtInRect(uint16_t x, uint16_t y, Rect *r)
{
    return (x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h) ? 1 : 0;
}

static void DrawUI(void)
{
    uint8_t i;
    uint16_t ly;
    int16_t param_vals[3];

    param_vals[0] = Tracking_GetBaseSpeed();
    param_vals[1] = Tracking_GetSlightSpeed();
    param_vals[2] = Tracking_GetStrongSpeed();

    LCD_Clear(BG_COLOR);

    /* 标题 */
    LCD_ShowString(250, TITLE_Y, "LINE TRACKING CAR v2.0", TITLE_COLOR, BG_COLOR, 16);

    /* 模式按钮 */
    for (i = 0; i < 2; i++) {
        uint16_t color = (i == current_mode) ? BTN_ACTIVE : BTN_INACTIVE;
        LCD_DrawButton(mode_btns[i].x, mode_btns[i].y,
                       mode_btns[i].w, mode_btns[i].h,
                       mode_names[i], BTN_TEXT, color);
    }

    /* 参数调节区域 */
    LCD_ShowString(30, PARAM_LABEL_Y, "PARAMETERS:", PARAM_LABEL, BG_COLOR, 16);

    for (i = 0; i < PARAM_NUM; i++) {
        ly = PARAM_Y + i * PARAM_ROW_GAP;

        LCD_ShowString(30, ly + 10, param_names[i], PARAM_LABEL, BG_COLOR, 16);

        LCD_DrawButton(param_minus[i].x, param_minus[i].y,
                       param_minus[i].w, param_minus[i].h,
                       "-", BTN_TEXT, DARKGRAY);

        LCD_ShowNum(param_minus[i].x + param_minus[i].w + 10, ly + 10,
                    param_vals[i], 4, WHITE, BG_COLOR, 16);

        LCD_DrawButton(param_plus[i].x, param_plus[i].y,
                       param_plus[i].w, param_plus[i].h,
                       "+", BTN_TEXT, DARKGRAY);
    }

    /* 状态区域 */
    LCD_ShowString(30, STATUS_Y, "STATUS:", STATUS_COLOR, BG_COLOR, 16);
}

/* ========== 公共接口 ========== */

void UI_Init(void)
{
    uint8_t i;
    uint16_t total_w, start_x;

    /* 模式按钮布局 (居中) */
    total_w = 2 * BTN_W + BTN_GAP;
    start_x = (800 - total_w) / 2;
    for (i = 0; i < 2; i++) {
        mode_btns[i].x = start_x + i * (BTN_W + BTN_GAP);
        mode_btns[i].y = BTN_Y;
        mode_btns[i].w = BTN_W;
        mode_btns[i].h = BTN_H;
    }

    /* 参数按钮布局 */
    for (i = 0; i < PARAM_NUM; i++) {
        uint16_t ly = PARAM_Y + i * PARAM_ROW_GAP;

        param_minus[i].x = PARAM_LX;
        param_minus[i].y = ly;
        param_minus[i].w = PARAM_BTN_W;
        param_minus[i].h = PARAM_BTN_H;

        param_plus[i].x = PARAM_LX + PARAM_BTN_W + PARAM_VAL_W + 15;
        param_plus[i].y = ly;
        param_plus[i].w = PARAM_BTN_W;
        param_plus[i].h = PARAM_BTN_H;
    }

    DrawUI();
}

void UI_HandleTouch(uint32_t dt_us)
{
    uint8_t i;
    uint16_t tx, ty;

    touch_tick += dt_us;
    if (touch_tick < 50000) return;
    touch_tick = 0;

    if (Touch_Scan(&touch) == 0) return;

    tx = touch.x[0];
    ty = touch.y[0];

    /* 检查模式按钮 */
    for (i = 0; i < 2; i++) {
        if (PtInRect(tx, ty, &mode_btns[i])) {
            uint8_t j;
            current_mode = i;
            Motor_Stop();

            for (j = 0; j < 2; j++) {
                uint16_t c = (j == current_mode) ? BTN_ACTIVE : BTN_INACTIVE;
                LCD_DrawButton(mode_btns[j].x, mode_btns[j].y,
                               mode_btns[j].w, mode_btns[j].h,
                               mode_names[j], BTN_TEXT, c);
            }
            return;
        }
    }

    /* 检查参数按钮 */
    {
        typedef int16_t (*getter_t)(void);
        typedef void    (*setter_t)(int16_t);
        getter_t getters[]  = { Tracking_GetBaseSpeed, Tracking_GetSlightSpeed, Tracking_GetStrongSpeed };
        setter_t setters[]  = { Tracking_SetBaseSpeed, Tracking_SetSlightSpeed, Tracking_SetStrongSpeed };

        for (i = 0; i < PARAM_NUM; i++) {
            if (PtInRect(tx, ty, &param_minus[i])) {
                int16_t val = getters[i]() - 10;
                if (val < 0) val = 0;
                setters[i](val);

                LCD_FillRect(param_minus[i].x + param_minus[i].w + 10,
                             param_minus[i].y + 10, PARAM_VAL_W, 16, BG_COLOR);
                LCD_ShowNum(param_minus[i].x + param_minus[i].w + 10,
                            param_minus[i].y + 10, getters[i](), 4, WHITE, BG_COLOR, 16);
                return;
            }
            if (PtInRect(tx, ty, &param_plus[i])) {
                int16_t val = getters[i]() + 10;
                if (val > 999) val = 999;
                setters[i](val);

                LCD_FillRect(param_minus[i].x + param_minus[i].w + 10,
                             param_minus[i].y + 10, PARAM_VAL_W, 16, BG_COLOR);
                LCD_ShowNum(param_minus[i].x + param_minus[i].w + 10,
                            param_minus[i].y + 10, getters[i](), 4, WHITE, BG_COLOR, 16);
                return;
            }
        }
    }
}

void UI_UpdateStatus(void)
{
    float yaw = Tracking_GetYaw();
    uint8_t gs = Tracking_GetGS();
    uint8_t lost = Tracking_GetLost();

    LCD_FillRect(30, STATUS_Y + 25, 740, 80, BG_COLOR);

    /* 模式 */
    LCD_ShowString(30, STATUS_Y + 25, "Mode:", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowString(80, STATUS_Y + 25, mode_names[current_mode],
                   (current_mode == MODE_TRACK) ? GREEN : RED, BG_COLOR, 16);

    /* Yaw */
    LCD_ShowString(200, STATUS_Y + 25, "Yaw:", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(240, STATUS_Y + 25, (int32_t)yaw, 6, WHITE, BG_COLOR, 16);

    /* 传感器 */
    LCD_ShowString(400, STATUS_Y + 25, "GS:", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(430, STATUS_Y + 25, gs, 3, WHITE, BG_COLOR, 16);

    /* 脱线状态 */
    LCD_ShowString(550, STATUS_Y + 25, "Lost:", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowString(600, STATUS_Y + 25, lost ? "YES" : "NO",
                   lost ? RED : GREEN, BG_COLOR, 16);

    /* 速度 */
    LCD_ShowString(30, STATUS_Y + 50, "SPD:", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(70, STATUS_Y + 50, Tracking_GetBaseSpeed(), 4, WHITE, BG_COLOR, 16);
    LCD_ShowString(120, STATUS_Y + 50, "/", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(135, STATUS_Y + 50, Tracking_GetSlightSpeed(), 4, WHITE, BG_COLOR, 16);
    LCD_ShowString(185, STATUS_Y + 50, "/", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(200, STATUS_Y + 50, Tracking_GetStrongSpeed(), 4, WHITE, BG_COLOR, 16);
}
