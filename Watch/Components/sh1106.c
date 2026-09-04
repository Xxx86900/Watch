#include "sh1106.h"

#include <stddef.h>

#define SH1106_I2C_ADDRESS      (0x3CU << 1U)
#define SH1106_I2C_TIMEOUT_MS   30U
#define SH1106_COLUMN_OFFSET     2U
#define SH1106_PAGE_COUNT       (SH1106_HEIGHT / 8U)

static bool sh1106_write_commands(sh1106_t *display,const uint8_t *commands,uint8_t count)
{
  uint8_t packet[32];

  if ((display == NULL) || (display->i2c == NULL) ||
      (commands == NULL) || (count == 0U) || (count >= sizeof(packet)))
  {
    return false;
  }

  packet[0] = 0x00U;
  for (uint8_t index = 0U; index < count; index++)
  {
    packet[index + 1U] = commands[index];
  }

  return HAL_I2C_Master_Transmit(display->i2c,SH1106_I2C_ADDRESS,
                                 packet,(uint16_t)count + 1U,
                                 SH1106_I2C_TIMEOUT_MS) == HAL_OK;
}

bool sh1106_init(sh1106_t *display,I2C_HandleTypeDef *i2c)
{
  static const uint8_t init_commands[] =
  {
    0xAEU,
    0xD5U,0x80U,
    0xA8U,0x3FU,
    0xD3U,0x00U,
    0x40U,
    0xA1U,
    0xC8U,
    0xDAU,0x12U,
    0x81U,0xCFU,
    0xD9U,0xF1U,
    0xDBU,0x30U,
    0xA4U,
    0xA6U,
    0x8DU,0x14U,
    0xAFU
  };

  if ((display == NULL) || (i2c == NULL))
  {
    return false;
  }

  display->i2c = i2c;
  display->ready = false;
  HAL_Delay(50U);

  if (HAL_I2C_IsDeviceReady(i2c,SH1106_I2C_ADDRESS,2U,
                            SH1106_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }

  display->ready = sh1106_write_commands(display,init_commands,
                                          (uint8_t)sizeof(init_commands));
  return display->ready;
}

bool sh1106_update(sh1106_t *display,const uint8_t *buffer)
{
  uint8_t data_packet[SH1106_WIDTH + 1U];

  if ((display == NULL) || !display->ready || (buffer == NULL))
  {
    return false;
  }

  data_packet[0] = 0x40U;
  for (uint8_t page = 0U; page < SH1106_PAGE_COUNT; page++)
  {
    const uint8_t cursor_commands[] =
    {
      (uint8_t)(0xB0U | page),
      (uint8_t)(0x10U | (SH1106_COLUMN_OFFSET >> 4U)),
      (uint8_t)(SH1106_COLUMN_OFFSET & 0x0FU)
    };

    if (!sh1106_write_commands(display,cursor_commands,
                                (uint8_t)sizeof(cursor_commands)))
    {
      display->ready = false;
      return false;
    }

    for (uint16_t x = 0U; x < SH1106_WIDTH; x++)
    {
      data_packet[x + 1U] = buffer[(uint16_t)page * SH1106_WIDTH + x];
    }

    if (HAL_I2C_Master_Transmit(display->i2c,SH1106_I2C_ADDRESS,
                                data_packet,(uint16_t)sizeof(data_packet),
                                SH1106_I2C_TIMEOUT_MS) != HAL_OK)
    {
      display->ready = false;
      return false;
    }
  }

  return true;
}

bool sh1106_set_contrast(sh1106_t *display,uint8_t contrast)
{
  const uint8_t commands[] = {0x81U,contrast};
  return sh1106_write_commands(display,commands,(uint8_t)sizeof(commands));
}

bool sh1106_set_power(sh1106_t *display,bool enabled)
{
  const uint8_t command = enabled ? 0xAFU : 0xAEU;
  return sh1106_write_commands(display,&command,1U);
}

bool sh1106_is_ready(const sh1106_t *display)
{
  return (display != NULL) && display->ready;
}
