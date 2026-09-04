#include "watch_app.h"

#include "adc.h"
#include "app_config.h"
#include "app_event.h"
#include "battery_service.h"
#include "board.h"
#include "clock_service.h"
#include "i2c.h"
#include "input_service.h"
#include "motion_service.h"
#include "mpu6050.h"
#include "rtc.h"
#include "sh1106.h"
#include "ui_canvas.h"
#include "ui_manager.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
  sh1106_t display;
  mpu6050_t motion_sensor;
  ui_data_t ui;
  clock_service_time_t clock;
  uint32_t last_clock_ms;
  uint32_t last_battery_ms;
  uint32_t last_display_ms;
  uint32_t last_retry_ms;
  uint32_t last_interaction_ms;
  uint32_t stopwatch_base_ms;
  uint32_t stopwatch_started_ms;
  uint8_t brightness_percent;
  bool display_awake;
  bool flashlight_enabled;
  bool stopwatch_running;
} watch_app_state_t;

static watch_app_state_t s_app;

static uint8_t watch_app_days_in_month(uint16_t year,uint8_t month)
{
  static const uint8_t days[] = {31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U};
  bool leap = ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));

  if ((month < 1U) || (month > 12U))
  {
    return 31U;
  }
  if ((month == 2U) && leap)
  {
    return 29U;
  }
  return days[month - 1U];
}

static uint8_t watch_app_contrast(uint8_t percent)
{
  uint16_t contrast = ((uint16_t)percent * 255U) / 100U;
  return (contrast < 16U) ? 16U : (uint8_t)contrast;
}

static uint32_t watch_app_stopwatch_elapsed(uint32_t now_ms)
{
  if (s_app.stopwatch_running)
  {
    return s_app.stopwatch_base_ms + (uint32_t)(now_ms - s_app.stopwatch_started_ms);
  }
  return s_app.stopwatch_base_ms;
}

static void watch_app_refresh_clock(void)
{
  (void)clock_service_get(&s_app.clock);
}

static void watch_app_sync_ui(uint32_t now_ms)
{
  const battery_service_data_t *battery = battery_service_get();
  const motion_service_data_t *motion = motion_service_get();

  s_app.ui.home.hour = s_app.clock.hour;
  s_app.ui.home.minute = s_app.clock.minute;
  s_app.ui.home.month = s_app.clock.month;
  s_app.ui.home.day = s_app.clock.day;
  s_app.ui.home.battery_percent = battery->valid ? battery->percent : 0U;

  s_app.ui.motion.steps = motion->steps;
  s_app.ui.motion.distance_m = motion->distance_m;
  s_app.ui.motion.calories_kcal = motion->calories_kcal;
  s_app.ui.motion.data_valid = motion->valid;

  s_app.ui.level.pitch_tenths = motion->pitch_tenths;
  s_app.ui.level.roll_tenths = motion->roll_tenths;
  s_app.ui.level.calibrated = motion->valid && motion->calibrated;

  s_app.ui.stopwatch.elapsed_ms = watch_app_stopwatch_elapsed(now_ms);
  s_app.ui.stopwatch.running = s_app.stopwatch_running;
  s_app.ui.flashlight.enabled = s_app.flashlight_enabled;

  s_app.ui.settings.brightness_percent = s_app.brightness_percent;
  s_app.ui.settings.firmware_version = WATCH_FIRMWARE_VERSION;
  if (s_app.ui.settings.edit_field == SCREEN_SETTINGS_EDIT_NONE)
  {
    s_app.ui.settings.hour = s_app.clock.hour;
    s_app.ui.settings.minute = s_app.clock.minute;
    s_app.ui.settings.month = s_app.clock.month;
    s_app.ui.settings.day = s_app.clock.day;
  }
}

static void watch_app_draw(uint32_t now_ms)
{
  watch_app_sync_ui(now_ms);
  ui_manager_draw(&s_app.ui);
  if (sh1106_is_ready(&s_app.display))
  {
    (void)sh1106_update(&s_app.display,ui_canvas_get_buffer());
  }
}

static void watch_app_wake_display(uint32_t now_ms)
{
  s_app.display_awake = true;
  s_app.last_interaction_ms = now_ms;
  s_app.last_display_ms = now_ms - WATCH_DISPLAY_REFRESH_MS;
  if (sh1106_is_ready(&s_app.display))
  {
    (void)sh1106_set_power(&s_app.display,true);
  }
}

static void watch_app_leave_feature(void)
{
  if (ui_manager_get_screen() == UI_SCREEN_FLASHLIGHT)
  {
    s_app.flashlight_enabled = false;
    board_flashlight_set(false);
  }
  ui_manager_back();
}

static void watch_app_setting_save_time(void)
{
  clock_service_time_t time = s_app.clock;
  time.hour = s_app.ui.settings.hour;
  time.minute = s_app.ui.settings.minute;
  time.second = 0U;
  if (clock_service_set(&time))
  {
    s_app.clock = time;
  }
}

static void watch_app_setting_save_date(void)
{
  clock_service_time_t time = s_app.clock;
  time.month = s_app.ui.settings.month;
  time.day = s_app.ui.settings.day;
  if (clock_service_set(&time))
  {
    s_app.clock = time;
  }
}

static void watch_app_setting_adjust(bool increase)
{
  screen_settings_edit_t field = s_app.ui.settings.edit_field;

  if (field == SCREEN_SETTINGS_EDIT_HOUR)
  {
    s_app.ui.settings.hour = increase ?
      (uint8_t)((s_app.ui.settings.hour + 1U) % 24U) :
      (uint8_t)((s_app.ui.settings.hour + 23U) % 24U);
  }
  else if (field == SCREEN_SETTINGS_EDIT_MINUTE)
  {
    s_app.ui.settings.minute = increase ?
      (uint8_t)((s_app.ui.settings.minute + 1U) % 60U) :
      (uint8_t)((s_app.ui.settings.minute + 59U) % 60U);
  }
  else if (field == SCREEN_SETTINGS_EDIT_MONTH)
  {
    uint8_t max_day;
    s_app.ui.settings.month = increase ?
      (uint8_t)((s_app.ui.settings.month % 12U) + 1U) :
      (uint8_t)((s_app.ui.settings.month + 10U) % 12U + 1U);
    max_day = watch_app_days_in_month(s_app.clock.year,s_app.ui.settings.month);
    if (s_app.ui.settings.day > max_day)
    {
      s_app.ui.settings.day = max_day;
    }
  }
  else if (field == SCREEN_SETTINGS_EDIT_DAY)
  {
    uint8_t max_day = watch_app_days_in_month(s_app.clock.year,s_app.ui.settings.month);
    s_app.ui.settings.day = increase ?
      (uint8_t)((s_app.ui.settings.day % max_day) + 1U) :
      (uint8_t)((s_app.ui.settings.day + max_day - 2U) % max_day + 1U);
  }
}

static void watch_app_setting_confirm(void)
{
  if (s_app.ui.settings.edit_field == SCREEN_SETTINGS_EDIT_HOUR)
  {
    s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_MINUTE;
    return;
  }
  if (s_app.ui.settings.edit_field == SCREEN_SETTINGS_EDIT_MINUTE)
  {
    watch_app_setting_save_time();
    s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_NONE;
    return;
  }
  if (s_app.ui.settings.edit_field == SCREEN_SETTINGS_EDIT_MONTH)
  {
    s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_DAY;
    return;
  }
  if (s_app.ui.settings.edit_field == SCREEN_SETTINGS_EDIT_DAY)
  {
    watch_app_setting_save_date();
    s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_NONE;
    return;
  }

  switch (s_app.ui.settings.selected_item)
  {
    case SCREEN_SETTINGS_BRIGHTNESS:
      s_app.brightness_percent = (s_app.brightness_percent >= 100U) ?
                                 20U : (uint8_t)(s_app.brightness_percent + 20U);
      (void)sh1106_set_contrast(&s_app.display,watch_app_contrast(s_app.brightness_percent));
      break;
    case SCREEN_SETTINGS_TIME:
      s_app.ui.settings.hour = s_app.clock.hour;
      s_app.ui.settings.minute = s_app.clock.minute;
      s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_HOUR;
      break;
    case SCREEN_SETTINGS_DATE:
      s_app.ui.settings.month = s_app.clock.month;
      s_app.ui.settings.day = s_app.clock.day;
      s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_MONTH;
      break;
    case SCREEN_SETTINGS_ABOUT:
    case SCREEN_SETTINGS_ITEM_COUNT:
    default:
      break;
  }
}

static void watch_app_handle_settings(const app_event_t *event)
{
  if (s_app.ui.settings.edit_field != SCREEN_SETTINGS_EDIT_NONE)
  {
    if (event->key == APP_KEY_PREV)
    {
      watch_app_setting_adjust(false);
    }
    else if (event->key == APP_KEY_NEXT)
    {
      watch_app_setting_adjust(true);
    }
    else if (event->key == APP_KEY_OK)
    {
      watch_app_setting_confirm();
    }
    return;
  }

  if (event->key == APP_KEY_PREV)
  {
    s_app.ui.settings.selected_item =
      (s_app.ui.settings.selected_item == SCREEN_SETTINGS_BRIGHTNESS) ?
      (SCREEN_SETTINGS_ITEM_COUNT - 1) :
      (screen_settings_item_t)(s_app.ui.settings.selected_item - 1);
  }
  else if (event->key == APP_KEY_NEXT)
  {
    s_app.ui.settings.selected_item =
      (screen_settings_item_t)((s_app.ui.settings.selected_item + 1U) %
                               SCREEN_SETTINGS_ITEM_COUNT);
  }
  else if (event->key == APP_KEY_OK)
  {
    watch_app_setting_confirm();
  }
}

static void watch_app_handle_short_event(const app_event_t *event,uint32_t now_ms)
{
  ui_screen_t screen = ui_manager_get_screen();

  switch (screen)
  {
    case UI_SCREEN_HOME:
      if (event->key == APP_KEY_OK)
      {
        ui_manager_set_screen(UI_SCREEN_MENU);
      }
      break;

    case UI_SCREEN_MENU:
      if (event->key == APP_KEY_PREV)
      {
        s_app.ui.menu.selected_item =
          (s_app.ui.menu.selected_item == SCREEN_MENU_MOTION) ?
          (SCREEN_MENU_ITEM_COUNT - 1) :
          (screen_menu_item_t)(s_app.ui.menu.selected_item - 1);
      }
      else if (event->key == APP_KEY_NEXT)
      {
        s_app.ui.menu.selected_item =
          (screen_menu_item_t)((s_app.ui.menu.selected_item + 1U) %
                               SCREEN_MENU_ITEM_COUNT);
      }
      else if (event->key == APP_KEY_OK)
      {
        ui_manager_open_menu_item(s_app.ui.menu.selected_item);
      }
      break;

    case UI_SCREEN_MOTION:
      if (event->key == APP_KEY_PREV)
      {
        watch_app_leave_feature();
      }
      else if (event->key == APP_KEY_NEXT)
      {
        motion_service_reset_steps();
      }
      break;

    case UI_SCREEN_LEVEL:
      if (event->key == APP_KEY_PREV)
      {
        watch_app_leave_feature();
      }
      else if (event->key == APP_KEY_OK)
      {
        motion_service_calibrate();
      }
      break;

    case UI_SCREEN_STOPWATCH:
      if (event->key == APP_KEY_PREV)
      {
        watch_app_leave_feature();
      }
      else if (event->key == APP_KEY_OK)
      {
        if (s_app.stopwatch_running)
        {
          s_app.stopwatch_base_ms = watch_app_stopwatch_elapsed(now_ms);
          s_app.stopwatch_running = false;
        }
        else
        {
          s_app.stopwatch_started_ms = now_ms;
          s_app.stopwatch_running = true;
        }
      }
      else if ((event->key == APP_KEY_NEXT) && !s_app.stopwatch_running)
      {
        s_app.stopwatch_base_ms = 0U;
      }
      break;

    case UI_SCREEN_FLASHLIGHT:
      if (event->key == APP_KEY_PREV)
      {
        watch_app_leave_feature();
      }
      else if ((event->key == APP_KEY_OK) || (event->key == APP_KEY_NEXT))
      {
        s_app.flashlight_enabled = !s_app.flashlight_enabled;
        board_flashlight_set(s_app.flashlight_enabled);
      }
      break;

    case UI_SCREEN_SETTINGS:
      watch_app_handle_settings(event);
      break;

    case UI_SCREEN_COUNT:
    default:
      ui_manager_set_screen(UI_SCREEN_HOME);
      break;
  }
}

static void watch_app_handle_event(const app_event_t *event,uint32_t now_ms)
{
  if (!s_app.display_awake)
  {
    watch_app_wake_display(now_ms);
    return;
  }

  s_app.last_interaction_ms = now_ms;
  if ((event->key == APP_KEY_OK) && (event->action == APP_KEY_ACTION_LONG))
  {
    if (ui_manager_get_screen() == UI_SCREEN_HOME)
    {
      board_power_off();
    }
    if ((ui_manager_get_screen() == UI_SCREEN_SETTINGS) &&
        (s_app.ui.settings.edit_field != SCREEN_SETTINGS_EDIT_NONE))
    {
      s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_NONE;
      watch_app_refresh_clock();
    }
    else
    {
      watch_app_leave_feature();
    }
    return;
  }

  if (event->action == APP_KEY_ACTION_SHORT)
  {
    watch_app_handle_short_event(event,now_ms);
  }
}

void watch_app_init(void)
{
  uint32_t now_ms = HAL_GetTick();

  memset(&s_app,0,sizeof(s_app));
  s_app.brightness_percent = WATCH_DEFAULT_BRIGHTNESS_PERCENT;
  s_app.display_awake = true;
  s_app.last_interaction_ms = now_ms;
  s_app.last_clock_ms = now_ms - WATCH_CLOCK_REFRESH_MS;
  s_app.last_battery_ms = now_ms - WATCH_BATTERY_REFRESH_MS;
  s_app.last_display_ms = now_ms - WATCH_DISPLAY_REFRESH_MS;
  s_app.last_retry_ms = now_ms;
  s_app.ui.menu.selected_item = SCREEN_MENU_MOTION;
  s_app.ui.settings.selected_item = SCREEN_SETTINGS_BRIGHTNESS;
  s_app.ui.settings.edit_field = SCREEN_SETTINGS_EDIT_NONE;

  input_service_init(now_ms);
  clock_service_init(&hrtc);
  battery_service_init(&hadc1);
  ui_manager_init();

  (void)mpu6050_init(&s_app.motion_sensor,&hi2c2);
  motion_service_init(&s_app.motion_sensor,now_ms);

  if (sh1106_init(&s_app.display,&hi2c1))
  {
    (void)sh1106_set_contrast(&s_app.display,watch_app_contrast(s_app.brightness_percent));
  }

  watch_app_refresh_clock();
  (void)battery_service_sample();
  watch_app_draw(now_ms);
}

void watch_app_process(void)
{
  uint32_t now_ms = HAL_GetTick();
  app_event_t event;

  input_service_update(now_ms);
  while (input_service_get_event(&event))
  {
    watch_app_handle_event(&event,now_ms);
    s_app.last_display_ms = now_ms - WATCH_DISPLAY_REFRESH_MS;
  }

  motion_service_update(now_ms);

  if ((uint32_t)(now_ms - s_app.last_clock_ms) >= WATCH_CLOCK_REFRESH_MS)
  {
    s_app.last_clock_ms = now_ms;
    watch_app_refresh_clock();
  }

  if ((uint32_t)(now_ms - s_app.last_battery_ms) >= WATCH_BATTERY_REFRESH_MS)
  {
    s_app.last_battery_ms = now_ms;
    (void)battery_service_sample();
  }

  if ((uint32_t)(now_ms - s_app.last_retry_ms) >= WATCH_DEVICE_RETRY_MS)
  {
    s_app.last_retry_ms = now_ms;
    if (!mpu6050_is_ready(&s_app.motion_sensor))
    {
      (void)mpu6050_init(&s_app.motion_sensor,&hi2c2);
    }
    if (!sh1106_is_ready(&s_app.display) && sh1106_init(&s_app.display,&hi2c1))
    {
      (void)sh1106_set_contrast(&s_app.display,
                                watch_app_contrast(s_app.brightness_percent));
      s_app.last_display_ms = now_ms - WATCH_DISPLAY_REFRESH_MS;
    }
  }

  if (s_app.display_awake &&
      ((uint32_t)(now_ms - s_app.last_interaction_ms) >= WATCH_DISPLAY_SLEEP_MS))
  {
    s_app.display_awake = false;
    (void)sh1106_set_power(&s_app.display,false);
  }

  if (s_app.display_awake &&
      ((uint32_t)(now_ms - s_app.last_display_ms) >= WATCH_DISPLAY_REFRESH_MS))
  {
    s_app.last_display_ms = now_ms;
    watch_app_draw(now_ms);
  }
}
