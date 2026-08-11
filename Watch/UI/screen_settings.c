#include "screen_settings.h"

#include "fonts.h"
#include "ui_canvas.h"

#include <stddef.h>

static const char *const s_setting_labels[SCREEN_SETTINGS_ITEM_COUNT] =
{
  "BRIGHT",
  "VIBRATE",
  "LANG",
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

static const char *screen_settings_value(const screen_settings_data_t *data,
                                         screen_settings_item_t item,char percent[5])
{
  switch (item)
  {
    case SCREEN_SETTINGS_BRIGHTNESS:
      screen_settings_percent(data->brightness_percent,percent);
      return percent;
    case SCREEN_SETTINGS_VIBRATION:
      return data->vibration_enabled ? "ON" : "OFF";
    case SCREEN_SETTINGS_LANGUAGE:
      return data->chinese_language ? "CN" : "EN";
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
  screen_settings_item_t item;
  const char *value;
  uint16_t value_width;
  uint8_t y;
  ui_color_t color;
  char percent[5];

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

  for (item = SCREEN_SETTINGS_BRIGHTNESS; item < SCREEN_SETTINGS_ITEM_COUNT; item++)
  {
    y = (uint8_t)(21U + ((uint8_t)item * 10U));
    color = (item == selected) ? UI_COLOR_BLACK : UI_COLOR_WHITE;
    if (item == selected)
    {
      ui_canvas_fill_rectangle(2U,(uint8_t)(y - 2U),124U,10U,UI_COLOR_WHITE);
    }

    value = screen_settings_value(data,item,percent);
    value_width = ui_canvas_measure_utf8(value);
    ui_canvas_draw_text(5U,y,s_setting_labels[item],&ui_font_5x7,color);
    ui_canvas_draw_text((uint8_t)(123U - value_width),y,value,&ui_font_5x7,color);
  }
}
