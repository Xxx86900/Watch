#ifndef WATCH_COMPONENTS_SH1106_H
#define WATCH_COMPONENTS_SH1106_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

#define SH1106_WIDTH  128U
#define SH1106_HEIGHT  64U
#define SH1106_BUFFER_SIZE (SH1106_WIDTH * SH1106_HEIGHT / 8U)

typedef struct
{
  I2C_HandleTypeDef *i2c;
  bool ready;
} sh1106_t;

bool sh1106_init(sh1106_t *display,I2C_HandleTypeDef *i2c);
bool sh1106_update(sh1106_t *display,const uint8_t *buffer);
bool sh1106_set_contrast(sh1106_t *display,uint8_t contrast);
bool sh1106_set_power(sh1106_t *display,bool enabled);
bool sh1106_is_ready(const sh1106_t *display);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_COMPONENTS_SH1106_H */
