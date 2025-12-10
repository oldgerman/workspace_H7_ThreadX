
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_filex.c
  * @author  MCD Application Team
  * @brief   FileX applicative file
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
#include "app_filex.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* Main thread stack size */
#define FX_APP_THREAD_STACK_SIZE         4096
/* Main thread priority */
#define FX_APP_THREAD_PRIO               10

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* 标记CubeMX生成的变量为弱符号（GCC专属） */
#pragma weak fx_sd_media_memory
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Main thread global data structures.  */
TX_THREAD       fx_app_thread;

/* Buffer for FileX FX_MEDIA sector cache. */
ALIGN_32BYTES (uint32_t fx_sd_media_memory[FX_STM32_SD_DEFAULT_SECTOR_SIZE / sizeof(uint32_t)]);
/* Define FileX global data structures.  */
FX_MEDIA        sdio_disk;

/* USER CODE BEGIN PV */
__attribute__((section(".axisram2_bss"), aligned(32)))
uint32_t fx_sd_media_memory[FX_STM32_SD_DEFAULT_SECTOR_SIZE / sizeof(uint32_t)];
extern void DemoFileX(void);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* Main thread entry function.  */
void fx_app_thread_entry(ULONG thread_input);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Application FileX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_FileX_Init(VOID *memory_ptr)
{
  UINT ret = FX_SUCCESS;

  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  VOID *pointer;

  /* USER CODE BEGIN MX_FileX_MEM_POOL */

  /* USER CODE END MX_FileX_MEM_POOL */

  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */

  /*Allocate memory for the main thread's stack*/
  ret = tx_byte_allocate(byte_pool, &pointer, FX_APP_THREAD_STACK_SIZE, TX_NO_WAIT);

  /* Check FX_APP_THREAD_STACK_SIZE allocation*/
  if (ret != FX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread.  */
  ret = tx_thread_create(&fx_app_thread, FX_APP_THREAD_NAME, fx_app_thread_entry, 0, pointer, FX_APP_THREAD_STACK_SIZE,
                         FX_APP_THREAD_PRIO, FX_APP_PREEMPTION_THRESHOLD, FX_APP_THREAD_TIME_SLICE, FX_APP_THREAD_AUTO_START);

  /* Check main thread creation */
  if (ret != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }
  /* USER CODE BEGIN MX_FileX_Init */

  /* USER CODE END MX_FileX_Init */

  /* Initialize FileX.  */
  fx_system_initialize();

  /* USER CODE BEGIN MX_FileX_Init 1*/
  /* TODO：
   * fx_system_initialize() 完成后发送信号量给 DemoFileX任务？
   * 算了，此函数可以多次调用，DemoFileX() 再调用一次解决  */
  /* USER CODE END MX_FileX_Init 1*/

  return ret;
}

 /**
 * @brief  Main thread entry.
 * @param thread_input: ULONG user argument used by the thread entry
 * @retval none
 */
void fx_app_thread_entry(ULONG thread_input)
{
  UINT sd_status = FX_SUCCESS;
  /* USER CODE BEGIN fx_app_thread_entry 0 */
  /**
   * https://forum.anfulai.cn/forum.php?mod=viewthread&tid=128791&highlight=usbx%2Bmsc
   * 原帖作者说到：FileX和USBX都开启，会有资源访问冲突的问题，我这边的临时解决方案是挂起FileX相关任务。后边可能会使用互斥信号量来解决。
   *
   * 我实测，加这个才会 Os 优化编译弹出 有容量的磁盘
   */
  tx_thread_suspend(&fx_app_thread);
  /* USER CODE END fx_app_thread_entry 0 */

  /* Open the SD disk driver */
  sd_status =  fx_media_open(&sdio_disk, FX_SD_VOLUME_NAME, fx_stm32_sd_driver, (VOID *)FX_NULL, (VOID *) fx_sd_media_memory, sizeof(fx_sd_media_memory));

  /* Check the media open sd_status */
  if (sd_status != FX_SUCCESS)
  {
    /* USER CODE BEGIN SD open error */
    while(1);
    /* USER CODE END SD open error */
  }

  /* USER CODE BEGIN fx_app_thread_entry 1 */
  sd_status =  fx_file_create(&sdio_disk, "myFile008.txt");
  sd_status =  fx_media_close(&sdio_disk);
  /* 卸载SD卡 */
//  DemoFileX();
  /**
   * https://forum.anfulai.cn/forum.php?mod=viewthread&tid=128791&highlight=usbx%2Bmsc
   * 原帖作者说到：FileX和USBX都开启，会有资源访问冲突的问题，我这边的临时解决方案是挂起FileX相关任务。后边可能会使用互斥信号量来解决。
   *
   * 我实测，加这个才会 Os 优化编译弹出 有容量的磁盘
   */
  tx_thread_suspend(&fx_app_thread);
  /* USER CODE END fx_app_thread_entry 1 */
}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
