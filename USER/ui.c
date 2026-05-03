#include "ui.h"
#include "tracking.h"
#include "lcd.h"
#include "touch.h"
#include "motor.h"
#include "debug.h"

#define BG_COLOR       BLACK
#define TITLE_COLOR    CYAN
#define BTN_ACTIVE     GREEN
#define BTN_INACTIVE   DARKGRAY
#define BTN_TEXT       WHITE
#define PARAM_LABEL    LIGHTGRAY
#define STATUS_COLOR   YELLOW

typedef struct { uint16_t x, y, w, h; } Rect;

static Page_t current_page = PAGE_MAIN;
static uint32_t touch_tick = 0;
static Touch_Data_t touch;

#define MAIN_BTN_W   300
#define MAIN_BTN_H   100
#define MAIN_BTN_GAP 40
static Rect main_btns[PAGE_NUM];
static const char *main_btn_names[] = { "", "循迹模式", "陀螺仪直线", "待开发  1", "待开发  2" };

#define BACK_BTN_X   20
#define BACK_BTN_Y   10
#define BACK_BTN_W   80
#define BACK_BTN_H   36
static Rect back_btn;

#define TRACK_BTN_Y      60
#define TRACK_BTN_H      70
#define TRACK_BTN_W      200
#define TRACK_BTN_GAP    40
#define TRACK_PARAM_Y    180
#define TRACK_PARAM_ROW  75
#define TRACK_PARAM_BTN  50
#define TRACK_PARAM_BH   36
#define TRACK_PARAM_VW   70
#define TRACK_PARAM_LX   180
#define TRACK_STATUS_Y   380
#define TRACK_PARAM_NUM  3

static Rect track_mode_btns[2];
static Rect track_minus[TRACK_PARAM_NUM];
static Rect track_plus[TRACK_PARAM_NUM];
static const char *track_mode_names[] = { "停止", "循迹" };
static const char *track_param_names[] = { "Base", "Slight", "Strong" };

#define GYRO_ROW1_Y   80
#define GYRO_ROW2_Y   150
#define GYRO_ROW3_Y   230
#define GYRO_STATUS_Y 310
#define GYRO_BTN_W    50
#define GYRO_BTN_H    36
#define GYRO_VAL_W    70

static Rect gyro_tgt_minus, gyro_tgt_plus;
static Rect gyro_spd_minus, gyro_spd_plus;
static Rect gyro_start_btn, gyro_stop_btn;
static int16_t gyro_speed = 200;
static float   gyro_target = 0.0f;

static uint8_t PtInRect(uint16_t x, uint16_t y, Rect *r)
{
    return (x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h) ? 1 : 0;
}

static void DrawBackBtn(void)
{
    back_btn.x = BACK_BTN_X; back_btn.y = BACK_BTN_Y;
    back_btn.w = BACK_BTN_W; back_btn.h = BACK_BTN_H;
    LCD_DrawButtonCN(back_btn.x, back_btn.y, back_btn.w, back_btn.h,
                     "返回", BTN_TEXT, DARKGRAY);
}

static void DrawMainPage(void)
{
    uint8_t i;
    uint16_t start_x, start_y;
    LCD_Clear(BG_COLOR);
    LCD_ShowMixedString(280, 20, "直线循迹小车 v2.0", TITLE_COLOR, BG_COLOR);
    LCD_ShowMixedString(340, 50, "选择模式", LIGHTGRAY, BG_COLOR);
    start_x = (800 - 2 * MAIN_BTN_W - MAIN_BTN_GAP) / 2;
    start_y = 100;
    for (i = 1; i < PAGE_NUM; i++) {
        uint8_t row = (i - 1) / 2;
        uint8_t col = (i - 1) % 2;
        main_btns[i].x = start_x + col * (MAIN_BTN_W + MAIN_BTN_GAP);
        main_btns[i].y = start_y + row * (MAIN_BTN_H + MAIN_BTN_GAP);
        main_btns[i].w = MAIN_BTN_W;
        main_btns[i].h = MAIN_BTN_H;
        LCD_DrawButtonCN(main_btns[i].x, main_btns[i].y,
                         main_btns[i].w, main_btns[i].h,
                         main_btn_names[i], BTN_TEXT, BTN_INACTIVE);
    }
}

static void HandleMainTouch(uint16_t tx, uint16_t ty)
{
    uint8_t i;
    for (i = 1; i < PAGE_NUM; i++) {
        if (PtInRect(tx, ty, &main_btns[i])) {
            Debug_Printf("Main: btn %d\r\n", i);
            UI_NavigateTo((Page_t)i);
            return;
        }
    }
    Debug_Printf("Main touch: x=%d y=%d\r\n", tx, ty);
}

static void DrawTrackPage(void)
{
    uint8_t i;
    int16_t param_vals[3];
    param_vals[0] = Tracking_GetBaseSpeed();
    param_vals[1] = Tracking_GetSlightSpeed();
    param_vals[2] = Tracking_GetStrongSpeed();
    LCD_Clear(BG_COLOR);
    DrawBackBtn();
    LCD_ShowMixedString(340, 20, "循迹模式", TITLE_COLOR, BG_COLOR);
    {
        uint16_t total_w = 2 * TRACK_BTN_W + TRACK_BTN_GAP;
        uint16_t sx = (800 - total_w) / 2;
        for (i = 0; i < 2; i++) {
            track_mode_btns[i].x = sx + i * (TRACK_BTN_W + TRACK_BTN_GAP);
            track_mode_btns[i].y = TRACK_BTN_Y;
            track_mode_btns[i].w = TRACK_BTN_W;
            track_mode_btns[i].h = TRACK_BTN_H;
            uint16_t c = (i == current_mode) ? BTN_ACTIVE : BTN_INACTIVE;
            LCD_DrawButtonCN(track_mode_btns[i].x, track_mode_btns[i].y,
                             track_mode_btns[i].w, track_mode_btns[i].h,
                             track_mode_names[i], BTN_TEXT, c);
        }
    }
    LCD_ShowMixedString(30, 145, "PARAMS:", PARAM_LABEL, BG_COLOR);
    for (i = 0; i < TRACK_PARAM_NUM; i++) {
        uint16_t ly = TRACK_PARAM_Y + i * TRACK_PARAM_ROW;
        track_minus[i].x = TRACK_PARAM_LX;
        track_minus[i].y = ly;
        track_minus[i].w = TRACK_PARAM_BTN;
        track_minus[i].h = TRACK_PARAM_BH;
        track_plus[i].x = TRACK_PARAM_LX + TRACK_PARAM_BTN + TRACK_PARAM_VW + 15;
        track_plus[i].y = ly;
        track_plus[i].w = TRACK_PARAM_BTN;
        track_plus[i].h = TRACK_PARAM_BH;
        LCD_ShowMixedString(30, ly + 10, track_param_names[i], PARAM_LABEL, BG_COLOR);
        LCD_DrawButtonCN(track_minus[i].x, track_minus[i].y,
                         track_minus[i].w, track_minus[i].h, "-", BTN_TEXT, DARKGRAY);
        LCD_ShowNum(track_minus[i].x + track_minus[i].w + 10, ly + 10,
                    param_vals[i], 4, WHITE, BG_COLOR, 16);
        LCD_DrawButtonCN(track_plus[i].x, track_plus[i].y,
                         track_plus[i].w, track_plus[i].h, "+", BTN_TEXT, DARKGRAY);
    }
    LCD_ShowMixedString(30, TRACK_STATUS_Y, "状态:", STATUS_COLOR, BG_COLOR);
}

static void HandleTrackTouch(uint16_t tx, uint16_t ty)
{
    uint8_t i;
    for (i = 0; i < 2; i++) {
        if (PtInRect(tx, ty, &track_mode_btns[i])) {
            uint8_t j;
            current_mode = i;
            Motor_Stop();
            for (j = 0; j < 2; j++) {
                uint16_t c = (j == current_mode) ? BTN_ACTIVE : BTN_INACTIVE;
                LCD_DrawButtonCN(track_mode_btns[j].x, track_mode_btns[j].y,
                                 track_mode_btns[j].w, track_mode_btns[j].h,
                                 track_mode_names[j], BTN_TEXT, c);
            }
            return;
        }
    }
    {
        typedef int16_t (*getter_t)(void);
        typedef void    (*setter_t)(int16_t);
        getter_t getters[] = { Tracking_GetBaseSpeed, Tracking_GetSlightSpeed, Tracking_GetStrongSpeed };
        setter_t setters[] = { Tracking_SetBaseSpeed, Tracking_SetSlightSpeed, Tracking_SetStrongSpeed };
        for (i = 0; i < TRACK_PARAM_NUM; i++) {
            if (PtInRect(tx, ty, &track_minus[i])) {
                int16_t val = getters[i]() - 10;
                if (val < 0) val = 0;
                setters[i](val);
                LCD_FillRect(track_minus[i].x + track_minus[i].w + 10,
                             track_minus[i].y + 10, TRACK_PARAM_VW, 16, BG_COLOR);
                LCD_ShowNum(track_minus[i].x + track_minus[i].w + 10,
                            track_minus[i].y + 10, getters[i](), 4, WHITE, BG_COLOR, 16);
                return;
            }
            if (PtInRect(tx, ty, &track_plus[i])) {
                int16_t val = getters[i]() + 10;
                if (val > 999) val = 999;
                setters[i](val);
                LCD_FillRect(track_minus[i].x + track_minus[i].w + 10,
                             track_minus[i].y + 10, TRACK_PARAM_VW, 16, BG_COLOR);
                LCD_ShowNum(track_minus[i].x + track_minus[i].w + 10,
                            track_minus[i].y + 10, getters[i](), 4, WHITE, BG_COLOR, 16);
                return;
            }
        }
    }
}

static void UpdateTrackStatus(void)
{
    float yaw = Tracking_GetYaw();
    uint8_t lost = Tracking_GetLost();
    LCD_FillRect(30, TRACK_STATUS_Y + 25, 740, 80, BG_COLOR);
    LCD_ShowMixedString(30, TRACK_STATUS_Y + 25, "模式:", LIGHTGRAY, BG_COLOR);
    LCD_ShowMixedString(80, TRACK_STATUS_Y + 25, track_mode_names[current_mode],
                        (current_mode == MODE_TRACK) ? GREEN : RED, BG_COLOR);
    LCD_ShowMixedString(180, TRACK_STATUS_Y + 25, "角度:", LIGHTGRAY, BG_COLOR);
    LCD_ShowNum(230, TRACK_STATUS_Y + 25, (int32_t)yaw, 6, WHITE, BG_COLOR, 16);
    LCD_ShowMixedString(500, TRACK_STATUS_Y + 25, "丢失:", LIGHTGRAY, BG_COLOR);
    LCD_ShowMixedString(560, TRACK_STATUS_Y + 25, lost ? "是" : "否",
                        lost ? RED : GREEN, BG_COLOR);
    {
        const uint16_t *raw = Tracking_GetGSRaw();
        uint8_t ch;
        for (ch = 0; ch < 8; ch++) {
            LCD_ShowNum(350 + ch * 50, TRACK_STATUS_Y + 50, raw[ch], 4, WHITE, BG_COLOR, 16);
        }
    }
    LCD_ShowMixedString(30, TRACK_STATUS_Y + 70, "Speed:", LIGHTGRAY, BG_COLOR);
    LCD_ShowNum(70, TRACK_STATUS_Y + 50, Tracking_GetBaseSpeed(), 4, WHITE, BG_COLOR, 16);
    LCD_ShowString(120, TRACK_STATUS_Y + 50, "/", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(135, TRACK_STATUS_Y + 50, Tracking_GetSlightSpeed(), 4, WHITE, BG_COLOR, 16);
    LCD_ShowString(185, TRACK_STATUS_Y + 50, "/", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowNum(200, TRACK_STATUS_Y + 50, Tracking_GetStrongSpeed(), 4, WHITE, BG_COLOR, 16);
}

static void DrawGyroPage(void)
{
    LCD_Clear(BG_COLOR);
    DrawBackBtn();
    LCD_ShowMixedString(320, 20, "陀螺仪直线", TITLE_COLOR, BG_COLOR);
    LCD_ShowMixedString(100, GYRO_ROW1_Y + 10, "Target:", PARAM_LABEL, BG_COLOR);
    gyro_tgt_minus.x = 260; gyro_tgt_minus.y = GYRO_ROW1_Y;
    gyro_tgt_minus.w = GYRO_BTN_W; gyro_tgt_minus.h = GYRO_BTN_H;
    gyro_tgt_plus.x  = 390; gyro_tgt_plus.y  = GYRO_ROW1_Y;
    gyro_tgt_plus.w  = GYRO_BTN_W; gyro_tgt_plus.h  = GYRO_BTN_H;
    LCD_DrawButtonCN(gyro_tgt_minus.x, gyro_tgt_minus.y, gyro_tgt_minus.w, gyro_tgt_minus.h,
                     "-", BTN_TEXT, DARKGRAY);
    LCD_ShowNum(320, GYRO_ROW1_Y + 10, (int32_t)gyro_target, 4, WHITE, BG_COLOR, 16);
    LCD_ShowString(370, GYRO_ROW1_Y + 10, "deg", LIGHTGRAY, BG_COLOR, 16);
    LCD_DrawButtonCN(gyro_tgt_plus.x, gyro_tgt_plus.y, gyro_tgt_plus.w, gyro_tgt_plus.h,
                     "+", BTN_TEXT, DARKGRAY);
    LCD_ShowMixedString(100, GYRO_ROW2_Y + 10, "Speed:", PARAM_LABEL, BG_COLOR);
    gyro_spd_minus.x = 260; gyro_spd_minus.y = GYRO_ROW2_Y;
    gyro_spd_minus.w = GYRO_BTN_W; gyro_spd_minus.h = GYRO_BTN_H;
    gyro_spd_plus.x  = 390; gyro_spd_plus.y  = GYRO_ROW2_Y;
    gyro_spd_plus.w  = GYRO_BTN_W; gyro_spd_plus.h  = GYRO_BTN_H;
    LCD_DrawButtonCN(gyro_spd_minus.x, gyro_spd_minus.y, gyro_spd_minus.w, gyro_spd_minus.h,
                     "-", BTN_TEXT, DARKGRAY);
    LCD_ShowNum(320, GYRO_ROW2_Y + 10, gyro_speed, 4, WHITE, BG_COLOR, 16);
    LCD_DrawButtonCN(gyro_spd_plus.x, gyro_spd_plus.y, gyro_spd_plus.w, gyro_spd_plus.h,
                     "+", BTN_TEXT, DARKGRAY);
    gyro_start_btn.x = 220; gyro_start_btn.y = GYRO_ROW3_Y;
    gyro_start_btn.w = 150; gyro_start_btn.h = 50;
    gyro_stop_btn.x  = 430; gyro_stop_btn.y  = GYRO_ROW3_Y;
    gyro_stop_btn.w  = 150; gyro_stop_btn.h  = 50;
    LCD_DrawButtonCN(gyro_start_btn.x, gyro_start_btn.y, gyro_start_btn.w, gyro_start_btn.h,
                     "运行中", BTN_TEXT, GREEN);
    LCD_DrawButtonCN(gyro_stop_btn.x, gyro_stop_btn.y, gyro_stop_btn.w, gyro_stop_btn.h,
                     "停止", BTN_TEXT, DARKGRAY);
    LCD_ShowMixedString(30, GYRO_STATUS_Y, "状态:", STATUS_COLOR, BG_COLOR);
}

static void HandleGyroTouch(uint16_t tx, uint16_t ty)
{
    if (PtInRect(tx, ty, &gyro_tgt_minus)) {
        gyro_target -= 5.0f;
        if (gyro_target < -180.0f) gyro_target = -180.0f;
        LCD_FillRect(320, GYRO_ROW1_Y + 10, GYRO_VAL_W, 16, BG_COLOR);
        LCD_ShowNum(320, GYRO_ROW1_Y + 10, (int32_t)gyro_target, 4, WHITE, BG_COLOR, 16);
        return;
    }
    if (PtInRect(tx, ty, &gyro_tgt_plus)) {
        gyro_target += 5.0f;
        if (gyro_target > 180.0f) gyro_target = 180.0f;
        LCD_FillRect(320, GYRO_ROW1_Y + 10, GYRO_VAL_W, 16, BG_COLOR);
        LCD_ShowNum(320, GYRO_ROW1_Y + 10, (int32_t)gyro_target, 4, WHITE, BG_COLOR, 16);
        return;
    }
    if (PtInRect(tx, ty, &gyro_spd_minus)) {
        gyro_speed -= 10;
        if (gyro_speed < 0) gyro_speed = 0;
        LCD_FillRect(320, GYRO_ROW2_Y + 10, GYRO_VAL_W, 16, BG_COLOR);
        LCD_ShowNum(320, GYRO_ROW2_Y + 10, gyro_speed, 4, WHITE, BG_COLOR, 16);
        return;
    }
    if (PtInRect(tx, ty, &gyro_spd_plus)) {
        gyro_speed += 10;
        if (gyro_speed > 999) gyro_speed = 999;
        LCD_FillRect(320, GYRO_ROW2_Y + 10, GYRO_VAL_W, 16, BG_COLOR);
        LCD_ShowNum(320, GYRO_ROW2_Y + 10, gyro_speed, 4, WHITE, BG_COLOR, 16);
        return;
    }
    if (PtInRect(tx, ty, &gyro_start_btn)) {
        Tracking_GyroStraight_Start(gyro_target, gyro_speed);
        return;
    }
    if (PtInRect(tx, ty, &gyro_stop_btn)) {
        Tracking_GyroStraight_Stop();
        return;
    }
    Debug_Printf("Gyro touch: x=%d y=%d\r\n", tx, ty);
}

static void UpdateGyroStatus(void)
{
    uint8_t running = Tracking_GyroStraight_IsRunning();
    float yaw = Tracking_GetYaw();
    int16_t out = Tracking_GyroStraight_GetOutput();
    int32_t yaw_x10 = (int32_t)(yaw * 10);
    LCD_FillRect(30, GYRO_STATUS_Y + 25, 740, 60, BG_COLOR);
    LCD_ShowMixedString(30, GYRO_STATUS_Y + 25, "状态:", LIGHTGRAY, BG_COLOR);
    LCD_ShowMixedString(80, GYRO_STATUS_Y + 25, running ? "运行中" : "已停止",
                        running ? GREEN : RED, BG_COLOR);
    LCD_ShowMixedString(230, GYRO_STATUS_Y + 25, "角度:", LIGHTGRAY, BG_COLOR);
    if (yaw_x10 < 0) {
        LCD_ShowChar(270, GYRO_STATUS_Y + 25, '-', WHITE, BG_COLOR, 16);
        yaw_x10 = -yaw_x10;
    }
    LCD_ShowNum(280, GYRO_STATUS_Y + 25, yaw_x10 / 10, 4, WHITE, BG_COLOR, 16);
    LCD_ShowChar(320, GYRO_STATUS_Y + 25, '.', WHITE, BG_COLOR, 16);
    LCD_ShowNum(330, GYRO_STATUS_Y + 25, yaw_x10 % 10, 1, WHITE, BG_COLOR, 16);
    LCD_ShowString(345, GYRO_STATUS_Y + 25, "deg", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowMixedString(430, GYRO_STATUS_Y + 25, "输出:", LIGHTGRAY, BG_COLOR);
    LCD_ShowNum(480, GYRO_STATUS_Y + 25, out, 5, WHITE, BG_COLOR, 16);
    LCD_ShowMixedString(30, GYRO_STATUS_Y + 50, "Target:", LIGHTGRAY, BG_COLOR);
    LCD_ShowNum(80, GYRO_STATUS_Y + 50, (int32_t)gyro_target, 4, WHITE, BG_COLOR, 16);
    LCD_ShowString(130, GYRO_STATUS_Y + 50, "deg", LIGHTGRAY, BG_COLOR, 16);
    LCD_ShowMixedString(220, GYRO_STATUS_Y + 50, "Speed:", LIGHTGRAY, BG_COLOR);
    LCD_ShowNum(310, GYRO_STATUS_Y + 50, gyro_speed, 4, WHITE, BG_COLOR, 16);
}

static void DrawBlankPage(uint8_t idx)
{
    LCD_Clear(BG_COLOR);
    DrawBackBtn();
    if (idx == 3) {
        LCD_ShowMixedString(340, 20, "待开发 1", TITLE_COLOR, BG_COLOR);
    } else {
        LCD_ShowMixedString(340, 20, "待开发 2", TITLE_COLOR, BG_COLOR);
    }
    LCD_ShowMixedString(320, 220, "敬请期待...", LIGHTGRAY, BG_COLOR);
}

void UI_Init(void)
{
    current_page = PAGE_MAIN;
    DrawMainPage();
}

void UI_NavigateTo(Page_t page)
{
    current_mode = MODE_STOP;
    Motor_Stop();
    current_page = page;
    switch (page) {
        case PAGE_MAIN:   DrawMainPage(); break;
        case PAGE_TRACK:  DrawTrackPage(); break;
        case PAGE_GYRO:   DrawGyroPage(); break;
        case PAGE_BLANK1: DrawBlankPage(3); break;
        case PAGE_BLANK2: DrawBlankPage(4); break;
        default: break;
    }
}

Page_t UI_GetPage(void)
{
    return current_page;
}

void UI_HandleTouch(uint32_t dt_us)
{
    uint16_t tx, ty;
    touch_tick += dt_us;
    if (touch_tick < 50000) return;
    touch_tick = 0;
    if (Touch_Scan(&touch) == 0) return;
    tx = touch.x[0];
    ty = touch.y[0];
    if (current_page != PAGE_MAIN) {
        if (PtInRect(tx, ty, &back_btn)) {
            UI_NavigateTo(PAGE_MAIN);
            return;
        }
    }
    switch (current_page) {
        case PAGE_MAIN:  HandleMainTouch(tx, ty); break;
        case PAGE_TRACK: HandleTrackTouch(tx, ty); break;
        case PAGE_GYRO:  HandleGyroTouch(tx, ty); break;
        default: break;
    }
}

void UI_UpdateStatus(void)
{
    switch (current_page) {
        case PAGE_TRACK: UpdateTrackStatus(); break;
        case PAGE_GYRO:  UpdateGyroStatus(); break;
        default: break;
    }
}
