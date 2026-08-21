/**
 * @file battery_service.c
 *
 * @brief 电池管理服务
 *
 * 功能:
 *
 * 1. ADC采样值转换为电池电压
 * 2. 根据锂电池放电曲线计算百分比
 * 3. 提供低电量判断
 */


#include "battery_service.h"

#include "bsp_adc.h"



/*
 * PCB电阻分压:

 * VBAT+
 *
 * R14 = 1k
 *
 * ADC
 *
 * R15 = 4k
 *
 * GND
 *
 */


#define BATTERY_R_TOP       10000U

#define BATTERY_R_BOTTOM    2000U



#define BATTERY_LOW_LEVEL   5U



/**
 * @brief 电池放电曲线
 *
 * voltage_mv:
 *      电池电压
 *
 * percent:
 *      对应容量
 */
typedef struct
{
    uint16_t voltage_mv;

    uint8_t percent;

}BatteryCurve_t;



static const BatteryCurve_t battery_curve[] =
{

    {4200,100},

    {4100,90},

    {4000,80},

    {3850,60},

    {3700,40},

    {3500,20},

    {3200,0}

};



/**
 * @brief 获取电池电压
 * @per   void
 * @return uint16_t 电池电压mV
 */
uint16_t battery_service_get_voltage_mv(void)
{

    uint16_t adc_voltage;


    adc_voltage =
        adc_convert_to_voltage_mv(
            adc_read_value()
        );



    uint32_t battery_voltage;


    battery_voltage =
        (uint32_t)adc_voltage
        *
        (BATTERY_R_TOP + BATTERY_R_BOTTOM)
        /
        BATTERY_R_BOTTOM;



    return (uint16_t)battery_voltage;

}





/**
 * @brief 根据电压计算电量
 *
 * 使用线性插值
 *
 * @return uint8_t 电量百分比
 */
uint8_t battery_service_get_percent(void)
{

    uint16_t voltage =
        battery_service_get_voltage_mv();



    uint32_t size =
        sizeof(battery_curve)
        /
        sizeof(battery_curve[0]);

    if(voltage >= battery_curve[0].voltage_mv)
    {
        return 100;
    }



    if(voltage <= battery_curve[size-1].voltage_mv)
    {
        return 0;
    }



    for(uint32_t i=1;i<size;i++)
    {

        if(voltage >= battery_curve[i].voltage_mv)
        {


            uint16_t high_voltage =
                battery_curve[i-1].voltage_mv;


            uint16_t low_voltage =
                battery_curve[i].voltage_mv;



            uint8_t high_percent =
                battery_curve[i-1].percent;


            uint8_t low_percent =
                battery_curve[i].percent;



            uint8_t percent;


            percent =
                high_percent
                -
                (
                    (high_voltage-voltage)
                    *
                    (high_percent-low_percent)
                    /
                    (high_voltage-low_voltage)
                );


            return percent;

        }

    }


    return 0;

}




/**
 * @brief 判断低电量
 *
 * @return true 电量低
 * @return false 正常
 */
bool battery_service_is_low(void)
{

    return battery_service_get_percent()
            <= BATTERY_LOW_LEVEL;

}