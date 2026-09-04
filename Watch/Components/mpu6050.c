#include "mpu6050.h"

#include <stddef.h>

#define MPU6050_I2C_ADDRESS       (0x68U << 1U)
#define MPU6050_I2C_TIMEOUT_MS    30U

#define MPU6050_REG_SMPLRT_DIV    0x19U
#define MPU6050_REG_CONFIG        0x1AU
#define MPU6050_REG_GYRO_CONFIG   0x1BU
#define MPU6050_REG_ACCEL_CONFIG  0x1CU
#define MPU6050_REG_ACCEL_XOUT_H  0x3BU
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_PWR_MGMT_2    0x6CU
#define MPU6050_REG_WHO_AM_I      0x75U

static bool mpu6050_write_register(mpu6050_t *device,uint8_t reg,uint8_t value)
{
  return (device != NULL) && (device->i2c != NULL) &&
         (HAL_I2C_Mem_Write(device->i2c,MPU6050_I2C_ADDRESS,reg,
                            I2C_MEMADD_SIZE_8BIT,&value,1U,
                            MPU6050_I2C_TIMEOUT_MS) == HAL_OK);
}

static bool mpu6050_read_registers(mpu6050_t *device,uint8_t reg,
                                   uint8_t *data,uint16_t size)
{
  return (device != NULL) && (device->i2c != NULL) && (data != NULL) &&
         (HAL_I2C_Mem_Read(device->i2c,MPU6050_I2C_ADDRESS,reg,
                           I2C_MEMADD_SIZE_8BIT,data,size,
                           MPU6050_I2C_TIMEOUT_MS) == HAL_OK);
}

bool mpu6050_init(mpu6050_t *device,I2C_HandleTypeDef *i2c)
{
  uint8_t identity = 0U;

  if ((device == NULL) || (i2c == NULL))
  {
    return false;
  }

  device->i2c = i2c;
  device->ready = false;

  if (HAL_I2C_IsDeviceReady(i2c,MPU6050_I2C_ADDRESS,2U,
                            MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }

  if (!mpu6050_read_registers(device,MPU6050_REG_WHO_AM_I,&identity,1U) ||
      ((identity & 0x7EU) != 0x68U))
  {
    return false;
  }

  if (!mpu6050_write_register(device,MPU6050_REG_PWR_MGMT_1,0x01U) ||
      !mpu6050_write_register(device,MPU6050_REG_PWR_MGMT_2,0x00U) ||
      !mpu6050_write_register(device,MPU6050_REG_SMPLRT_DIV,0x09U) ||
      !mpu6050_write_register(device,MPU6050_REG_CONFIG,0x03U) ||
      !mpu6050_write_register(device,MPU6050_REG_GYRO_CONFIG,0x00U) ||
      !mpu6050_write_register(device,MPU6050_REG_ACCEL_CONFIG,0x00U))
  {
    return false;
  }

  HAL_Delay(20U);
  device->ready = true;
  return true;
}

bool mpu6050_read(mpu6050_t *device,mpu6050_sample_t *sample)
{
  uint8_t data[14];

  if ((device == NULL) || !device->ready || (sample == NULL) ||
      !mpu6050_read_registers(device,MPU6050_REG_ACCEL_XOUT_H,
                              data,(uint16_t)sizeof(data)))
  {
    if (device != NULL)
    {
      device->ready = false;
    }
    return false;
  }

  sample->accel_x = (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
  sample->accel_y = (int16_t)(((uint16_t)data[2] << 8U) | data[3]);
  sample->accel_z = (int16_t)(((uint16_t)data[4] << 8U) | data[5]);
  sample->temperature = (int16_t)(((uint16_t)data[6] << 8U) | data[7]);
  sample->gyro_x = (int16_t)(((uint16_t)data[8] << 8U) | data[9]);
  sample->gyro_y = (int16_t)(((uint16_t)data[10] << 8U) | data[11]);
  sample->gyro_z = (int16_t)(((uint16_t)data[12] << 8U) | data[13]);
  return true;
}

bool mpu6050_set_sleep(mpu6050_t *device,bool sleep)
{
  if ((device == NULL) || !device->ready)
  {
    return false;
  }
  return mpu6050_write_register(device,MPU6050_REG_PWR_MGMT_1,
                                sleep ? 0x40U : 0x01U);
}

bool mpu6050_is_ready(const mpu6050_t *device)
{
  return (device != NULL) && device->ready;
}
