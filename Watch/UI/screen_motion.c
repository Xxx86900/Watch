#include "screen_motion.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

static void screen_motion_format_u32(uint32_t value,char text[11])
{
  char reverse[10];
  uint8_t count = 0U;
  uint8_t index;

  do
  {
    reverse[count++] = (char)('0' + (value % 10U));
    value /= 10U;
  } while ((value > 0U) && (count < sizeof(reverse)));

  for (index = 0U; index < count; index++)
  {
    text[index] = reverse[count - index - 1U];
  }
  text[count] = '\0';
}

static void screen_motion_draw_value(uint8_t y,uint32_t value,const char *suffix)
{
  char text[14];
  uint8_t length = 0U;
  uint16_t width;

  screen_motion_format_u32(value,text);
  while (text[length] != '\0')
  {
    length++;
  }
  while ((*suffix != '\0') && (length < (sizeof(text) - 1U)))
  {
    text[length++] = *suffix++;
  }
  text[length] = '\0';

  width = ui_canvas_measure_utf8(text);
  ui_canvas_draw_text((uint8_t)(124U - width),y,text,&ui_font_5x7,UI_COLOR_WHITE);
}

void screen_motion_draw(const screen_motion_data_t *data)
{
  uint16_t text_width;

  if (data == NULL)
  {
    return;
  }

  ui_canvas_clear(UI_COLOR_BLACK);
  ui_canvas_draw_utf8(4U,1U,"运动",UI_COLOR_WHITE);
  ui_canvas_draw_line(0U,18U,127U,18U,UI_COLOR_WHITE);

  if (!data->data_valid)
  {
    text_width = ui_canvas_measure_utf8("无数据");
    ui_canvas_draw_utf8((uint8_t)((128U - text_width) / 2U),31U,"无数据",UI_COLOR_WHITE);
    return;
  }

  ui_canvas_draw_text(4U,23U,"STEPS",&ui_font_5x7,UI_COLOR_WHITE);
  screen_motion_draw_value(23U,data->steps,"");

  ui_canvas_draw_text(4U,36U,"DIST",&ui_font_5x7,UI_COLOR_WHITE);
  screen_motion_draw_value(36U,data->distance_m,"m");

  ui_canvas_draw_text(4U,49U,"KCAL",&ui_font_5x7,UI_COLOR_WHITE);
  screen_motion_draw_value(49U,data->calories_kcal,"");
}
