#include "screen_stopwatch.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

void screen_stopwatch_draw(const screen_stopwatch_data_t *data)
{
  uint32_t centiseconds;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t hundredths;
  uint16_t status_width;
  const char *status;
  char time_text[6];
  char fraction_text[4];

  if (data == NULL)
  {
    return;
  }

  centiseconds = data->elapsed_ms / 10U;
  minutes = (uint8_t)((centiseconds / 6000U) % 100U);
  seconds = (uint8_t)((centiseconds / 100U) % 60U);
  hundredths = (uint8_t)(centiseconds % 100U);

  time_text[0] = (char)('0' + (minutes / 10U));
  time_text[1] = (char)('0' + (minutes % 10U));
  time_text[2] = ':';
  time_text[3] = (char)('0' + (seconds / 10U));
  time_text[4] = (char)('0' + (seconds % 10U));
  time_text[5] = '\0';

  fraction_text[0] = '.';
  fraction_text[1] = (char)('0' + (hundredths / 10U));
  fraction_text[2] = (char)('0' + (hundredths % 10U));
  fraction_text[3] = '\0';

  ui_canvas_clear(UI_COLOR_BLACK);
  ui_canvas_draw_utf8(4U,1U,"秒表",UI_COLOR_WHITE);
  if (data->running)
  {
    ui_canvas_fill_circle(119,8,3U,UI_COLOR_WHITE);
  }
  ui_canvas_draw_line(0U,18U,127U,18U,UI_COLOR_WHITE);

  ui_canvas_draw_text_scaled(20U,23U,time_text,&ui_font_5x7,3U,UI_COLOR_WHITE);
  ui_canvas_draw_text(108U,38U,fraction_text,&ui_font_5x7,UI_COLOR_WHITE);

  status = data->running ? "运行" : "暂停";
  status_width = ui_canvas_measure_utf8(status);
  ui_canvas_draw_utf8((uint8_t)((128U - status_width) / 2U),47U,status,UI_COLOR_WHITE);
}
