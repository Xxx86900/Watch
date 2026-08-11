#include "ui_manager.h"

#include "ui_canvas.h"

#include <stddef.h>

static ui_screen_t s_current_screen = UI_SCREEN_HOME;

void ui_manager_init(void)
{
  ui_canvas_init();
  s_current_screen = UI_SCREEN_HOME;
}

void ui_manager_set_screen(ui_screen_t screen)
{
  if ((screen >= UI_SCREEN_HOME) && (screen < UI_SCREEN_COUNT))
  {
    s_current_screen = screen;
  }
}

ui_screen_t ui_manager_get_screen(void)
{
  return s_current_screen;
}

void ui_manager_open_menu_item(screen_menu_item_t item)
{
  switch (item)
  {
    case SCREEN_MENU_MOTION:
      s_current_screen = UI_SCREEN_MOTION;
      break;
    case SCREEN_MENU_LEVEL:
      s_current_screen = UI_SCREEN_LEVEL;
      break;
    case SCREEN_MENU_STOPWATCH:
      s_current_screen = UI_SCREEN_STOPWATCH;
      break;
    case SCREEN_MENU_FLASHLIGHT:
      s_current_screen = UI_SCREEN_FLASHLIGHT;
      break;
    case SCREEN_MENU_SETTINGS:
      s_current_screen = UI_SCREEN_SETTINGS;
      break;
    case SCREEN_MENU_ITEM_COUNT:
    default:
      break;
  }
}

void ui_manager_back(void)
{
  if (s_current_screen == UI_SCREEN_MENU)
  {
    s_current_screen = UI_SCREEN_HOME;
  }
  else if (s_current_screen != UI_SCREEN_HOME)
  {
    s_current_screen = UI_SCREEN_MENU;
  }
}

void ui_manager_draw(const ui_data_t *data)
{
  if (data == NULL)
  {
    return;
  }

  switch (s_current_screen)
  {
    case UI_SCREEN_HOME:
      screen_home_draw(&data->home);
      break;
    case UI_SCREEN_MENU:
      screen_menu_draw(&data->menu);
      break;
    case UI_SCREEN_MOTION:
      screen_motion_draw(&data->motion);
      break;
    case UI_SCREEN_LEVEL:
      screen_level_draw(&data->level);
      break;
    case UI_SCREEN_STOPWATCH:
      screen_stopwatch_draw(&data->stopwatch);
      break;
    case UI_SCREEN_FLASHLIGHT:
      screen_flashlight_draw(&data->flashlight);
      break;
    case UI_SCREEN_SETTINGS:
      screen_settings_draw(&data->settings);
      break;
    case UI_SCREEN_COUNT:
    default:
      ui_canvas_clear(UI_COLOR_BLACK);
      break;
  }
}
