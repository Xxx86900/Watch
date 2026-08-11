#ifndef WATCH_UI_SCREEN_MOTION_H
#define WATCH_UI_SCREEN_MOTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint32_t steps;
  uint32_t distance_m;
  uint16_t calories_kcal;
  bool data_valid;
} screen_motion_data_t;

void screen_motion_draw(const screen_motion_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_MOTION_H */
