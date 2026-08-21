/**
 * @file    bsp_rtc.h
 * @brief   STM32F103 RTC BSP interface
 *
 * 本模块负责对 STM32F103 内部 RTC 硬件进行基础封装。
 *
 * RTC 使用 32 位硬件计数器保存秒计数，BSP 层统一将其作为
 * Unix Timestamp 使用。
 *
 * 本模块只负责 RTC 硬件访问，不负责年月日、星期等日历转换。
 */

#ifndef BSP_RTC_H
#define BSP_RTC_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

/**
 * @brief BSP RTC 操作状态
 */
typedef enum
{
    BSP_RTC_OK = 0,
    BSP_RTC_ERROR,
    BSP_RTC_TIMEOUT,
    BSP_RTC_INVALID_PARAM
} BspRtcStatus_t;


/**
 * @brief 初始化 RTC BSP
 *
 * CubeMX 生成的 MX_RTC_Init() 应当在调用本函数之前完成。
 * 本函数不会修改当前 RTC 时间，只负责完成 BSP 层需要的同步检查。
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      初始化成功
 * @retval BSP_RTC_ERROR   RTC 初始化状态异常
 * @retval BSP_RTC_TIMEOUT RTC 同步超时
 */
BspRtcStatus_t bsp_rtc_init(void);


/**
 * @brief 获取当前 RTC 时间戳
 *
 * 直接读取 STM32F103 RTC 32 位计数器。
 *
 * @param[out] timestamp 用于保存 Unix Timestamp 的指针
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK            读取成功
 * @retval BSP_RTC_INVALID_PARAM 参数为空
 */
BspRtcStatus_t bsp_rtc_get_timestamp(uint32_t *timestamp);


/**
 * @brief 设置 RTC 时间戳
 *
 * 将 Unix Timestamp 写入 STM32F103 RTC 32 位计数器。
 * 设置成功后同时在 RTC 备份寄存器中写入初始化标志。
 *
 * @param[in] timestamp 要设置的 Unix Timestamp
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      设置成功
 * @retval BSP_RTC_ERROR   设置失败
 * @retval BSP_RTC_TIMEOUT RTC 写入超时
 */
BspRtcStatus_t bsp_rtc_set_timestamp(uint32_t timestamp);


/**
 * @brief 判断 RTC 是否已经设置过有效时间
 *
 * 通过 RTC 备份寄存器中的初始化标志判断。
 *
 * @return bool
 * @retval true  RTC 已经设置过时间
 * @retval false RTC 尚未设置时间
 */
bool bsp_rtc_is_time_valid(void);


/**
 * @brief 清除 RTC 时间有效标志
 *
 * 主要用于调试、恢复出厂设置等场景。
 *
 * @return void
 */
void bsp_rtc_invalidate(void);


#endif /* BSP_RTC_H */