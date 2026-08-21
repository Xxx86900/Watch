/**
 * @file    bsp_i2c.h
 * @brief   Board I2C abstraction layer.
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Available I2C buses on the board.
 */
typedef enum
{
    BSP_I2C_BUS_1 = 0,
    BSP_I2C_BUS_2
} BspI2cBus_t;

/**
 * @brief BSP I2C operation status.
 */
typedef enum
{
    BSP_I2C_STATUS_OK = 0,
    BSP_I2C_STATUS_ERROR,
    BSP_I2C_STATUS_BUSY,
    BSP_I2C_STATUS_TIMEOUT,
    BSP_I2C_STATUS_INVALID_PARAM
} BspI2cStatus_t;

/**
 * @brief Check whether an I2C device responds.
 *
 * @param bus         I2C bus.
 * @param address_7bit Device 7-bit I2C address.
 * @param timeout_ms  Timeout in milliseconds.
 *
 * @return BSP I2C status.
 */
BspI2cStatus_t bsp_i2c_is_device_ready(BspI2cBus_t bus,
                                       uint8_t address_7bit,
                                       uint32_t timeout_ms);

/**
 * @brief Write data to an I2C device.
 *
 * @param bus          I2C bus.
 * @param address_7bit Device 7-bit I2C address.
 * @param data         Data buffer.
 * @param length       Number of bytes.
 * @param timeout_ms   Timeout in milliseconds.
 *
 * @return BSP I2C status.
 */
BspI2cStatus_t bsp_i2c_write(BspI2cBus_t bus,
                             uint8_t address_7bit,
                             const uint8_t *data,
                             uint16_t length,
                             uint32_t timeout_ms);

/**
 * @brief Write data to an I2C device register.
 *
 * @param bus          I2C bus.
 * @param address_7bit Device 7-bit I2C address.
 * @param reg          8-bit register address.
 * @param data         Data buffer.
 * @param length       Number of bytes.
 * @param timeout_ms   Timeout in milliseconds.
 *
 * @return BSP I2C status.
 */
BspI2cStatus_t bsp_i2c_mem_write(BspI2cBus_t bus,
                                 uint8_t address_7bit,
                                 uint8_t reg,
                                 const uint8_t *data,
                                 uint16_t length,
                                 uint32_t timeout_ms);

/**
 * @brief Read data from an I2C device register.
 *
 * @param bus          I2C bus.
 * @param address_7bit Device 7-bit I2C address.
 * @param reg          8-bit register address.
 * @param data         Destination buffer.
 * @param length       Number of bytes.
 * @param timeout_ms   Timeout in milliseconds.
 *
 * @return BSP I2C status.
 */
BspI2cStatus_t bsp_i2c_mem_read(BspI2cBus_t bus,
                                uint8_t address_7bit,
                                uint8_t reg,
                                uint8_t *data,
                                uint16_t length,
                                uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_I2C_H */