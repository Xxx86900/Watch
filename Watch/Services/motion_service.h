#ifndef WATCH_SERVICES_MOTION_SERVICE_H
#define WATCH_SERVICES_MOTION_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "mpu6050.h"

typedef struct
{
  uint32_t steps;
  uint32_t distance_m;
  uint16_t calories_kcal;
  int16_t pitch_tenths;
  int16_t roll_tenths;
  bool valid;
  bool calibrated;
} motion_service_data_t;

void motion_service_init(mpu6050_t *sensor,uint32_t now_ms);
void motion_service_update(uint32_t now_ms);
void motion_service_calibrate(void);
void motion_service_reset_steps(void);
const motion_service_data_t *motion_service_get(void);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_SERVICES_MOTION_SERVICE_H */
