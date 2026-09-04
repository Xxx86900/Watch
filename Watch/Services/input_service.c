#include "input_service.h"

#include "app_config.h"
#include "board.h"

#include <stddef.h>

#define INPUT_EVENT_QUEUE_SIZE 8U

typedef struct
{
  bool raw_pressed;
  bool stable_pressed;
  bool long_sent;
  uint32_t raw_changed_at;
  uint32_t pressed_at;
} input_button_state_t;

static input_button_state_t s_buttons[APP_KEY_COUNT];
static app_event_t s_queue[INPUT_EVENT_QUEUE_SIZE];
static uint8_t s_queue_read;
static uint8_t s_queue_write;
static uint32_t s_last_scan_ms;

static board_key_t input_service_board_key(app_key_t key)
{
  switch (key)
  {
    case APP_KEY_PREV:
      return BOARD_KEY_PREV;
    case APP_KEY_NEXT:
      return BOARD_KEY_NEXT;
    case APP_KEY_OK:
    case APP_KEY_COUNT:
    default:
      return BOARD_KEY_OK;
  }
}

static void input_service_push(app_key_t key,app_key_action_t action)
{
  uint8_t next = (uint8_t)((s_queue_write + 1U) % INPUT_EVENT_QUEUE_SIZE);

  if (next == s_queue_read)
  {
    s_queue_read = (uint8_t)((s_queue_read + 1U) % INPUT_EVENT_QUEUE_SIZE);
  }

  s_queue[s_queue_write].key = key;
  s_queue[s_queue_write].action = action;
  s_queue_write = next;
}

void input_service_init(uint32_t now_ms)
{
  s_queue_read = 0U;
  s_queue_write = 0U;
  s_last_scan_ms = now_ms;

  for (app_key_t key = APP_KEY_PREV; key < APP_KEY_COUNT; key++)
  {
    bool pressed = board_key_is_pressed(input_service_board_key(key));
    s_buttons[key].raw_pressed = pressed;
    s_buttons[key].stable_pressed = pressed;
    /* Ignore the release of a key that was held while the MCU powered up. */
    s_buttons[key].long_sent = pressed;
    s_buttons[key].raw_changed_at = now_ms;
    s_buttons[key].pressed_at = now_ms;
  }
}

void input_service_update(uint32_t now_ms)
{
  if ((uint32_t)(now_ms - s_last_scan_ms) < WATCH_INPUT_SCAN_PERIOD_MS)
  {
    return;
  }
  s_last_scan_ms = now_ms;

  for (app_key_t key = APP_KEY_PREV; key < APP_KEY_COUNT; key++)
  {
    input_button_state_t *button = &s_buttons[key];
    bool pressed = board_key_is_pressed(input_service_board_key(key));

    if (pressed != button->raw_pressed)
    {
      button->raw_pressed = pressed;
      button->raw_changed_at = now_ms;
    }

    if ((pressed != button->stable_pressed) &&
        ((uint32_t)(now_ms - button->raw_changed_at) >= WATCH_INPUT_DEBOUNCE_MS))
    {
      button->stable_pressed = pressed;
      if (pressed)
      {
        button->pressed_at = now_ms;
        button->long_sent = false;
      }
      else if (!button->long_sent)
      {
        input_service_push(key,APP_KEY_ACTION_SHORT);
      }
    }

    if (button->stable_pressed && !button->long_sent &&
        ((uint32_t)(now_ms - button->pressed_at) >= WATCH_INPUT_LONG_PRESS_MS))
    {
      button->long_sent = true;
      input_service_push(key,APP_KEY_ACTION_LONG);
    }
  }
}

bool input_service_get_event(app_event_t *event)
{
  if ((event == NULL) || (s_queue_read == s_queue_write))
  {
    return false;
  }

  *event = s_queue[s_queue_read];
  s_queue_read = (uint8_t)((s_queue_read + 1U) % INPUT_EVENT_QUEUE_SIZE);
  return true;
}
