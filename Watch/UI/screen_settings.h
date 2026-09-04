#ifndef WATCH_UI_SCREEN_SETTINGS_H
#define WATCH_UI_SCREEN_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  SCREEN_SETTINGS_BRIGHTNESS = 0,
  SCREEN_SETTINGS_TIME,
  SCREEN_SETTINGS_DATE,
  SCREEN_SETTINGS_ABOUT,
  SCREEN_SETTINGS_ITEM_COUNT
} screen_settings_item_t;

typedef enum
{
  SCREEN_SETTINGS_EDIT_NONE = 0,
  SCREEN_SETTINGS_EDIT_HOUR,
  SCREEN_SETTINGS_EDIT_MINUTE,
  SCREEN_SETTINGS_EDIT_MONTH,
  SCREEN_SETTINGS_EDIT_DAY
} screen_settings_edit_t;

typedef struct
{
  screen_settings_item_t selected_item;
  screen_settings_edit_t edit_field;
  uint8_t brightness_percent;
  uint8_t hour;
  uint8_t minute;
  uint8_t month;
  uint8_t day;
  const char *firmware_version;
} screen_settings_data_t;

void screen_settings_draw(const screen_settings_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_SETTINGS_H */
