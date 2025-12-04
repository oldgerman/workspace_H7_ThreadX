/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_usbx_device.h
  * @author  MCD Application Team
  * @brief   USBX Device applicative header file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_USBX_DEVICE_H__
#define __APP_USBX_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Includes ------------------------------------------------------------------*/
#include "ux_api.h"
#include "ux_device_descriptors.h"
#include "ux_device_msc.h"
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tx_api.h"
#include "ux_device_class_cdc_acm.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/**
 * 启用CDC ACM 非阻塞传输，必须禁用 UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE，并且需要在 USBX 字节池和 USBX_MEMORY_SIZE 中添加 2048 个额外字节。
 * USBX_MEMORY_SIZE 就是 USBX_DEVICE_MEMORY_STACK_SIZE：
 * https://github.com/STMicroelectronics/STM32CubeH5/blob/188d908ee29667daf0ec2f1dd40bfc2e91389e5e/Projects/STM32H573I-DK/Applications/OpenBootloader/USBX/App/app_usbx_device.c#L25
 *   ↓ USBX_DEVICE_MEMORY_STACK_SIZE 30KB不够用，卡住在 ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name,
 *   ↓ 改为40K够用！
 */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
#define USBX_DEVICE_MEMORY_STACK_SIZE       1024*40

#define UX_DEVICE_APP_THREAD_STACK_SIZE   4096
#define UX_DEVICE_APP_THREAD_PRIO         10

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT MX_USBX_Device_Init(VOID *memory_ptr);

/* USER CODE BEGIN EFP */
VOID USBX_APP_Device_Init(VOID);
UINT USBX_MSC_Pause(VOID);
UINT USBX_MSC_Resume(VOID);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

#ifndef UX_DEVICE_APP_THREAD_NAME
#define UX_DEVICE_APP_THREAD_NAME  "USBX Device App Main Thread"
#endif

#ifndef UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD
#define UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD  UX_DEVICE_APP_THREAD_PRIO
#endif

#ifndef UX_DEVICE_APP_THREAD_TIME_SLICE
#define UX_DEVICE_APP_THREAD_TIME_SLICE  TX_NO_TIME_SLICE
#endif

#ifndef UX_DEVICE_APP_THREAD_START_OPTION
#define UX_DEVICE_APP_THREAD_START_OPTION  TX_AUTO_START
#endif

/* USER CODE BEGIN 2 */
/* Exported variables --------------------------------------------------------*/
extern TX_EVENT_FLAGS_GROUP EventFlagMsc;
extern TX_EVENT_FLAGS_GROUP EventFlagCdcAcm;
extern UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER cdc_acm_callback_parameter;

/* USER CODE END 2 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_USBX_DEVICE_H__ */
