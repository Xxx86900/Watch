#ifndef WATCH_APP_APP_EVENT_H
#define WATCH_APP_APP_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  APP_KEY_PREV = 0,
  APP_KEY_NEXT,
  APP_KEY_OK,
  APP_KEY_COUNT
} app_key_t;

typedef enum
{
  APP_KEY_ACTION_SHORT = 0,
  APP_KEY_ACTION_LONG
} app_key_action_t;

typedef struct
{
  app_key_t key;
  app_key_action_t action;
} app_event_t;

#ifdef __cplusplus
}
#endif

#endif /* WATCH_APP_APP_EVENT_H */
