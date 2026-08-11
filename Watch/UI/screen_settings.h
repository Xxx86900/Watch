#ifndef WATCH_UI_SCREEN_SETTINGS_H
#define WATCH_UI_SCREEN_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  SCREEN_SETTINGS_BRIGHTNESS = 0,
  SCREEN_SETTINGS_VIBRATION,
  SCREEN_SETTINGS_LANGUAGE,
  SCREEN_SETTINGS_ABOUT,
  SCREEN_SETTINGS_ITEM_COUNT
} screen_settings_item_t;

typedef struct
{
  screen_settings_item_t selected_item;
  uint8_t brightness_percent;
  bool vibration_enabled;
  bool chinese_language;
  const char *firmware_version;
} screen_settings_data_t;

void screen_settings_draw(const screen_settings_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_SETTINGS_H */
