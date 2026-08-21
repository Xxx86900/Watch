#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H


#include <stdint.h>
#include <stdbool.h>



/**
 * @brief 获取当前电池电压
 *
 * @return uint16_t 电池电压(mV)
 */
uint16_t battery_service_get_voltage_mv(void);



/**
 * @brief 获取当前电池剩余电量
 *
 * @return uint8_t 电量百分比(0~100)
 */
uint8_t battery_service_get_percent(void);



/**
 * @brief 判断电池是否低电量
 *
 * @return true 电量低
 * @return false 电量正常
 */
bool battery_service_is_low(void);



#endif