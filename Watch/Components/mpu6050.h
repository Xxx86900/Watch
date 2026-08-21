/**
 * @file    mpu6050.h
 * @brief   MPU6050 六轴惯性传感器驱动接口。
 *
 * 本文件定义 MPU6050 设备驱动对上层提供的数据结构和操作接口。
 *
 * 驱动负责：
 * - MPU6050 初始化与设备识别；
 * - 加速度计、陀螺仪和温度原始数据读取；
 * - 原始数据向物理量的转换。
 *
 * 本驱动不负责姿态解算、互补滤波、抬腕检测等运动算法，
 * 相关功能应由上层 motion_service 模块实现。
 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief MPU6050 驱动状态。
 */
typedef enum
{
    MPU6050_STATUS_OK = 0,
    MPU6050_STATUS_ERROR_PARAM,
    MPU6050_STATUS_ERROR_I2C,
    MPU6050_STATUS_ERROR_ID
} Mpu6050Status_t;


/**
 * @brief MPU6050 原始传感器数据。
 *
 * 数据保持 MPU6050 寄存器中的原始有符号 16 位数值，
 * 未进行量程换算和零偏补偿。
 */
typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t temperature;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} Mpu6050RawData_t;


/**
 * @brief MPU6050 转换后的物理量数据。
 *
 * 加速度单位为 g；
 * 角速度单位为 degree/s；
 * 温度单位为摄氏度。
 */
typedef struct
{
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

    float temperature_c;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
} Mpu6050Data_t;


/**
 * @brief 初始化 MPU6050。
 *
 * 初始化过程包括：
 * - 检查 I2C 设备是否存在；
 * - 软件复位 MPU6050；
 * - 唤醒传感器；
 * - 校验 WHO_AM_I；
 * - 配置数字低通滤波器；
 * - 配置采样率；
 * - 配置加速度计和陀螺仪量程。
 *
 * 默认配置：
 * - 采样率：100 Hz；
 * - 加速度计量程：±4 g；
 * - 陀螺仪量程：±500 degree/s；
 * - DLPF：加速度约 44 Hz，陀螺仪约 42 Hz。
 *
 * @return MPU6050_STATUS_OK 初始化成功；
 *         其他值表示初始化失败。
 */
Mpu6050Status_t mpu6050_init(void);


/**
 * @brief 读取 MPU6050 WHO_AM_I 寄存器。
 *
 * @param id 用于保存读取到的设备 ID。
 *
 * @return MPU6050_STATUS_OK 读取成功；
 *         其他值表示读取失败。
 */
Mpu6050Status_t mpu6050_read_id(uint8_t *id);


/**
 * @brief 判断 MPU6050 是否正常连接。
 *
 * 函数读取 WHO_AM_I 寄存器并判断设备 ID 是否为 MPU6050
 * 的标准值 0x68。
 *
 * @return true  MPU6050 正常连接；
 *         false MPU6050 未连接或通信异常。
 */
bool mpu6050_is_connected(void);


/**
 * @brief 读取 MPU6050 原始六轴和温度数据。
 *
 * 从 ACCEL_XOUT_H 开始连续读取 14 字节，以一次 I2C
 * 事务获得三轴加速度、温度和三轴角速度原始数据。
 *
 * @param data 用于保存原始数据。
 *
 * @return MPU6050_STATUS_OK 读取成功；
 *         其他值表示读取失败。
 */
Mpu6050Status_t mpu6050_read_raw(Mpu6050RawData_t *data);


/**
 * @brief 读取 MPU6050 数据并转换为物理量。
 *
 * 当前驱动配置：
 * - 加速度计 ±4 g，对应 8192 LSB/g；
 * - 陀螺仪 ±500 degree/s，对应 65.5 LSB/(degree/s)。
 *
 * @param data 用于保存转换后的传感器数据。
 *
 * @return MPU6050_STATUS_OK 读取成功；
 *         其他值表示读取失败。
 */
Mpu6050Status_t mpu6050_read(Mpu6050Data_t *data);


#endif /* MPU6050_H */