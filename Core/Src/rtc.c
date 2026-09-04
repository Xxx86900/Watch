/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

#define WATCH_RTC_BACKUP_MAGIC 0xA55AU
#define WATCH_RTC_PACK_DATE(year,month,day) \
  ((((uint32_t)(year) & 0x7FU) << 9U) | \
   (((uint32_t)(month) & 0x0FU) << 5U) | ((uint32_t)(day) & 0x1FU))
#define WATCH_RTC_DEFAULT_DATE WATCH_RTC_PACK_DATE(26U,1U,1U)

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  if (HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1) != WATCH_RTC_BACKUP_MAGIC)
  {

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 0x1;
  DateToUpdate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

    DateToUpdate.Year = 26U;
    DateToUpdate.Month = 1U;
    DateToUpdate.Date = 1U;
    if (HAL_RTC_SetDate(&hrtc,&DateToUpdate,RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR1,WATCH_RTC_BACKUP_MAGIC);
    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR2,WATCH_RTC_DEFAULT_DATE);
  }
  else
  {
    uint32_t packed_date = HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR2);

    DateToUpdate.Year = (uint8_t)((packed_date >> 9U) & 0x7FU);
    DateToUpdate.Month = (uint8_t)((packed_date >> 5U) & 0x0FU);
    DateToUpdate.Date = (uint8_t)(packed_date & 0x1FU);
    if ((DateToUpdate.Year > 99U) ||
        (DateToUpdate.Month < 1U) || (DateToUpdate.Month > 12U) ||
        (DateToUpdate.Date < 1U) || (DateToUpdate.Date > 31U))
    {
      DateToUpdate.Year = 26U;
      DateToUpdate.Month = 1U;
      DateToUpdate.Date = 1U;
      HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR2,WATCH_RTC_DEFAULT_DATE);
    }
    if (HAL_RTC_SetDate(&hrtc,&DateToUpdate,RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
  }

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
