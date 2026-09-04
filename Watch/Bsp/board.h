#ifndef WATCH_BSP_BOARD_H
#define WATCH_BSP_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief 手表 PCB 上可用的物理按键。
 *
 * 板上按键均为低电平有效。调用方应使用 board_key_is_pressed()，
 * 不要直接读取 GPIO 引脚。
 */
typedef enum
{
    BOARD_KEY_PREV = 0,
    BOARD_KEY_NEXT,
    BOARD_KEY_OK,
    BOARD_KEY_COUNT
} board_key_t;

/**
 * @brief 在系统时钟初始化前尽早锁定硬件电源。
 *
 * HAL_Init() 完成后立即调用，避免等待 LSE 起振期间松开开机键导致掉电。
 */
void board_power_hold_early(void);

/**
 * @brief 将板级控制的所有输出设置为安全的启动状态。
 *
 * 调用本函数前必须先调用 MX_GPIO_Init()。
 */
void board_init(void);

/**
 * @brief 读取物理按键的瞬时状态。
 * @param key 要读取的按键。
 * @return 按键实际按下时返回 true，否则返回 false。
 */
bool board_key_is_pressed(board_key_t key);

/**
 * @brief 打开或关闭手电筒 LED。
 */
void board_flashlight_set(bool enabled);

/**
 * @brief 接通或断开电池电压采样分压电路。
 *
 * 在进行 ADC 采样前短暂启用分压电路，采样完成后再次关闭，
 * 以降低待机电流。
 */
void board_battery_sense_enable(bool enabled);

/**
 * @brief 释放硬件电源锁存，并停止执行应用程序代码。
 */
void board_power_off(void);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_BSP_BOARD_H */
