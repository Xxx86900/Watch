#ifndef WATCH_UI_SCREEN_STOPWATCH_H
#define WATCH_UI_SCREEN_STOPWATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint32_t elapsed_ms;
  bool running;
} screen_stopwatch_data_t;

void screen_stopwatch_draw(const screen_stopwatch_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_UI_SCREEN_STOPWATCH_H */
