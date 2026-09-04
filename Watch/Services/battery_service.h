#ifndef WATCH_SERVICES_BATTERY_SERVICE_H
#define WATCH_SERVICES_BATTERY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

typedef struct
{
  uint16_t raw;
  uint16_t voltage_mv;
  uint8_t percent;
  bool valid;
} battery_service_data_t;

void battery_service_init(ADC_HandleTypeDef *adc);
bool battery_service_sample(void);
const battery_service_data_t *battery_service_get(void);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_SERVICES_BATTERY_SERVICE_H */
