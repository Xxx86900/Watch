#include "board.h"

#include "main.h"

void board_init(void)
{
    /* PB13 是高电平有效的硬件电源锁存控制引脚。 */
    HAL_GPIO_WritePin(PWR_HOLD_GPIO_Port, PWR_HOLD_Pin, GPIO_PIN_SET);

    board_flashlight_set(false);
    board_battery_sense_enable(false);
}

bool board_key_is_pressed(board_key_t key)
{
    GPIO_PinState state;

    switch (key)
    {
        case BOARD_KEY_PREV:
            state = HAL_GPIO_ReadPin(KEY_PREV_GPIO_Port, KEY_PREV_Pin);
            break;

        case BOARD_KEY_NEXT:
            state = HAL_GPIO_ReadPin(KEY_NEXT_GPIO_Port, KEY_NEXT_Pin);
            break;

        case BOARD_KEY_OK:
            state = HAL_GPIO_ReadPin(KEY_OK_GPIO_Port, KEY_OK_Pin);
            break;

        case BOARD_KEY_COUNT:
        default:
            return false;
    }

    return state == GPIO_PIN_RESET;
}

void board_flashlight_set(bool enabled)
{
    /* FLASH_LED_N 为低电平有效。 */
    HAL_GPIO_WritePin(FLASH_LED_N_GPIO_Port,
                      FLASH_LED_N_Pin,
                      enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void board_battery_sense_enable(bool enabled)
{
    /* VBAT_SENSE_EN_N 为低电平有效。 */
    HAL_GPIO_WritePin(VBAT_SENSE_EN_N_GPIO_Port,
                      VBAT_SENSE_EN_N_Pin,
                      enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void board_power_off(void)
{
    board_flashlight_set(false);
    board_battery_sense_enable(false);

    HAL_GPIO_WritePin(PWR_HOLD_GPIO_Port, PWR_HOLD_Pin, GPIO_PIN_RESET);

    /* 即使电池锁存已断开，USB 供电仍可能使 MCU 保持运行。 */
    __disable_irq();
    while (1)
    {
        __WFI();
    }
}
