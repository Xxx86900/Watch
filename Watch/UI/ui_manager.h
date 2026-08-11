#ifndef WATCH_UI_UI_MANAGER_H
#define WATCH_UI_UI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "screen_home.h"
#include "screen_menu.h"
#include "screen_motion.h"
#include "screen_level.h"
#include "screen_stopwatch.h"
#include "screen_flashlight.h"
#include "screen_settings.h"

/* 手表的所有页面 */
typedef enum
{
  UI_SCREEN_HOME = 0,
  UI_SCREEN_MENU,
  UI_SCREEN_MOTION,
  UI_SCREEN_LEVEL,
  UI_SCREEN_STOPWATCH,
  UI_SCREEN_FLASHLIGHT,
  UI_SCREEN_SETTINGS,
  UI_SCREEN_COUNT
} ui_screen_t;

/* UI 绘制时需要的数据，后续逐步增加其他页面数据 */
typedef struct
{
  screen_home_data_t home;
  screen_menu_data_t menu;
  screen_motion_data_t motion;
  screen_level_data_t level;
  screen_stopwatch_data_t stopwatch;
  screen_flashlight_data_t flashlight;
  screen_settings_data_t settings;
} ui_data_t;

/* 初始化 UI，默认进入主界面 */
void ui_manager_init(void);

/* 切换当前页面 */
void ui_manager_set_screen(ui_screen_t screen);

/* 获取当前页面 */
ui_screen_t ui_manager_get_screen(void);

void ui_manager_open_menu_item(screen_menu_item_t item);
void ui_manager_back(void);

/* 根据当前页面生成一帧画面 */
void ui_manager_draw(const ui_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_UI_MANAGER_H */
