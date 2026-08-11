#ifndef WATCH_UI_SCREEN_LEVEL_H
#define WATCH_UI_SCREEN_LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  int16_t pitch_tenths;
  int16_t roll_tenths;
  bool calibrated;
} screen_level_data_t;

void screen_level_draw(const screen_level_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_LEVEL_H */
