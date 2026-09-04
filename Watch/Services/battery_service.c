#include "battery_service.h"

#include "board.h"

#include <stddef.h>

#define BATTERY_ADC_SAMPLES       8U
#define BATTERY_ADC_TIMEOUT_MS   10U
#define BATTERY_ADC_REFERENCE_MV 3300U
#define BATTERY_DIVIDER_TOP_OHM  1000U
#define BATTERY_DIVIDER_LOW_OHM  4000U
#define BATTERY_FULL_MV           4120U

static ADC_HandleTypeDef *s_adc;
static battery_service_data_t s_data;

static uint8_t battery_service_voltage_to_percent(uint16_t voltage_mv)
{
  if (voltage_mv <= 3300U)
  {
    return 0U;
  }
  if (voltage_mv < 3600U)
  {
    return (uint8_t)(((uint32_t)(voltage_mv - 3300U) * 20U) / 300U);
  }
  if (voltage_mv < 3800U)
  {
    return (uint8_t)(20U + ((uint32_t)(voltage_mv - 3600U) * 40U) / 200U);
  }
  if (voltage_mv < 4000U)
  {
    return (uint8_t)(60U + ((uint32_t)(voltage_mv - 3800U) * 30U) / 200U);
  }
  if (voltage_mv < BATTERY_FULL_MV)
  {
    return (uint8_t)(90U + ((uint32_t)(voltage_mv - 4000U) * 10U) /
                     (BATTERY_FULL_MV - 4000U));
  }
  return 100U;
}

void battery_service_init(ADC_HandleTypeDef *adc)
{
  s_adc = adc;
  s_data.raw = 0U;
  s_data.voltage_mv = 0U;
  s_data.percent = 0U;
  s_data.valid = false;

  if (s_adc != NULL)
  {
    (void)HAL_ADCEx_Calibration_Start(s_adc);
  }
}

bool battery_service_sample(void)
{
  uint32_t total = 0U;
  uint8_t samples = 0U;

  if (s_adc == NULL)
  {
    return false;
  }

  board_battery_sense_enable(true);
  HAL_Delay(2U);

  for (uint8_t index = 0U; index < BATTERY_ADC_SAMPLES; index++)
  {
    if ((HAL_ADC_Start(s_adc) == HAL_OK) &&
        (HAL_ADC_PollForConversion(s_adc,BATTERY_ADC_TIMEOUT_MS) == HAL_OK))
    {
      total += HAL_ADC_GetValue(s_adc);
      samples++;
    }
    (void)HAL_ADC_Stop(s_adc);
  }

  board_battery_sense_enable(false);
  if (samples == 0U)
  {
    s_data.valid = false;
    return false;
  }

  s_data.raw = (uint16_t)(total / samples);
  s_data.voltage_mv = (uint16_t)(((uint32_t)s_data.raw * BATTERY_ADC_REFERENCE_MV *
                                  (BATTERY_DIVIDER_TOP_OHM + BATTERY_DIVIDER_LOW_OHM)) /
                                 (4095U * BATTERY_DIVIDER_LOW_OHM));
  s_data.percent = battery_service_voltage_to_percent(s_data.voltage_mv);
  s_data.valid = true;
  return true;
}

const battery_service_data_t *battery_service_get(void)
{
  return &s_data;
}
