#ifndef _BSP_ADC_H
#define _BSP_ADC_H


#include <stdint.h>


/**
 * @brief 读取ADC原始采样值
 *
 * @return uint16_t ADC数值(0~4095)
 */
uint16_t adc_read_value(void);


/**
 * @brief 将ADC值转换为电压
 *
 * @param adc_value ADC采样值
 *
 * @return uint16_t 电压(mV)
 */
uint16_t adc_convert_to_voltage_mv(uint16_t adc_value);


#endif