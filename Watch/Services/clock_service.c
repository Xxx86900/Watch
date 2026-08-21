/**
 * @file    clock_service.c
 * @brief   Watch clock service implementation
 *
 * 本模块位于 BSP 与应用层之间，负责将 RTC 保存的秒计数
 * 转换为手表实际使用的年月日、星期、时分秒。
 *
 * 分层关系：
 *
 * STM32 RTC
 *     ↓
 * bsp_rtc
 *     ↓
 * clock_service
 *     ↓
 * UI / Alarm / Application
 *
 * clock_service 不直接操作 RTC 寄存器或 HAL RTC API。
 */

#include "clock_service.h"

#include "bsp_rtc.h"

#include <stddef.h>


#define CLOCK_SECONDS_PER_MINUTE     60U
#define CLOCK_SECONDS_PER_HOUR       3600U
#define CLOCK_SECONDS_PER_DAY        86400U

/*
 * 当前手表第一版只考虑正常产品生命周期，
 * 将有效日期范围限制在 2000~2099 年。
 *
 * 这样可以避免无意义的历史日期，同时也完全覆盖
 * STM32F103 RTC 32 位秒计数器的实际使用需求。
 */
#define CLOCK_MIN_YEAR               2000U
#define CLOCK_MAX_YEAR               2099U


/**
 * @brief 普通年份每个月的天数
 */
static const uint8_t s_days_in_month[12] =
{
    31U, 28U, 31U, 30U,
    31U, 30U, 31U, 31U,
    30U, 31U, 30U, 31U
};


/**
 * @brief 计算从 1970-01-01 到指定年份开始经过的总天数
 *
 * 例如：
 *
 * year = 1970 -> 0
 * year = 1971 -> 365
 *
 * @param[in] year 目标年份
 *
 * @return uint32_t 从1970年开始经过的总天数
 */
static uint32_t clock_days_before_year(uint16_t year)
{
    uint32_t days = 0U;

    for (uint16_t current_year = 1970U;
         current_year < year;
         ++current_year)
    {
        days += clock_service_is_leap_year(current_year) ? 366U : 365U;
    }

    return days;
}


/**
 * @brief 计算指定年份中，目标月份之前已经经过的天数
 *
 * 例如：
 *
 * month = 1 -> 0
 * month = 2 -> 31
 * month = 3 -> 59 / 60
 *
 * @param[in] year  年份
 * @param[in] month 月份
 *
 * @return uint16_t 当前年份中已经经过的天数
 */
static uint16_t clock_days_before_month(uint16_t year, uint8_t month)
{
    uint16_t days = 0U;

    for (uint8_t current_month = 1U;
         current_month < month;
         ++current_month)
    {
        days += clock_service_get_days_in_month(year, current_month);
    }

    return days;
}


/**
 * @brief 将日期时间转换为 Epoch 秒计数
 *
 * 当前第一版不处理时区和夏令时。
 *
 * @param[in] datetime 日期时间
 *
 * @return uint32_t 从1970-01-01 00:00:00开始经过的秒数
 */
static uint32_t clock_datetime_to_timestamp(const ClockDateTime_t *datetime)
{
    uint32_t days;

    days = clock_days_before_year(datetime->year);

    days += clock_days_before_month(datetime->year, datetime->month);

    /*
     * day = 1 表示该月第0天已经过去，
     * 因此这里必须减1。
     */
    days += (uint32_t)(datetime->day - 1U);

    return
        days * CLOCK_SECONDS_PER_DAY +
        (uint32_t)datetime->hour * CLOCK_SECONDS_PER_HOUR +
        (uint32_t)datetime->minute * CLOCK_SECONDS_PER_MINUTE +
        (uint32_t)datetime->second;
}


/**
 * @brief 根据总天数计算星期
 *
 * Unix Epoch:
 *
 * 1970-01-01 = Thursday
 *
 * 本工程定义：
 *
 * Monday    = 1
 * Tuesday   = 2
 * Wednesday = 3
 * Thursday  = 4
 * Friday    = 5
 * Saturday  = 6
 * Sunday    = 7
 *
 * @param[in] days_since_epoch 从1970-01-01开始经过的天数
 *
 * @return ClockWeekday_t 星期
 */
static ClockWeekday_t clock_calculate_weekday(uint32_t days_since_epoch)
{
    /*
     * 1970-01-01 为星期四。
     *
     * Monday = 1 时，Thursday = 4，
     * 所以需要先偏移3天。
     */
    return (ClockWeekday_t)(((days_since_epoch + 3U) % 7U) + 1U);
}


/**
 * @brief 将 Epoch 秒计数转换为日期时间
 *
 * @param[in]  timestamp RTC秒计数
 * @param[out] datetime  转换后的日期时间
 *
 * @return void
 */
static void clock_timestamp_to_datetime(uint32_t timestamp,
                                        ClockDateTime_t *datetime)
{
    uint32_t days;
    uint32_t seconds_of_day;

    uint16_t year;
    uint8_t month;
    uint16_t days_in_year;
    uint8_t days_in_month;

    days = timestamp / CLOCK_SECONDS_PER_DAY;

    seconds_of_day = timestamp % CLOCK_SECONDS_PER_DAY;

    /*
     * 首先处理时分秒。
     */
    datetime->hour =
        (uint8_t)(seconds_of_day / CLOCK_SECONDS_PER_HOUR);

    seconds_of_day %= CLOCK_SECONDS_PER_HOUR;

    datetime->minute =
        (uint8_t)(seconds_of_day / CLOCK_SECONDS_PER_MINUTE);

    datetime->second =
        (uint8_t)(seconds_of_day % CLOCK_SECONDS_PER_MINUTE);


    /*
     * 星期可以直接根据自1970年以来经过的总天数计算。
     */
    datetime->weekday = clock_calculate_weekday(days);


    /*
     * 然后逐年扣除天数，得到当前年份。
     */
    year = 1970U;

    while (1)
    {
        days_in_year =
            clock_service_is_leap_year(year) ? 366U : 365U;

        if (days < days_in_year)
        {
            break;
        }

        days -= days_in_year;
        ++year;
    }

    datetime->year = year;


    /*
     * 再逐月扣除天数，得到当前月份。
     */
    month = 1U;

    while (month <= 12U)
    {
        days_in_month =
            clock_service_get_days_in_month(year, month);

        if (days < days_in_month)
        {
            break;
        }

        days -= days_in_month;
        ++month;
    }

    datetime->month = month;

    /*
     * days 从0开始，因此最终日期需要 +1。
     */
    datetime->day = (uint8_t)(days + 1U);
}


/**
 * @brief 初始化时钟服务
 *
 * @return ClockServiceStatus_t
 */
ClockServiceStatus_t clock_service_init(void)
{
    BspRtcStatus_t status = bsp_rtc_init();

    if (status != BSP_RTC_OK)
    {
        return CLOCK_SERVICE_ERROR;
    }

    /*
     * 不自动伪造默认时间。
     *
     * RTC首次启动时，应由 UI 或应用层要求用户设置时间。
     */
    if (!bsp_rtc_is_time_valid())
    {
        return CLOCK_SERVICE_TIME_NOT_SET;
    }

    return CLOCK_SERVICE_OK;
}


/**
 * @brief 获取当前日期时间
 *
 * @param[out] datetime 日期时间输出
 *
 * @return ClockServiceStatus_t
 */
ClockServiceStatus_t clock_service_get_datetime(ClockDateTime_t *datetime)
{
    uint32_t timestamp;

    if (datetime == NULL)
    {
        return CLOCK_SERVICE_INVALID_PARAM;
    }

    if (!bsp_rtc_is_time_valid())
    {
        return CLOCK_SERVICE_TIME_NOT_SET;
    }

    if (bsp_rtc_get_timestamp(&timestamp) != BSP_RTC_OK)
    {
        return CLOCK_SERVICE_ERROR;
    }

    clock_timestamp_to_datetime(timestamp, datetime);

    /*
     * 防止 RTC 中出现超出当前产品支持范围的异常值。
     */
    if (datetime->year < CLOCK_MIN_YEAR ||
        datetime->year > CLOCK_MAX_YEAR)
    {
        return CLOCK_SERVICE_ERROR;
    }

    return CLOCK_SERVICE_OK;
}


/**
 * @brief 设置当前日期时间
 *
 * @param[in] datetime 日期时间
 *
 * @return ClockServiceStatus_t
 */
ClockServiceStatus_t clock_service_set_datetime(const ClockDateTime_t *datetime)
{
    uint32_t timestamp;

    if (datetime == NULL)
    {
        return CLOCK_SERVICE_INVALID_PARAM;
    }

    if (!clock_service_is_datetime_valid(datetime))
    {
        return CLOCK_SERVICE_INVALID_DATETIME;
    }

    timestamp = clock_datetime_to_timestamp(datetime);

    if (bsp_rtc_set_timestamp(timestamp) != BSP_RTC_OK)
    {
        return CLOCK_SERVICE_ERROR;
    }

    return CLOCK_SERVICE_OK;
}


/**
 * @brief 判断指定年份是否为闰年
 *
 * @param[in] year 年份
 *
 * @return bool
 */
bool clock_service_is_leap_year(uint16_t year)
{
    if ((year % 400U) == 0U)
    {
        return true;
    }

    if ((year % 100U) == 0U)
    {
        return false;
    }

    return (year % 4U) == 0U;
}


/**
 * @brief 获取指定月份的天数
 *
 * @param[in] year  年份
 * @param[in] month 月份
 *
 * @return uint8_t 月份天数，非法月份返回0
 */
uint8_t clock_service_get_days_in_month(uint16_t year, uint8_t month)
{
    uint8_t days;

    if (month < 1U || month > 12U)
    {
        return 0U;
    }

    days = s_days_in_month[month - 1U];

    if (month == 2U && clock_service_is_leap_year(year))
    {
        days = 29U;
    }

    return days;
}


/**
 * @brief 检查日期时间是否合法
 *
 * @param[in] datetime 日期时间
 *
 * @return bool
 */
bool clock_service_is_datetime_valid(const ClockDateTime_t *datetime)
{
    uint8_t max_day;

    if (datetime == NULL)
    {
        return false;
    }

    if (datetime->year < CLOCK_MIN_YEAR ||
        datetime->year > CLOCK_MAX_YEAR)
    {
        return false;
    }

    if (datetime->month < 1U || datetime->month > 12U)
    {
        return false;
    }

    max_day =
        clock_service_get_days_in_month(datetime->year,
                                        datetime->month);

    if (datetime->day < 1U || datetime->day > max_day)
    {
        return false;
    }

    if (datetime->hour > 23U)
    {
        return false;
    }

    if (datetime->minute > 59U)
    {
        return false;
    }

    if (datetime->second > 59U)
    {
        return false;
    }

    return true;
}