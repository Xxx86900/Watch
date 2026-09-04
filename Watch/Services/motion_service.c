#include "motion_service.h"

#include "app_config.h"

#include <stddef.h>

#define MOTION_STEP_THRESHOLD       1800
#define MOTION_STEP_RELEASE          700
#define MOTION_STEP_REFRACTORY_MS    280U

static mpu6050_t *s_sensor;
static motion_service_data_t s_data;
static uint32_t s_last_update_ms;
static uint32_t s_last_step_ms;
static int32_t s_gravity;
static bool s_step_high;
static int16_t s_pitch_offset;
static int16_t s_roll_offset;

static uint16_t motion_service_isqrt(uint32_t value)
{
  uint32_t result = 0U;
  uint32_t bit = 1UL << 30U;

  while (bit > value)
  {
    bit >>= 2U;
  }
  while (bit != 0U)
  {
    if (value >= result + bit)
    {
      value -= result + bit;
      result = (result >> 1U) + bit;
    }
    else
    {
      result >>= 1U;
    }
    bit >>= 2U;
  }
  return (uint16_t)result;
}

static int16_t motion_service_atan2_tenths(int32_t y,int32_t x)
{
  uint32_t abs_x = (x < 0) ? (uint32_t)(-x) : (uint32_t)x;
  uint32_t abs_y = (y < 0) ? (uint32_t)(-y) : (uint32_t)y;
  int32_t angle;

  if ((abs_x == 0U) && (abs_y == 0U))
  {
    return 0;
  }

  if (abs_y <= abs_x)
  {
    angle = (int32_t)((abs_y * 450U) / ((abs_x == 0U) ? 1U : abs_x));
  }
  else
  {
    angle = 900 - (int32_t)((abs_x * 450U) / abs_y);
  }

  if (x < 0)
  {
    angle = 1800 - angle;
  }
  if (y < 0)
  {
    angle = -angle;
  }
  return (int16_t)angle;
}

void motion_service_init(mpu6050_t *sensor,uint32_t now_ms)
{
  s_sensor = sensor;
  s_last_update_ms = now_ms;
  s_last_step_ms = now_ms;
  s_gravity = 16384;
  s_step_high = false;
  s_pitch_offset = 0;
  s_roll_offset = 0;
  s_data.steps = 0U;
  s_data.distance_m = 0U;
  s_data.calories_kcal = 0U;
  s_data.pitch_tenths = 0;
  s_data.roll_tenths = 0;
  s_data.valid = false;
  s_data.calibrated = false;
}

void motion_service_update(uint32_t now_ms)
{
  mpu6050_sample_t sample;
  uint32_t horizontal_squared;
  uint32_t magnitude_squared;
  uint16_t horizontal;
  uint16_t magnitude;
  int32_t dynamic;
  int16_t pitch;
  int16_t roll;

  if ((uint32_t)(now_ms - s_last_update_ms) < WATCH_MOTION_REFRESH_MS)
  {
    return;
  }
  s_last_update_ms = now_ms;

  if ((s_sensor == NULL) || !mpu6050_read(s_sensor,&sample))
  {
    s_data.valid = false;
    return;
  }

  horizontal_squared = (uint32_t)((int32_t)sample.accel_y * sample.accel_y) +
                       (uint32_t)((int32_t)sample.accel_z * sample.accel_z);
  magnitude_squared = horizontal_squared +
                      (uint32_t)((int32_t)sample.accel_x * sample.accel_x);
  horizontal = motion_service_isqrt(horizontal_squared);
  magnitude = motion_service_isqrt(magnitude_squared);

  pitch = motion_service_atan2_tenths(-(int32_t)sample.accel_x,horizontal);
  roll = motion_service_atan2_tenths(sample.accel_y,sample.accel_z);
  s_data.pitch_tenths = (int16_t)(pitch - s_pitch_offset);
  s_data.roll_tenths = (int16_t)(roll - s_roll_offset);
  s_data.valid = true;

  s_gravity = ((s_gravity * 15) + magnitude) / 16;
  dynamic = (int32_t)magnitude - s_gravity;

  if (!s_step_high && (dynamic > MOTION_STEP_THRESHOLD) &&
      ((uint32_t)(now_ms - s_last_step_ms) >= MOTION_STEP_REFRACTORY_MS))
  {
    s_step_high = true;
    s_last_step_ms = now_ms;
    s_data.steps++;
    s_data.distance_m = (s_data.steps * 70U) / 100U;
    s_data.calories_kcal = (uint16_t)((s_data.steps * 4U) / 100U);
  }
  else if (dynamic < MOTION_STEP_RELEASE)
  {
    s_step_high = false;
  }
}

void motion_service_calibrate(void)
{
  if (s_data.valid)
  {
    s_pitch_offset += s_data.pitch_tenths;
    s_roll_offset += s_data.roll_tenths;
    s_data.pitch_tenths = 0;
    s_data.roll_tenths = 0;
    s_data.calibrated = true;
  }
}

void motion_service_reset_steps(void)
{
  s_data.steps = 0U;
  s_data.distance_m = 0U;
  s_data.calories_kcal = 0U;
}

const motion_service_data_t *motion_service_get(void)
{
  return &s_data;
}
