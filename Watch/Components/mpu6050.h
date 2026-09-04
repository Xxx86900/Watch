#ifndef WATCH_COMPONENTS_MPU6050_H
#define WATCH_COMPONENTS_MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

typedef struct
{
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t temperature;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
} mpu6050_sample_t;

typedef struct
{
  I2C_HandleTypeDef *i2c;
  bool ready;
} mpu6050_t;

bool mpu6050_init(mpu6050_t *device,I2C_HandleTypeDef *i2c);
bool mpu6050_read(mpu6050_t *device,mpu6050_sample_t *sample);
bool mpu6050_set_sleep(mpu6050_t *device,bool sleep);
bool mpu6050_is_ready(const mpu6050_t *device);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_COMPONENTS_MPU6050_H */
