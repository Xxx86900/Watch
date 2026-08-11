#ifndef WATCH_UI_SCREEN_FLASHLIGHT_H
#define WATCH_UI_SCREEN_FLASHLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct
{
  bool enabled;
} screen_flashlight_data_t;

void screen_flashlight_draw(const screen_flashlight_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_FLASHLIGHT_H */
