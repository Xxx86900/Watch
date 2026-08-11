#ifndef WATCH_UI_SCREEN_MENU_H
#define WATCH_UI_SCREEN_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  SCREEN_MENU_MOTION = 0,
  SCREEN_MENU_LEVEL,
  SCREEN_MENU_STOPWATCH,
  SCREEN_MENU_FLASHLIGHT,
  SCREEN_MENU_SETTINGS,
  SCREEN_MENU_ITEM_COUNT
} screen_menu_item_t;

typedef struct
{
  screen_menu_item_t selected_item;
} screen_menu_data_t;

void screen_menu_draw(const screen_menu_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_MENU_H */
