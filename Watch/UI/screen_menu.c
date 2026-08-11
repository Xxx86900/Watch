#include "screen_menu.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

static const char *const s_menu_labels[SCREEN_MENU_ITEM_COUNT] =
{
  "运动",
  "水平仪",
  "秒表",
  "手电筒",
  "设置"
};

void screen_menu_draw(const screen_menu_data_t *data)
{
  screen_menu_item_t item;
  const char *label;
  uint16_t label_width;
  uint8_t label_x;
  char page_text[4];

  if (data == NULL)
  {
    return;
  }

  item = data->selected_item;
  if (item >= SCREEN_MENU_ITEM_COUNT)
  {
    item = SCREEN_MENU_MOTION;
  }

  label = s_menu_labels[item];
  label_width = ui_canvas_measure_utf8(label);
  label_x = (label_width < UI_CANVAS_WIDTH) ?
            (uint8_t)((UI_CANVAS_WIDTH - label_width) / 2U) : 0U;

  page_text[0] = (char)('1' + item);
  page_text[1] = '/';
  page_text[2] = (char)('0' + SCREEN_MENU_ITEM_COUNT);
  page_text[3] = '\0';

  ui_canvas_clear(UI_COLOR_BLACK);
  ui_canvas_draw_utf8(4U,1U,"菜单",UI_COLOR_WHITE);
  ui_canvas_draw_text(108U,5U,page_text,&ui_font_5x7,UI_COLOR_WHITE);
  ui_canvas_draw_line(0U,18U,127U,18U,UI_COLOR_WHITE);

  ui_canvas_fill_rectangle(4U,23U,120U,28U,UI_COLOR_WHITE);
  ui_canvas_draw_utf8(label_x,29U,label,UI_COLOR_BLACK);

  ui_canvas_draw_text(5U,56U,"<",&ui_font_5x7,UI_COLOR_WHITE);
  ui_canvas_draw_text(58U,56U,"OK",&ui_font_5x7,UI_COLOR_WHITE);
  ui_canvas_draw_text(119U,56U,">",&ui_font_5x7,UI_COLOR_WHITE);
}
