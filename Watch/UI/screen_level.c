#include "screen_level.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

static int16_t screen_level_clamp(int16_t value,int16_t minimum,int16_t maximum)
{
  if (value < minimum)
  {
    return minimum;
  }
  if (value > maximum)
  {
    return maximum;
  }
  return value;
}

static void screen_level_format_angle(char prefix,int16_t tenths,char text[7])
{
  int16_t value = tenths / 10;
  uint16_t absolute;

  value = screen_level_clamp(value,-99,99);
  absolute = (value < 0) ? (uint16_t)(-value) : (uint16_t)value;
  text[0] = prefix;
  text[1] = ':';
  text[2] = (value < 0) ? '-' : '+';
  text[3] = (char)('0' + (absolute / 10U));
  text[4] = (char)('0' + (absolute % 10U));
  text[5] = '\0';
}

void screen_level_draw(const screen_level_data_t *data)
{
  int16_t bubble_x;
  int16_t bubble_y;
  uint16_t text_width;
  char pitch_text[7];
  char roll_text[7];

  if (data == NULL)
  {
    return;
  }

  ui_canvas_clear(UI_COLOR_BLACK);
  ui_canvas_draw_utf8(4U,1U,"水平仪",UI_COLOR_WHITE);
  ui_canvas_draw_line(0U,18U,127U,18U,UI_COLOR_WHITE);

  if (!data->calibrated)
  {
    text_width = ui_canvas_measure_utf8("未校准");
    ui_canvas_draw_utf8((uint8_t)((128U - text_width) / 2U),31U,"未校准",UI_COLOR_WHITE);
    return;
  }

  ui_canvas_draw_circle(64,38,16U,UI_COLOR_WHITE);
  ui_canvas_draw_line(44U,38U,84U,38U,UI_COLOR_WHITE);
  ui_canvas_draw_line(64U,20U,64U,56U,UI_COLOR_WHITE);

  bubble_x = 64 + screen_level_clamp((int16_t)(data->roll_tenths / 25),-12,12);
  bubble_y = 38 + screen_level_clamp((int16_t)(data->pitch_tenths / 25),-12,12);
  ui_canvas_fill_circle(bubble_x,bubble_y,3U,UI_COLOR_WHITE);

  screen_level_format_angle('P',data->pitch_tenths,pitch_text);
  screen_level_format_angle('R',data->roll_tenths,roll_text);
  ui_canvas_draw_text(2U,56U,pitch_text,&ui_font_5x7,UI_COLOR_WHITE);
  ui_canvas_draw_text(93U,56U,roll_text,&ui_font_5x7,UI_COLOR_WHITE);
}
