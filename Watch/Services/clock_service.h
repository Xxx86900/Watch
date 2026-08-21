/**
 * @file    clock_service.h
 * @brief   Watch clock service interface
 *
 * 本模块负责手表时间与日期相关的业务逻辑。
 *
 * 主要功能：
 * 1. 获取当前年月日、星期、时分秒；
 * 2. 设置当前日期和时间；
 * 3. Unix Epoch 秒计数与日历时间之间的转换；
 * 4. 日期合法性、闰年及星期计算。
 *
 * 本模块不直接访问 STM32 RTC 硬件，
 * RTC 硬件访问统一由 bsp_rtc 模块负责。
 */

#ifndef CLOCK_SERVICE_H
#define CLOCK_SERVICE_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Clock service operation status
 */
typedef enum
{
    CLOCK_SERVICE_OK = 0,
    CLOCK_SERVICE_ERROR,
    CLOCK_SERVICE_INVALID_PARAM,
    CLOCK_SERVICE_INVALID_DATETIME,
    CLOCK_SERVICE_TIME_NOT_SET
} ClockServiceStatus_t;


/**
 * @brief Weekday definition
 *
 * 使用 ISO 风格：
 * Monday = 1
 * ...
 * Sunday = 7
 */
typedef enum
{
    CLOCK_WEEKDAY_MONDAY = 1,
    CLOCK_WEEKDAY_TUESDAY,
    CLOCK_WEEKDAY_WEDNESDAY,
    CLOCK_WEEKDAY_THURSDAY,
    CLOCK_WEEKDAY_FRIDAY,
    CLOCK_WEEKDAY_SATURDAY,
    CLOCK_WEEKDAY_SUNDAY
} ClockWeekday_t;


/**
 * @brief Date and time structure
 *
 * weekday 在读取时间时由 clock_service 自动计算。
 * 设置时间时不需要填写 weekday。
 */
typedef struct
{
    uint16_t year;

    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    ClockWeekday_t weekday;
} ClockDateTime_t;


/**
 * @brief 初始化时钟服务
 *
 * 调用本函数之前，应先由 CubeMX 完成 MX_RTC_Init()。
 *
 * 本函数会初始化 BSP RTC，并检查 RTC 是否已经具有有效时间。
 * 如果 RTC 尚未设置过时间，则返回 CLOCK_SERVICE_TIME_NOT_SET，
 * 不会自动写入一个假的默认时间。
 *
 * @return ClockServiceStatus_t
 * @retval CLOCK_SERVICE_OK           RTC 时间有效
 * @retval CLOCK_SERVICE_TIME_NOT_SET RTC 尚未设置时间
 * @retval CLOCK_SERVICE_ERROR        RTC 初始化失败
 */
ClockServiceStatus_t clock_service_init(void);


/**
 * @brief 获取当前日期和时间
 *
 * 从 RTC 获取当前秒计数，并转换成年、月、日、星期、
 * 时、分、秒。
 *
 * @param[out] datetime 日期时间输出结构体
 *
 * @return ClockServiceStatus_t
 * @retval CLOCK_SERVICE_OK            获取成功
 * @retval CLOCK_SERVICE_INVALID_PARAM 参数为空
 * @retval CLOCK_SERVICE_TIME_NOT_SET  RTC 尚未设置时间
 * @retval CLOCK_SERVICE_ERROR         RTC读取失败
 */
ClockServiceStatus_t clock_service_get_datetime(ClockDateTime_t *datetime);


/**
 * @brief 设置当前日期和时间
 *
 * 本函数会先检查日期是否合法，然后将年月日时分秒
 * 转换为秒计数并写入 RTC。
 *
 * weekday 字段不参与设置，由系统根据日期自动计算。
 *
 * @param[in] datetime 要设置的日期和时间
 *
 * @return ClockServiceStatus_t
 * @retval CLOCK_SERVICE_OK               设置成功
 * @retval CLOCK_SERVICE_INVALID_PARAM    参数为空
 * @retval CLOCK_SERVICE_INVALID_DATETIME 日期或时间非法
 * @retval CLOCK_SERVICE_ERROR            RTC写入失败
 */
ClockServiceStatus_t clock_service_set_datetime(const ClockDateTime_t *datetime);


/**
 * @brief 判断指定年份是否为闰年
 *
 * Gregorian calendar rule:
 * - 能被400整除：闰年
 * - 能被100整除：平年
 * - 能被4整除：闰年
 *
 * @param[in] year 年份
 *
 * @return bool
 * @retval true  闰年
 * @retval false 平年
 */
bool clock_service_is_leap_year(uint16_t year);


/**
 * @brief 获取指定年月的天数
 *
 * @param[in] year  年份
 * @param[in] month 月份，范围1~12
 *
 * @return uint8_t
 * @retval 28~31 对应月份天数
 * @retval 0     月份非法
 */
uint8_t clock_service_get_days_in_month(uint16_t year, uint8_t month);


/**
 * @brief 检查日期时间是否合法
 *
 * 当前版本支持 2000~2099 年。
 *
 * @param[in] datetime 日期时间
 *
 * @return bool
 * @retval true  日期时间合法
 * @retval false 日期时间非法
 */
bool clock_service_is_datetime_valid(const ClockDateTime_t *datetime);


#endif /* CLOCK_SERVICE_H */