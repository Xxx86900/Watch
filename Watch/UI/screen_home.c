#include "screen_home.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

#define SCREEN_HOME_TIME_X             20U
#define SCREEN_HOME_TIME_Y             28U
#define SCREEN_HOME_TIME_SCALE          3U
#define SCREEN_HOME_DATE_X              4U
#define SCREEN_HOME_DATE_Y              3U

#define SCREEN_HOME_SEPARATOR_Y        12U

#define SCREEN_HOME_BATTERY_X         108U
#define SCREEN_HOME_BATTERY_Y           2U
#define SCREEN_HOME_BATTERY_WIDTH      16U
#define SCREEN_HOME_BATTERY_HEIGHT      8U
#define SCREEN_HOME_BATTERY_FILL_X    110U
#define SCREEN_HOME_BATTERY_FILL_Y      4U
#define SCREEN_HOME_BATTERY_FILL_WIDTH 12U
#define SCREEN_HOME_BATTERY_FILL_HEIGHT 4U

static void screen_home_format_time(uint8_t hour,
                                    uint8_t minute,
                                    char time_text[6])
{
    /* 防止收到超出正常范围的数据 */
    hour %= 24U;
    minute %= 60U;

    time_text[0] = (char)('0' + (hour / 10U));
    time_text[1] = (char)('0' + (hour % 10U));
    time_text[2] = ':';
    time_text[3] = (char)('0' + (minute / 10U));
    time_text[4] = (char)('0' + (minute % 10U));
    time_text[5] = '\0';
}

static void screen_home_format_date(uint8_t month,uint8_t day,char date_text[6])
{
    if ((month < 1U) || (month > 12U) || (day < 1U) || (day > 31U))
    {
        date_text[0] = '-';
        date_text[1] = '-';
        date_text[2] = '-';
        date_text[3] = '-';
        date_text[4] = '-';
        date_text[5] = '\0';
        return;
    }

    date_text[0] = (char)('0' + (month / 10U));
    date_text[1] = (char)('0' + (month % 10U));
    date_text[2] = '-';
    date_text[3] = (char)('0' + (day / 10U));
    date_text[4] = (char)('0' + (day % 10U));
    date_text[5] = '\0';
}

static void screen_home_draw_battery(uint8_t battery_percent)
{
    uint8_t fill_width;

    if (battery_percent > 100U)
    {
        battery_percent = 100U;
    }

    /* 电池主体外框 */
    ui_canvas_draw_rectangle(SCREEN_HOME_BATTERY_X,
                             SCREEN_HOME_BATTERY_Y,
                             SCREEN_HOME_BATTERY_WIDTH,
                             SCREEN_HOME_BATTERY_HEIGHT,
                             UI_COLOR_WHITE);

    /* 电池右侧的小凸起 */
    ui_canvas_fill_rectangle(124U,
                             4U,
                             2U,
                             4U,
                             UI_COLOR_WHITE);

    fill_width =
        (uint8_t)(((uint16_t)battery_percent *
                   SCREEN_HOME_BATTERY_FILL_WIDTH) / 100U);

    if (fill_width > 0U)
    {
        ui_canvas_fill_rectangle(SCREEN_HOME_BATTERY_FILL_X,
                                 SCREEN_HOME_BATTERY_FILL_Y,
                                 fill_width,
                                 SCREEN_HOME_BATTERY_FILL_HEIGHT,
                                 UI_COLOR_WHITE);
    }
}

void screen_home_draw(const screen_home_data_t *data)
{
    char time_text[6];
    char date_text[6];

    if (data == NULL)
    {
        return;
    }

    /* 每次绘制主界面前，先清空上一帧 */
    ui_canvas_clear(UI_COLOR_BLACK);

    screen_home_format_time(data->hour,
                            data->minute,
                            time_text);
    screen_home_format_date(data->month,data->day,date_text);

    /* 顶部状态栏分隔线 */
    ui_canvas_draw_line(0U,
                        SCREEN_HOME_SEPARATOR_Y,
                        UI_CANVAS_WIDTH - 1U,
                        SCREEN_HOME_SEPARATOR_Y,
                        UI_COLOR_WHITE);

    screen_home_draw_battery(data->battery_percent);

    ui_canvas_draw_text(SCREEN_HOME_DATE_X,SCREEN_HOME_DATE_Y,
                        date_text,&ui_font_5x7,UI_COLOR_WHITE);

    ui_canvas_draw_text_scaled(SCREEN_HOME_TIME_X,SCREEN_HOME_TIME_Y,
                               time_text,&ui_font_5x7,
                               SCREEN_HOME_TIME_SCALE,UI_COLOR_WHITE);
}
