#ifndef WATCH_UI_UI_CANVAS_H
#define WATCH_UI_UI_CANVAS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fonts.h"
/* OLED 屏幕分辨率 */
#define UI_CANVAS_WIDTH        128U
#define UI_CANVAS_HEIGHT        64U
#define UI_CANVAS_BUFFER_SIZE  (UI_CANVAS_WIDTH * UI_CANVAS_HEIGHT / 8U)

/* 单色屏幕的像素颜色 */
typedef enum
{
  UI_COLOR_BLACK = 0,
  UI_COLOR_WHITE
} ui_color_t;

/* 初始化画布 */
void ui_canvas_init(void);

/* 使用指定颜色清空整个画布 */
void ui_canvas_clear(ui_color_t color);

/* 绘制一个像素点 */
void ui_canvas_draw_pixel(uint8_t x,uint8_t y,ui_color_t color);
/*在两个坐标点之间绘制直线*/
void ui_canvas_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, ui_color_t color);
/* 绘制空心矩形，x、y 表示左上角坐标 */
void ui_canvas_draw_rectangle(uint8_t x,uint8_t y,uint8_t width,uint8_t height,ui_color_t color);
/* 绘制实心矩形 */
void ui_canvas_fill_rectangle(uint8_t x,uint8_t y,uint8_t width,uint8_t height,ui_color_t color);
void ui_canvas_draw_circle(int16_t center_x,int16_t center_y,uint8_t radius,ui_color_t color);
void ui_canvas_fill_circle(int16_t center_x,int16_t center_y,uint8_t radius,ui_color_t color);
/* 绘制单个字符，背景保持不变 */
void ui_canvas_draw_char(uint8_t x,uint8_t y,char character,const ui_font_t *font,ui_color_t color);
void ui_canvas_draw_char_scaled(uint8_t x,uint8_t y,char character,const ui_font_t *font,uint8_t scale,ui_color_t color);
/* 从指定位置开始绘制字符串 */
void ui_canvas_draw_text(uint8_t x,uint8_t y,const char *text,const ui_font_t *font,ui_color_t color);
void ui_canvas_draw_text_scaled(uint8_t x,uint8_t y,const char *text,const ui_font_t *font,uint8_t scale,ui_color_t color);
/* 使用默认 ASCII 与中文字体绘制 UTF-8 字符串 */
void ui_canvas_draw_utf8(uint8_t x,uint8_t y,const char *text,ui_color_t color);
uint16_t ui_canvas_measure_utf8(const char *text);
/* 获取画布缓冲区，只允许显示驱动读取 */
const uint8_t *ui_canvas_get_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_UI_CANVAS_H */
