#include "bsp_adc.h"

#include "main.h"


extern ADC_HandleTypeDef hadc1;



#define ADC_MAX_VALUE       4095U

#define ADC_REFERENCE_MV    3300U



/**
 * @brief 读取ADC转换结果
 *
 * @return uint16_t ADC原始值
 */
uint16_t adc_read_value(void)
{

    uint16_t value = 0;


    HAL_ADC_Start(&hadc1);


    if(HAL_ADC_PollForConversion(&hadc1,10)==HAL_OK)
    {
        value = HAL_ADC_GetValue(&hadc1);
    }


    HAL_ADC_Stop(&hadc1);


    return value;

}



/**
 * @brief ADC值转换为引脚电压
 *
 * 计算:
 *
 * V = ADC / 4095 * 3300
 *
 * @param adc_value ADC值
 *
 * @return uint16_t 电压(mV)
 */
uint16_t adc_convert_to_voltage_mv(uint16_t adc_value)
{

    uint32_t voltage;


    voltage =
        ((uint32_t)adc_value *
        ADC_REFERENCE_MV)
        /
        ADC_MAX_VALUE;


    return (uint16_t)voltage;

}