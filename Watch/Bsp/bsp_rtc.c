/**
 * @file    bsp_rtc.c
 * @brief   STM32F103 RTC BSP implementation
 *
 * 本模块对 STM32F103 内部 RTC 进行基础硬件封装。
 *
 * STM32F103 的 RTC 本质上维护一个 32 位秒计数器，因此本模块
 * 直接将该计数器作为 Unix Timestamp 保存。
 *
 * 主要功能：
 * 1. 读取 RTC 32 位计数器；
 * 2. 写入 RTC 32 位计数器；
 * 3. 使用备份寄存器记录 RTC 时间是否已经初始化；
 * 4. 向上层屏蔽 STM32 HAL 和 RTC 寄存器操作细节。
 */

#include "bsp_rtc.h"

#include "rtc.h"


/* RTC 初始化标志，保存在备份寄存器中 */
#define BSP_RTC_INIT_FLAG           0x2333U

/* 用于保存 RTC 初始化标志的备份寄存器 */
#define BSP_RTC_BACKUP_REGISTER     RTC_BKP_DR1

/* RTC 寄存器操作超时时间 */
#define BSP_RTC_TIMEOUT_MS          1000U


/**
 * @brief 等待 RTC 完成前一次写操作，并进入配置模式
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      成功进入配置模式
 * @retval BSP_RTC_TIMEOUT 等待 RTC 超时
 */
static BspRtcStatus_t bsp_rtc_enter_write_mode(void)
{
    uint32_t tick_start = HAL_GetTick();

    /*
     * RTOFF = 1 表示 RTC 已完成上一条写操作，
     * 此时才能开始新的 RTC 寄存器写操作。
     */
    while ((hrtc.Instance->CRL & RTC_CRL_RTOFF) == 0U)
    {
        if ((HAL_GetTick() - tick_start) > BSP_RTC_TIMEOUT_MS)
        {
            return BSP_RTC_TIMEOUT;
        }
    }

    /*
     * STM32F1 RTC 修改 CNT、PRL、ALR 等寄存器之前，
     * 需要进入配置模式。
     */
    __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);

    return BSP_RTC_OK;
}


/**
 * @brief 退出 RTC 配置模式，并等待写操作完成
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      RTC 写入完成
 * @retval BSP_RTC_TIMEOUT RTC 写入超时
 */
static BspRtcStatus_t bsp_rtc_exit_write_mode(void)
{
    uint32_t tick_start;

    __HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);

    tick_start = HAL_GetTick();

    while ((hrtc.Instance->CRL & RTC_CRL_RTOFF) == 0U)
    {
        if ((HAL_GetTick() - tick_start) > BSP_RTC_TIMEOUT_MS)
        {
            return BSP_RTC_TIMEOUT;
        }
    }

    return BSP_RTC_OK;
}


/**
 * @brief 原子读取 RTC 32 位计数器
 *
 * STM32F103 的 RTC Counter 被拆分为：
 *
 * CNTH：高 16 位
 * CNTL：低 16 位
 *
 * 读取过程中低 16 位可能发生溢出，因此需要再次读取 CNTH
 * 判断高 16 位是否发生变化。
 *
 * @return uint32_t RTC 当前计数值
 */
static uint32_t bsp_rtc_read_counter(void)
{
    uint16_t high_first;
    uint16_t high_second;
    uint16_t low;

    high_first = (uint16_t)(hrtc.Instance->CNTH & RTC_CNTH_RTC_CNT);
    low = (uint16_t)(hrtc.Instance->CNTL & RTC_CNTL_RTC_CNT);
    high_second = (uint16_t)(hrtc.Instance->CNTH & RTC_CNTH_RTC_CNT);

    /*
     * 如果两次读取 CNTH 的结果不同，说明读取 CNTL 时
     * 低 16 位恰好发生溢出。
     *
     * 此时需要重新读取 CNTL，并使用新的高 16 位。
     */
    if (high_first != high_second)
    {
        low = (uint16_t)(hrtc.Instance->CNTL & RTC_CNTL_RTC_CNT);
        high_first = high_second;
    }

    return ((uint32_t)high_first << 16U) | (uint32_t)low;
}


/**
 * @brief 写入 RTC 32 位计数器
 *
 * @param[in] timestamp 要写入 RTC 的 32 位秒计数
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      写入成功
 * @retval BSP_RTC_TIMEOUT RTC 写入超时
 */
static BspRtcStatus_t bsp_rtc_write_counter(uint32_t timestamp)
{
    BspRtcStatus_t status;

    status = bsp_rtc_enter_write_mode();

    if (status != BSP_RTC_OK)
    {
        return status;
    }

    WRITE_REG(hrtc.Instance->CNTH, timestamp >> 16U);
    WRITE_REG(hrtc.Instance->CNTL, timestamp & RTC_CNTL_RTC_CNT);

    return bsp_rtc_exit_write_mode();
}


/**
 * @brief 初始化 RTC BSP
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      初始化成功
 * @retval BSP_RTC_ERROR   RTC 状态异常
 * @retval BSP_RTC_TIMEOUT RTC 同步超时
 */
BspRtcStatus_t bsp_rtc_init(void)
{
    HAL_StatusTypeDef status = HAL_RTC_WaitForSynchro(&hrtc);

    if (status == HAL_OK)
    {
        return BSP_RTC_OK;
    }

    if (status == HAL_TIMEOUT)
    {
        return BSP_RTC_TIMEOUT;
    }

    return BSP_RTC_ERROR;
}


/**
 * @brief 获取当前 RTC Unix Timestamp
 *
 * @param[out] timestamp Unix Timestamp 输出地址
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK            获取成功
 * @retval BSP_RTC_INVALID_PARAM 参数为空
 */
BspRtcStatus_t bsp_rtc_get_timestamp(uint32_t *timestamp)
{
    if (timestamp == NULL)
    {
        return BSP_RTC_INVALID_PARAM;
    }

    *timestamp = bsp_rtc_read_counter();

    return BSP_RTC_OK;
}


/**
 * @brief 设置 RTC Unix Timestamp
 *
 * @param[in] timestamp Unix Timestamp
 *
 * @return BspRtcStatus_t
 * @retval BSP_RTC_OK      设置成功
 * @retval BSP_RTC_TIMEOUT RTC 写入超时
 */
BspRtcStatus_t bsp_rtc_set_timestamp(uint32_t timestamp)
{
    BspRtcStatus_t status = bsp_rtc_write_counter(timestamp);

    if (status != BSP_RTC_OK)
    {
        return status;
    }

    /*
     * 时间写入成功后记录初始化标志。
     * RTC 后备域不断电时，该标志可以跨 MCU Reset 保存。
     */
    HAL_RTCEx_BKUPWrite(&hrtc, BSP_RTC_BACKUP_REGISTER, BSP_RTC_INIT_FLAG);

    return BSP_RTC_OK;
}


/**
 * @brief 判断 RTC 是否已经初始化
 *
 * @return bool
 * @retval true  RTC 已有有效时间
 * @retval false RTC 尚未设置有效时间
 */
bool bsp_rtc_is_time_valid(void)
{
    uint32_t flag = HAL_RTCEx_BKUPRead(&hrtc, BSP_RTC_BACKUP_REGISTER);

    return flag == BSP_RTC_INIT_FLAG;
}


/**
 * @brief 清除 RTC 时间有效标志
 *
 * @return void
 */
void bsp_rtc_invalidate(void)
{
    HAL_RTCEx_BKUPWrite(&hrtc, BSP_RTC_BACKUP_REGISTER, 0U);
}