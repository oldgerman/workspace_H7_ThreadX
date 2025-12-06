/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_msc.c
 * @author  MCD Application Team
 * @brief   USBX Device MSC applicative source file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2020-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_msc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_filex.h"
#include "app_usbx_device.h" //!< 提供 EventFlagMsc
#include "fx_stm32_sd_driver.h"
#include "main.h"
#include "sdmmc.h"

#include "ux_api.h"
#include "ux_device_class_storage.h"
#include "ux_device_stack.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MEDIA_STATUS_OK 0u
#define MEDIA_STATUS_NOT_PRESENT 1u
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define SD_READ_FLAG 0x01
#define SD_WRITE_FLAG 0x02
#define SD_TIMEOUT 100U
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static int32_t check_sd_status(VOID);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile UINT g_media_present = UX_TRUE;  // 介质已移除的标志
volatile UINT g_media_changed = UX_FALSE; // 介质已变更（只报一次）
volatile UINT g_media_changed_count = 0U; // 介质变更通知计数（让 Windows 多次收到 UNIT ATTENTION）

VOID ux_app_msc_media_eject(VOID) { g_media_present = UX_FALSE; }

/* DEBUG监视变量 */
/* 统计四个USBD函数被调用次数，CubeIDE 现场表达式可实时刷新 */
uint32_t cnt_USBD_STORAGE_Activate = 0;
uint32_t cnt_USBD_STORAGE_Deactivate = 0;
uint32_t cnt_USBD_STORAGE_Read = 0;
uint32_t cnt_USBD_STORAGE_Write = 0;
uint32_t cnt_USBD_STORAGE_Flush = 0;
uint32_t cnt_USBD_STORAGE_Status = 0;
uint32_t cnt_USBD_STORAGE_Notification = 0;
uint32_t cnt_USBD_STORAGE_GetMediaLastLba = 0;
uint32_t cnt_USBD_STORAGE_GetBlockLength = 0;
/* USBD_STORAGE_Status向win11返回介质变更标记 */
uint8_t called_USBD_STORAGE_Status = 0;
/* USBD_STORAGE_Flush向FileX返回介质变更标记 */
uint8_t called_USBD_STORAGE_Flush = 0;
/* 记录SD DMA读写函数参数 number_blocks 的历史最大值和最小值 */
ULONG sd_read_blocks_max;
ULONG sd_read_blocks_min;
ULONG sd_write_blocks_max;
ULONG sd_write_blocks_min;
/* USER CODE END 0 */

/**
 * @brief  USBD_STORAGE_Activate
 *         This function is called when insertion of a storage device.
 * @param  storage_instance: Pointer to the storage class instance.
 * @retval none
 */
VOID USBD_STORAGE_Activate(VOID *storage_instance) {
  /* USER CODE BEGIN USBD_STORAGE_Activate */
  UX_PARAMETER_NOT_USED(storage_instance);
  cnt_USBD_STORAGE_Activate++;
  /* USER CODE END USBD_STORAGE_Activate */

  return;
}

/**
 * @brief  USBD_STORAGE_Deactivate
 *         This function is called when extraction of a storage device.
 * @param  storage_instance: Pointer to the storage class instance.
 * @retval none
 */
VOID USBD_STORAGE_Deactivate(VOID *storage_instance) {
  /* USER CODE BEGIN USBD_STORAGE_Desactivate */
  UX_PARAMETER_NOT_USED(storage_instance);
  cnt_USBD_STORAGE_Deactivate++;
  /* USER CODE END USBD_STORAGE_Desactivate */

  return;
}

/**
 * @brief  USBD_STORAGE_Read
 *         This function is invoked to read from media.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  data_pointer: Address of the buffer to be used for reading or
 * writing.
 * @param  number_blocks: number of sectors to read/write.
 * @param  lba: Logical block address is the sector address to read.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Read(VOID *storage_instance, ULONG lun, UCHAR *data_pointer,
                       ULONG number_blocks, ULONG lba, ULONG *media_status) {
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Read */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_status);

  cnt_USBD_STORAGE_Read++;

  ULONG ReadFlags = 0U;

  /* Check if the SD card is present */
  //  if (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5) != GPIO_PIN_RESET)
  //  {
  /* Check id SD card is ready */
  if (check_sd_status() != HAL_OK) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02, 0x3A, 0x00);
    return UX_ERROR;
  }

  /* Clear the read flag before starting the DMA transfer */
  tx_event_flags_set(&EventFlagMsc, ~SD_READ_FLAG, TX_AND);

  /* Start the Dma write */
  status = HAL_SD_ReadBlocks_DMA(&hsd2, data_pointer, lba, number_blocks);
  if (status != HAL_OK) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x03, 0x11, 0x00);
    return UX_ERROR;
  }
  /* 记录number_blocks历史最大值和最小值 */
  // 更新最大值
  if (number_blocks > sd_read_blocks_max)
  {
	  sd_read_blocks_max = number_blocks;
  }
  // 更新最小值
  if (number_blocks < sd_read_blocks_min)
  {
      sd_read_blocks_min = number_blocks;
  }

// 不需要缓存维护API，访问的是AXISRAM2区域
#if (FX_STM32_SD_CACHE_MAINTENANCE == 1)
  invalidate_cache_by_addr((uint32_t *)data_pointer,
                           number_blocks * FX_STM32_SD_DEFAULT_SECTOR_SIZE);
#endif

  /* Wait on readflag until SD card is ready to use for new operation */
  if (tx_event_flags_get(&EventFlagMsc, SD_READ_FLAG, TX_OR_CLEAR, &ReadFlags,
                         TX_WAIT_FOREVER) != TX_SUCCESS) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x03, 0x11, 0x00);
    return UX_ERROR;
  }
  //  }

  /* USER CODE END USBD_STORAGE_Read */

  return status;
}

/**
 * @brief  USBD_STORAGE_Write
 *         This function is invoked to write in media.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  data_pointer: Address of the buffer to be used for reading or
 * writing.
 * @param  number_blocks: number of sectors to read/write.
 * @param  lba: Logical block address is the sector address to read.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Write(VOID *storage_instance, ULONG lun, UCHAR *data_pointer,
                        ULONG number_blocks, ULONG lba, ULONG *media_status) {
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Write */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_status);

  cnt_USBD_STORAGE_Write++;

  ULONG WriteFlags = 0U;

  /* Check if the SD card is present */
  //  if (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5) != GPIO_PIN_RESET)
  //  {
  /* Check if SD card is ready */
  if (check_sd_status() != HAL_OK) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02, 0x3A, 0x00);
    return UX_ERROR;
  }

// 不需要缓存维护API，访问的是AXISRAM2区域
#if (FX_STM32_SD_CACHE_MAINTENANCE == 1)
  clean_cache_by_addr((uint32_t *)data_pointer,
                      number_blocks * FX_STM32_SD_DEFAULT_SECTOR_SIZE);
#endif

  /* Clear the write flag before starting the DMA transfer */
  tx_event_flags_set(&EventFlagMsc, ~SD_WRITE_FLAG, TX_AND);

  /* Start the Dma write */
  status = HAL_SD_WriteBlocks_DMA(&hsd2, data_pointer, lba, number_blocks);

  if (status != HAL_OK) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x03, 0x0C, 0x02);
    return UX_ERROR;
  }

  /* 记录number_blocks历史最大值和最小值 */
  // 更新最大值
  if (number_blocks > sd_write_blocks_max)
  {
      sd_write_blocks_max = number_blocks;
  }
  // 更新最小值
  if (number_blocks < sd_write_blocks_min)
  {
      sd_write_blocks_min = number_blocks;
  }
  /* Wait on writeflag until SD card is ready to use for new operation */
  if (tx_event_flags_get(&EventFlagMsc, SD_WRITE_FLAG, TX_OR_CLEAR, &WriteFlags,
                         TX_WAIT_FOREVER) != TX_SUCCESS) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x03, 0x0C, 0x02);
    return UX_ERROR;
  }
  //  }

  /* USER CODE END USBD_STORAGE_Write */

  return status;
}

/**
 * @brief  USBD_STORAGE_Flush
 *         This function is invoked to flush media.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  number_blocks: number of sectors to read/write.
 * @param  lba: Logical block address is the sector address to read.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Flush(VOID *storage_instance, ULONG lun, ULONG number_blocks,
                        ULONG lba, ULONG *media_status) {
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Flush */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(number_blocks);
  UX_PARAMETER_NOT_USED(lba);
  UX_PARAMETER_NOT_USED(media_status);

  /* windows修改文件的最后一步一定调用这个函数
   * FIleX需要检测到这个函数被执行就需要调用 fx_media_flush */
  cnt_USBD_STORAGE_Flush++;

  called_USBD_STORAGE_Flush = 1;
  /* USER CODE END USBD_STORAGE_Flush */

  return status;
}

/**
 * @brief  USBD_STORAGE_Status
 *         This function is invoked to obtain the status of the device.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  media_id: is not currently used.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Status(VOID *storage_instance, ULONG lun, ULONG media_id,
                         ULONG *media_status) {
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Status */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_id);
  UX_PARAMETER_NOT_USED(media_status);

  cnt_USBD_STORAGE_Status++;
  // 此函数在 MSC U盘挂载在windows时，会被一直定期调用
  if (g_media_present == UX_FALSE) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02, 0x3A, 0x00);
    called_USBD_STORAGE_Status = 1;
    return UX_ERROR; // 让主机认为介质不在
  }
  if (g_media_changed == UX_TRUE) {
    *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x06, 0x28, 0x00);
    // 让 Windows 收到多次 UNIT ATTENTION 才清除标志，确保它重读 FAT
    g_media_changed_count++;
    if (g_media_changed_count >= 3) { // 返回 3 次后清除
      g_media_changed = UX_FALSE;
      g_media_changed_count = 0U;
    }
    return UX_ERROR;
  }
  /* USER CODE END USBD_STORAGE_Status */

  return status;
}

/**
 * @brief  USBD_STORAGE_Notification
 *         This function is invoked to obtain the notification of the device.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  media_id: is not currently used.
 * @param  notification_class: specifies the class of notification.
 * @param  media_notification: response for the notification.
 * @param  media_notification_length: length of the response buffer.
 * @retval status
 */
UINT USBD_STORAGE_Notification(VOID *storage_instance, ULONG lun,
                               ULONG media_id, ULONG notification_class,
                               UCHAR **media_notification,
                               ULONG *media_notification_length) {
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Notification */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_id);
  UX_PARAMETER_NOT_USED(notification_class);
  UX_PARAMETER_NOT_USED(media_notification);
  UX_PARAMETER_NOT_USED(media_notification_length);
  cnt_USBD_STORAGE_Notification++;
  /* USER CODE END USBD_STORAGE_Notification */

  return status;
}

/**
 * @brief  USBD_STORAGE_GetMediaLastLba
 *         Get Media last LBA.
 * @param  none
 * @retval last lba
 */
ULONG USBD_STORAGE_GetMediaLastLba(VOID) {
  ULONG LastLba = 0U;

  /* USER CODE BEGIN USBD_STORAGE_GetMediaLastLba */
#if 1
  LastLba = (ULONG)(USBD_SD_CardInfo.BlockNbr - 1);
#else // 等价写法
  LastLba = (ULONG)(hsd2.SdCard.BlockNbr - 1);
#endif

  cnt_USBD_STORAGE_GetMediaLastLba++;
  /* USER CODE END USBD_STORAGE_GetMediaLastLba */

  return LastLba;
}

/**
 * @brief  USBD_STORAGE_GetMediaBlocklength
 *         Get Media block length.
 * @param  none.
 * @retval block length.
 */
ULONG USBD_STORAGE_GetMediaBlocklength(VOID) {
  ULONG MediaBlockLen = 0U;

  /* USER CODE BEGIN USBD_STORAGE_GetMediaBlocklength */
#if 1
  MediaBlockLen = (ULONG)USBD_SD_CardInfo.BlockSize;
#else // 等价写法
  MediaBlockLen = (ULONG)hsd2.SdCard.BlockSize;
#endif
  cnt_USBD_STORAGE_GetBlockLength++;
  /* USER CODE END USBD_STORAGE_GetMediaBlocklength */

  return MediaBlockLen;
}

/* USER CODE BEGIN 2 */

/**
 * @brief  check_sd_status
 *         check SD card Transfer Status.
 * @param  none
 * @retval BSP status
 */
static int32_t check_sd_status(VOID) {

  uint32_t start = tx_time_get();

  while (tx_time_get() - start < SD_TIMEOUT) {
    if (HAL_SD_GetCardState(&hsd2) == HAL_SD_CARD_TRANSFER) {
      return HAL_OK;
    }
  }

  return HAL_ERROR;
}

/* USER CODE END 2 */
