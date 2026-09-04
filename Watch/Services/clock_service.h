#ifndef WATCH_SERVICES_CLOCK_SERVICE_H
#define WATCH_SERVICES_CLOCK_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

typedef struct
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} clock_service_time_t;

void clock_service_init(RTC_HandleTypeDef *rtc);
bool clock_service_get(clock_service_time_t *time);
bool clock_service_set(const clock_service_time_t *time);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_SERVICES_CLOCK_SERVICE_H */
