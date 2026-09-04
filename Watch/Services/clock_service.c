#include "clock_service.h"

#include <stddef.h>

#define CLOCK_BACKUP_DATE_REGISTER RTC_BKP_DR2
#define CLOCK_PACK_DATE(year,month,day) \
  ((((uint32_t)((year) - 2000U) & 0x7FU) << 9U) | \
   (((uint32_t)(month) & 0x0FU) << 5U) | ((uint32_t)(day) & 0x1FU))

static RTC_HandleTypeDef *s_rtc;
static uint32_t s_backup_date;

static bool clock_service_is_leap_year(uint16_t year)
{
  return ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

static uint8_t clock_service_days_in_month(uint16_t year,uint8_t month)
{
  static const uint8_t days[] = {31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U};

  if ((month < 1U) || (month > 12U))
  {
    return 0U;
  }
  if ((month == 2U) && clock_service_is_leap_year(year))
  {
    return 29U;
  }
  return days[month - 1U];
}

static uint8_t clock_service_weekday(uint16_t year,uint8_t month,uint8_t day)
{
  static const uint8_t offsets[] = {0U,3U,2U,5U,0U,3U,5U,1U,4U,6U,2U,4U};
  uint16_t adjusted_year = year;
  uint8_t weekday;

  if (month < 3U)
  {
    adjusted_year--;
  }
  weekday = (uint8_t)((adjusted_year + adjusted_year / 4U - adjusted_year / 100U +
                       adjusted_year / 400U + offsets[month - 1U] + day) % 7U);
  return (weekday == 0U) ? RTC_WEEKDAY_SUNDAY : weekday;
}

void clock_service_init(RTC_HandleTypeDef *rtc)
{
  s_rtc = rtc;
  s_backup_date = (rtc != NULL) ?
                  HAL_RTCEx_BKUPRead(rtc,CLOCK_BACKUP_DATE_REGISTER) : 0U;
}

bool clock_service_get(clock_service_time_t *time)
{
  RTC_TimeTypeDef rtc_time = {0};
  RTC_DateTypeDef rtc_date = {0};

  if ((s_rtc == NULL) || (time == NULL) ||
      (HAL_RTC_GetTime(s_rtc,&rtc_time,RTC_FORMAT_BIN) != HAL_OK) ||
      (HAL_RTC_GetDate(s_rtc,&rtc_date,RTC_FORMAT_BIN) != HAL_OK))
  {
    return false;
  }

  time->year = (uint16_t)(2000U + rtc_date.Year);
  time->month = rtc_date.Month;
  time->day = rtc_date.Date;
  time->weekday = rtc_date.WeekDay;
  time->hour = rtc_time.Hours;
  time->minute = rtc_time.Minutes;
  time->second = rtc_time.Seconds;

  {
    uint32_t packed_date = CLOCK_PACK_DATE(time->year,time->month,time->day);
    if (packed_date != s_backup_date)
    {
      HAL_RTCEx_BKUPWrite(s_rtc,CLOCK_BACKUP_DATE_REGISTER,packed_date);
      s_backup_date = packed_date;
    }
  }
  return true;
}

bool clock_service_set(const clock_service_time_t *time)
{
  RTC_TimeTypeDef rtc_time = {0};
  RTC_DateTypeDef rtc_date = {0};

  if ((s_rtc == NULL) || (time == NULL) ||
      (time->year < 2000U) || (time->year > 2099U) ||
      (time->month < 1U) || (time->month > 12U) ||
      (time->day < 1U) ||
      (time->day > clock_service_days_in_month(time->year,time->month)) ||
      (time->hour > 23U) || (time->minute > 59U) || (time->second > 59U))
  {
    return false;
  }

  rtc_time.Hours = time->hour;
  rtc_time.Minutes = time->minute;
  rtc_time.Seconds = time->second;

  rtc_date.Year = (uint8_t)(time->year - 2000U);
  rtc_date.Month = time->month;
  rtc_date.Date = time->day;
  rtc_date.WeekDay = clock_service_weekday(time->year,time->month,time->day);

  if ((HAL_RTC_SetTime(s_rtc,&rtc_time,RTC_FORMAT_BIN) != HAL_OK) ||
      (HAL_RTC_SetDate(s_rtc,&rtc_date,RTC_FORMAT_BIN) != HAL_OK))
  {
    return false;
  }

  s_backup_date = CLOCK_PACK_DATE(time->year,time->month,time->day);
  HAL_RTCEx_BKUPWrite(s_rtc,CLOCK_BACKUP_DATE_REGISTER,s_backup_date);
  return true;
}
