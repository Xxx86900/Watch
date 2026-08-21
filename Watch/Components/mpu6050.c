/**
 * @file    mpu6050.c
 * @brief   MPU6050 六轴惯性传感器驱动实现。
 *
 * 本文件实现 MPU6050 的寄存器配置、设备检测、原始数据读取
 * 以及物理量转换。
 *
 * MPU6050 通过板级 BSP I2C 接口访问，不直接依赖具体的
 * STM32 I2C 外设实例和 HAL I2C API。
 *
 * 当前硬件连接：
 * - MPU6050 挂载于 I2C2；
 * - 7 位从机地址为 0x68。
 */

#include "mpu6050.h"

#include "bsp_i2c.h"
#include "main.h"

#include <stddef.h>


/* ============================================================================
 * MPU6050 总线配置
 * ========================================================================== */

/**
 * @brief MPU6050 所连接的板级 I2C 总线。
 */
#define MPU6050_I2C_BUS                    BSP_I2C_BUS_2

/**
 * @brief MPU6050 7 位 I2C 从机地址。
 *
 * AD0 为低电平时，MPU6050 的 7 位地址为 0x68。
 *
 * 地址左移等 STM32 HAL 相关处理由 bsp_i2c 层负责，
 * Component 层始终使用标准 7 位 I2C 地址。
 */
#define MPU6050_I2C_ADDRESS                0x68U

/**
 * @brief MPU6050 I2C 操作超时时间，单位 ms。
 */
#define MPU6050_I2C_TIMEOUT_MS             100U


/* ============================================================================
 * MPU6050 寄存器定义
 * ========================================================================== */

#define MPU6050_REG_SMPLRT_DIV             0x19U
#define MPU6050_REG_CONFIG                 0x1AU
#define MPU6050_REG_GYRO_CONFIG            0x1BU
#define MPU6050_REG_ACCEL_CONFIG           0x1CU

#define MPU6050_REG_ACCEL_XOUT_H           0x3BU

#define MPU6050_REG_PWR_MGMT_1             0x6BU
#define MPU6050_REG_PWR_MGMT_2             0x6CU

#define MPU6050_REG_WHO_AM_I               0x75U


/* ============================================================================
 * MPU6050 参数配置
 * ========================================================================== */

/**
 * @brief MPU6050 标准 WHO_AM_I 值。
 */
#define MPU6050_WHO_AM_I_VALUE             0x68U


/**
 * @brief CONFIG 寄存器配置值。
 *
 * DLPF_CFG = 3：
 * - 陀螺仪带宽约 42 Hz；
 * - 加速度计带宽约 44 Hz。
 *
 * 对手表抬腕、姿态判断和普通人体动作检测而言，
 * 该带宽能够有效抑制高频噪声，同时保持足够的动态响应。
 */
#define MPU6050_CONFIG_DLPF                 0x03U


/**
 * @brief 采样率分频值。
 *
 * DLPF 开启后，陀螺仪内部输出频率为 1000 Hz。
 *
 * SampleRate = 1000 / (1 + SMPLRT_DIV)
 *
 * 当 SMPLRT_DIV = 9 时：
 *
 * SampleRate = 1000 / 10 = 100 Hz
 */
#define MPU6050_SAMPLE_RATE_DIV             0x09U


/**
 * @brief 陀螺仪量程配置。
 *
 * FS_SEL = 1：
 * - 量程：±500 degree/s；
 * - 灵敏度：65.5 LSB/(degree/s)。
 */
#define MPU6050_GYRO_CONFIG_VALUE           0x08U
#define MPU6050_GYRO_SENSITIVITY            65.5f


/**
 * @brief 加速度计量程配置。
 *
 * AFS_SEL = 1：
 * - 量程：±4 g；
 * - 灵敏度：8192 LSB/g。
 */
#define MPU6050_ACCEL_CONFIG_VALUE          0x08U
#define MPU6050_ACCEL_SENSITIVITY           8192.0f


/**
 * @brief MPU6050 软件复位后的等待时间。
 */
#define MPU6050_RESET_DELAY_MS              100U


/**
 * @brief MPU6050 唤醒后的稳定等待时间。
 */
#define MPU6050_WAKE_DELAY_MS               10U


/* ============================================================================
 * 私有函数
 * ========================================================================== */

/**
 * @brief 向 MPU6050 单个寄存器写入一个字节。
 *
 * @param reg   MPU6050 寄存器地址。
 * @param value 要写入的寄存器值。
 *
 * @return MPU6050_STATUS_OK 写入成功；
 *         MPU6050_STATUS_ERROR_I2C I2C 通信失败。
 */
static Mpu6050Status_t mpu6050_write_reg(uint8_t reg, uint8_t value)
{
    BspI2cStatus_t status = bsp_i2c_mem_write(
        MPU6050_I2C_BUS,
        MPU6050_I2C_ADDRESS,
        reg,
        &value,
        1U,
        MPU6050_I2C_TIMEOUT_MS
    );

    if (status != BSP_I2C_STATUS_OK)
    {
        return MPU6050_STATUS_ERROR_I2C;
    }

    return MPU6050_STATUS_OK;
}


/**
 * @brief 从 MPU6050 单个寄存器读取一个字节。
 *
 * @param reg   MPU6050 寄存器地址。
 * @param value 用于保存读取结果。
 *
 * @return MPU6050_STATUS_OK 读取成功；
 *         MPU6050_STATUS_ERROR_PARAM 参数无效；
 *         MPU6050_STATUS_ERROR_I2C I2C 通信失败。
 */
static Mpu6050Status_t mpu6050_read_reg(uint8_t reg, uint8_t *value)
{
    if (value == NULL)
    {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    BspI2cStatus_t status = bsp_i2c_mem_read(
        MPU6050_I2C_BUS,
        MPU6050_I2C_ADDRESS,
        reg,
        value,
        1U,
        MPU6050_I2C_TIMEOUT_MS
    );

    if (status != BSP_I2C_STATUS_OK)
    {
        return MPU6050_STATUS_ERROR_I2C;
    }

    return MPU6050_STATUS_OK;
}


/**
 * @brief 从 MPU6050 连续读取多个寄存器。
 *
 * MPU6050 支持寄存器地址自动递增，因此可以从指定起始寄存器
 * 连续读取多个字节，减少 I2C 事务数量。
 *
 * @param start_reg 起始寄存器地址。
 * @param data      用于保存读取结果的缓冲区。
 * @param length    需要读取的字节数。
 *
 * @return MPU6050_STATUS_OK 读取成功；
 *         MPU6050_STATUS_ERROR_PARAM 参数无效；
 *         MPU6050_STATUS_ERROR_I2C I2C 通信失败。
 */
static Mpu6050Status_t mpu6050_read_regs(uint8_t start_reg,
                                         uint8_t *data,
                                         uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    BspI2cStatus_t status = bsp_i2c_mem_read(
        MPU6050_I2C_BUS,
        MPU6050_I2C_ADDRESS,
        start_reg,
        data,
        length,
        MPU6050_I2C_TIMEOUT_MS
    );

    if (status != BSP_I2C_STATUS_OK)
    {
        return MPU6050_STATUS_ERROR_I2C;
    }

    return MPU6050_STATUS_OK;
}


/**
 * @brief 将 MPU6050 的两个大端字节转换为 int16_t。
 *
 * MPU6050 的轴数据采用高字节在前、低字节在后的存储方式。
 *
 * @param msb 高字节。
 * @param lsb 低字节。
 *
 * @return 合并后的有符号 16 位数据。
 */
static int16_t mpu6050_be16_to_i16(uint8_t msb, uint8_t lsb)
{
    return (int16_t)(((uint16_t)msb << 8U) | (uint16_t)lsb);
}


/* ============================================================================
 * 公共接口
 * ========================================================================== */

Mpu6050Status_t mpu6050_read_id(uint8_t *id)
{
    return mpu6050_read_reg(MPU6050_REG_WHO_AM_I, id);
}


bool mpu6050_is_connected(void)
{
    uint8_t id = 0U;

    if (mpu6050_read_id(&id) != MPU6050_STATUS_OK)
    {
        return false;
    }

    return id == MPU6050_WHO_AM_I_VALUE;
}


Mpu6050Status_t mpu6050_init(void)
{
    Mpu6050Status_t status;
    uint8_t id = 0U;

    /**
     * @brief 确认 MPU6050 在 I2C2 总线上能够正常应答。
     */
    if (bsp_i2c_is_device_ready(
            MPU6050_I2C_BUS,
            MPU6050_I2C_ADDRESS,
            MPU6050_I2C_TIMEOUT_MS) != BSP_I2C_STATUS_OK)
    {
        return MPU6050_STATUS_ERROR_I2C;
    }

    /**
     * @brief 对 MPU6050 执行软件复位。
     *
     * PWR_MGMT_1：
     * DEVICE_RESET = 1。
     */
    status = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x80U);
    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    HAL_Delay(MPU6050_RESET_DELAY_MS);

    /**
     * @brief 唤醒 MPU6050，并选择 X 轴陀螺仪 PLL 作为系统时钟。
     *
     * PWR_MGMT_1：
     * SLEEP  = 0；
     * CLKSEL = 001。
     */
    status = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x01U);
    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    HAL_Delay(MPU6050_WAKE_DELAY_MS);

    /**
     * @brief 使能全部三轴加速度计和三轴陀螺仪。
     *
     * PWR_MGMT_2 = 0x00 表示所有轴均正常工作。
     */
    status = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_2, 0x00U);
    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    /**
     * @brief 读取 WHO_AM_I，确认当前设备确实为 MPU6050。
     */
    status = mpu6050_read_id(&id);
    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    if (id != MPU6050_WHO_AM_I_VALUE)
    {
        return MPU6050_STATUS_ERROR_ID;
    }

    /**
     * @brief 配置数字低通滤波器。
     */
    status = mpu6050_write_reg(
        MPU6050_REG_CONFIG,
        MPU6050_CONFIG_DLPF
    );

    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    /**
     * @brief 配置 MPU6050 输出采样率为 100 Hz。
     */
    status = mpu6050_write_reg(
        MPU6050_REG_SMPLRT_DIV,
        MPU6050_SAMPLE_RATE_DIV
    );

    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    /**
     * @brief 配置陀螺仪量程为 ±500 degree/s。
     */
    status = mpu6050_write_reg(
        MPU6050_REG_GYRO_CONFIG,
        MPU6050_GYRO_CONFIG_VALUE
    );

    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    /**
     * @brief 配置加速度计量程为 ±4 g。
     */
    status = mpu6050_write_reg(
        MPU6050_REG_ACCEL_CONFIG,
        MPU6050_ACCEL_CONFIG_VALUE
    );

    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    return MPU6050_STATUS_OK;
}


Mpu6050Status_t mpu6050_read_raw(Mpu6050RawData_t *data)
{
    if (data == NULL)
    {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    /*
     * 从 ACCEL_XOUT_H 开始连续读取 14 字节。
     *
     * buffer[0:1]   ACCEL_X
     * buffer[2:3]   ACCEL_Y
     * buffer[4:5]   ACCEL_Z
     * buffer[6:7]   TEMP
     * buffer[8:9]   GYRO_X
     * buffer[10:11] GYRO_Y
     * buffer[12:13] GYRO_Z
     */
    uint8_t buffer[14];

    Mpu6050Status_t status = mpu6050_read_regs(
        MPU6050_REG_ACCEL_XOUT_H,
        buffer,
        (uint16_t)sizeof(buffer)
    );

    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    data->accel_x = mpu6050_be16_to_i16(buffer[0], buffer[1]);
    data->accel_y = mpu6050_be16_to_i16(buffer[2], buffer[3]);
    data->accel_z = mpu6050_be16_to_i16(buffer[4], buffer[5]);

    data->temperature = mpu6050_be16_to_i16(buffer[6], buffer[7]);

    data->gyro_x = mpu6050_be16_to_i16(buffer[8], buffer[9]);
    data->gyro_y = mpu6050_be16_to_i16(buffer[10], buffer[11]);
    data->gyro_z = mpu6050_be16_to_i16(buffer[12], buffer[13]);

    return MPU6050_STATUS_OK;
}


Mpu6050Status_t mpu6050_read(Mpu6050Data_t *data)
{
    if (data == NULL)
    {
        return MPU6050_STATUS_ERROR_PARAM;
    }

    Mpu6050RawData_t raw;

    Mpu6050Status_t status = mpu6050_read_raw(&raw);
    if (status != MPU6050_STATUS_OK)
    {
        return status;
    }

    /**
     * @brief 将加速度计原始值转换为 g。
     *
     * 当前配置为 ±4 g：
     * 8192 LSB = 1 g。
     */
    data->accel_x_g = (float)raw.accel_x / MPU6050_ACCEL_SENSITIVITY;
    data->accel_y_g = (float)raw.accel_y / MPU6050_ACCEL_SENSITIVITY;
    data->accel_z_g = (float)raw.accel_z / MPU6050_ACCEL_SENSITIVITY;

    /**
     * @brief 将温度原始值转换为摄氏度。
     *
     * MPU6050 温度换算关系：
     * Temperature = TEMP_OUT / 340 + 36.53。
     */
    data->temperature_c = ((float)raw.temperature / 340.0f) + 36.53f;

    /**
     * @brief 将陀螺仪原始值转换为 degree/s。
     *
     * 当前配置为 ±500 degree/s：
     * 65.5 LSB = 1 degree/s。
     */
    data->gyro_x_dps = (float)raw.gyro_x / MPU6050_GYRO_SENSITIVITY;
    data->gyro_y_dps = (float)raw.gyro_y / MPU6050_GYRO_SENSITIVITY;
    data->gyro_z_dps = (float)raw.gyro_z / MPU6050_GYRO_SENSITIVITY;

    return MPU6050_STATUS_OK;
}