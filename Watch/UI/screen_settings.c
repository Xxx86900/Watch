#include "screen_settings.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

static const char *const s_setting_labels[SCREEN_SETTINGS_ITEM_COUNT] =
{
  "BRIGHT",
  "TIME",
  "DATE",
  "ABOUT"
};

static void screen_settings_percent(uint8_t percent,char text[5])
{
  if (percent > 100U)
  {
    percent = 100U;
  }
  if (percent == 100U)
  {
    text[0] = '1';
    text[1] = '0';
    text[2] = '0';
    text[3] = '%';
    text[4] = '\0';
  }
  else
  {
    text[0] = (char)('0' + (percent / 10U));
    text[1] = (char)('0' + (percent % 10U));
    text[2] = '%';
    text[3] = '\0';
  }
}

static void screen_settings_two_fields(uint8_t first,uint8_t second,char text[6])
{
  text[0] = (char)('0' + (first / 10U));
  text[1] = (char)('0' + (first % 10U));
  text[2] = ':';
  text[3] = (char)('0' + (second / 10U));
  text[4] = (char)('0' + (second % 10U));
  text[5] = '\0';
}

static const char *screen_settings_value(const screen_settings_data_t *data,
                                         screen_settings_item_t item,char value[8])
{
  switch (item)
  {
    case SCREEN_SETTINGS_BRIGHTNESS:
      screen_settings_percent(data->brightness_percent,value);
      return value;
    case SCREEN_SETTINGS_TIME:
      screen_settings_two_fields(data->hour,data->minute,value);
      return value;
    case SCREEN_SETTINGS_DATE:
      screen_settings_two_fields(data->month,data->day,value);
      value[2] = '-';
      return value;
    case SCREEN_SETTINGS_ABOUT:
      return (data->firmware_version != NULL) ? data->firmware_version : "V1.0";
    case SCREEN_SETTINGS_ITEM_COUNT:
    default:
      return "";
  }
}

void screen_settings_draw(const screen_settings_data_t *data)
{
  screen_settings_item_t selected;
  char value_buffer[8];

  if (data == NULL)
  {
    return;
  }

  selected = data->selected_item;
  if (selected >= SCREEN_SETTINGS_ITEM_COUNT)
  {
    selected = SCREEN_SETTINGS_BRIGHTNESS;
  }

  ui_canvas_clear(UI_COLOR_BLACK);
  ui_canvas_draw_utf8(4U,1U,"设置",UI_COLOR_WHITE);
  ui_canvas_draw_line(0U,18U,127U,18U,UI_COLOR_WHITE);

  for (screen_settings_item_t item = SCREEN_SETTINGS_BRIGHTNESS;
       item < SCREEN_SETTINGS_ITEM_COUNT; item++)
  {
    uint8_t y = (uint8_t)(21U + ((uint8_t)item * 10U));
    ui_color_t color = (item == selected) ? UI_COLOR_BLACK : UI_COLOR_WHITE;
    const char *value = screen_settings_value(data,item,value_buffer);
    uint16_t value_width = ui_canvas_measure_utf8(value);
    uint8_t value_x = (value_width < 122U) ? (uint8_t)(123U - value_width) : 1U;

    if (item == selected)
    {
      ui_canvas_fill_rectangle(2U,(uint8_t)(y - 2U),124U,10U,UI_COLOR_WHITE);
    }

    ui_canvas_draw_text(5U,y,s_setting_labels[item],&ui_font_5x7,color);
    ui_canvas_draw_text(value_x,y,value,&ui_font_5x7,color);

    if ((item == selected) && (data->edit_field != SCREEN_SETTINGS_EDIT_NONE))
    {
      uint8_t underline_x = value_x;
      if ((data->edit_field == SCREEN_SETTINGS_EDIT_MINUTE) ||
          (data->edit_field == SCREEN_SETTINGS_EDIT_DAY))
      {
        underline_x = (uint8_t)(value_x + 18U);
      }
      ui_canvas_draw_line(underline_x,(uint8_t)(y + 7U),
                          (uint8_t)(underline_x + 10U),(uint8_t)(y + 7U),color);
    }
  }
}
