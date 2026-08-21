/**
 * @file    bsp_i2c.c
 * @brief   Board I2C abstraction layer.
 */

#include "bsp_i2c.h"

#include "i2c.h"

#include <stddef.h>


/**
 * @brief Get STM32 HAL I2C handle from BSP bus identifier.
 *
 * @param bus I2C bus.
 *
 * @return HAL I2C handle, or NULL if the bus is invalid.
 */
static I2C_HandleTypeDef *bsp_i2c_get_handle(BspI2cBus_t bus)
{
    switch (bus)
    {
        case BSP_I2C_BUS_1:
            return &hi2c1;

        case BSP_I2C_BUS_2:
            return &hi2c2;

        default:
            return NULL;
    }
}


/**
 * @brief Convert STM32 HAL status to BSP I2C status.
 */
static BspI2cStatus_t bsp_i2c_convert_status(HAL_StatusTypeDef status)
{
    switch (status)
    {
        case HAL_OK:
            return BSP_I2C_STATUS_OK;

        case HAL_BUSY:
            return BSP_I2C_STATUS_BUSY;

        case HAL_TIMEOUT:
            return BSP_I2C_STATUS_TIMEOUT;

        case HAL_ERROR:
        default:
            return BSP_I2C_STATUS_ERROR;
    }
}


BspI2cStatus_t bsp_i2c_is_device_ready(BspI2cBus_t bus,
                                       uint8_t address_7bit,
                                       uint32_t timeout_ms)
{
    I2C_HandleTypeDef *hi2c = bsp_i2c_get_handle(bus);

    if (hi2c == NULL)
    {
        return BSP_I2C_STATUS_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(
        hi2c,
        (uint16_t)(address_7bit << 1U),
        3U,
        timeout_ms
    );

    return bsp_i2c_convert_status(status);
}


BspI2cStatus_t bsp_i2c_write(BspI2cBus_t bus,
                             uint8_t address_7bit,
                             const uint8_t *data,
                             uint16_t length,
                             uint32_t timeout_ms)
{
    I2C_HandleTypeDef *hi2c = bsp_i2c_get_handle(bus);

    if ((hi2c == NULL) || (data == NULL) || (length == 0U))
    {
        return BSP_I2C_STATUS_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        hi2c,
        (uint16_t)(address_7bit << 1U),
        (uint8_t *)data,
        length,
        timeout_ms
    );

    return bsp_i2c_convert_status(status);
}


BspI2cStatus_t bsp_i2c_mem_write(BspI2cBus_t bus,
                                 uint8_t address_7bit,
                                 uint8_t reg,
                                 const uint8_t *data,
                                 uint16_t length,
                                 uint32_t timeout_ms)
{
    I2C_HandleTypeDef *hi2c = bsp_i2c_get_handle(bus);

    if ((hi2c == NULL) || (data == NULL) || (length == 0U))
    {
        return BSP_I2C_STATUS_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        hi2c,
        (uint16_t)(address_7bit << 1U),
        reg,
        I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)data,
        length,
        timeout_ms
    );

    return bsp_i2c_convert_status(status);
}


BspI2cStatus_t bsp_i2c_mem_read(BspI2cBus_t bus,
                                uint8_t address_7bit,
                                uint8_t reg,
                                uint8_t *data,
                                uint16_t length,
                                uint32_t timeout_ms)
{
    I2C_HandleTypeDef *hi2c = bsp_i2c_get_handle(bus);

    if ((hi2c == NULL) || (data == NULL) || (length == 0U))
    {
        return BSP_I2C_STATUS_INVALID_PARAM;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        hi2c,
        (uint16_t)(address_7bit << 1U),
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        length,
        timeout_ms
    );

    return bsp_i2c_convert_status(status);
}