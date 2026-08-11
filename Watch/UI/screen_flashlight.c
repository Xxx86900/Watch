#include "screen_flashlight.h"

#include "ui_canvas.h"

#include <stddef.h>

void screen_flashlight_draw(const screen_flashlight_data_t *data)
{
  const char *status;
  uint16_t status_width;

  if (data == NULL)
  {
    return;
  }

  ui_canvas_clear(UI_COLOR_BLACK);
  ui_canvas_draw_utf8(4U,1U,"手电筒",UI_COLOR_WHITE);
  ui_canvas_draw_line(0U,18U,127U,18U,UI_COLOR_WHITE);

  if (data->enabled)
  {
    ui_canvas_fill_circle(64,32,9U,UI_COLOR_WHITE);
  }
  else
  {
    ui_canvas_draw_circle(64,32,9U,UI_COLOR_WHITE);
  }

  ui_canvas_draw_line(64U,20U,64U,18U,UI_COLOR_WHITE);
  ui_canvas_draw_line(51U,24U,48U,22U,UI_COLOR_WHITE);
  ui_canvas_draw_line(77U,24U,80U,22U,UI_COLOR_WHITE);
  ui_canvas_draw_rectangle(59U,41U,11U,5U,UI_COLOR_WHITE);
  ui_canvas_draw_line(60U,47U,69U,47U,UI_COLOR_WHITE);

  status = data->enabled ? "开启" : "关闭";
  status_width = ui_canvas_measure_utf8(status);
  ui_canvas_draw_utf8((uint8_t)((128U - status_width) / 2U),48U,status,UI_COLOR_WHITE);
}
