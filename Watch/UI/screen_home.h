#ifndef WATCH_UI_SCREEN_HOME_H
#define WATCH_UI_SCREEN_HOME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * 主界面的显示数据。
 *
 * 主界面只负责把这些数据画出来，
 * 不负责读取 RTC 或测量电池电压。
 */
typedef struct
{
  uint8_t hour;
  uint8_t minute;
  uint8_t month;
  uint8_t day;
  uint8_t battery_percent;
} screen_home_data_t;

/*
 * 把主界面绘制到 ui_canvas 缓冲区。
 * 本函数只负责绘制，不负责把数据发送到 OLED。
 */
void screen_home_draw(const screen_home_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_HOME_H */
