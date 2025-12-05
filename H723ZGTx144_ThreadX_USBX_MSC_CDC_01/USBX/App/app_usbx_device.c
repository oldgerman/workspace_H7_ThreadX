/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_usbx_device.c
  * @author  MCD Application Team
  * @brief   USBX Device applicative file
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
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_otg.h"
#include "ux_dcd_stm32.h"
#include "ux_device_msc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

static ULONG storage_interface_number;
static ULONG storage_configuration_number;
static ULONG cdc_acm_interface_number;
static ULONG cdc_acm_configuration_number;
static UX_SLAVE_CLASS_STORAGE_PARAMETER storage_parameter;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter;
static TX_THREAD ux_device_app_thread;

/* USER CODE BEGIN PV */
/* 事件标志组 */
TX_EVENT_FLAGS_GROUP EventFlagMsc;         //!< MSC     事件标志组
TX_EVENT_FLAGS_GROUP EventFlagCdcAcm;      //!< CDC ACM 事件标志组

/* 线程控制块 */
static TX_THREAD ux_cdc_acm_thread;            //!< CDC ACM 线程控制块
static TX_THREAD ux_cdc_acm_cmd_parse_thread;  //!< CDC ACM 命令拆包解析线程控制块

/* 参数绑定 */
UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER cdc_acm_callback_parameter; //!< 参数绑定 CDC ACM 回调函数

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID app_ux_device_thread_entry(ULONG thread_input);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Application USBX Device Initialization.
  * @param  memory_ptr: memory pointer
  * @retval status
  */
UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
  UINT ret = UX_SUCCESS;
  UCHAR *device_framework_high_speed;
  UCHAR *device_framework_full_speed;
  ULONG device_framework_hs_length;
  ULONG device_framework_fs_length;
  ULONG string_framework_length;
  ULONG language_id_framework_length;
  UCHAR *string_framework;
  UCHAR *language_id_framework;
  UCHAR *pointer;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN MX_USBX_Device_Init0 */

  /* USER CODE END MX_USBX_Device_Init0 */

  /* Allocate the stack for USBX Memory */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_DEVICE_MEMORY_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_ALLOCATE_STACK_ERROR */
    return TX_POOL_ERROR;
    /* USER CODE END USBX_ALLOCATE_STACK_ERROR */
  }

  /* Initialize USBX Memory */
  if (ux_system_initialize(pointer, USBX_DEVICE_MEMORY_STACK_SIZE, UX_NULL, 0) != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_SYSTEM_INITIALIZE_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_SYSTEM_INITIALIZE_ERROR */
  }

  /* Get Device Framework High Speed and get the length */
  device_framework_high_speed = USBD_Get_Device_Framework_Speed(USBD_HIGH_SPEED,
                                                                &device_framework_hs_length);

  /* Get Device Framework Full Speed and get the length */
  device_framework_full_speed = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED,
                                                                &device_framework_fs_length);

  /* Get String Framework and get the length */
  string_framework = USBD_Get_String_Framework(&string_framework_length);

  /* Get Language Id Framework and get the length */
  language_id_framework = USBD_Get_Language_Id_Framework(&language_id_framework_length);

  /* Install the device portion of USBX */
  if (ux_device_stack_initialize(device_framework_high_speed,
                                 device_framework_hs_length,
                                 device_framework_full_speed,
                                 device_framework_fs_length,
                                 string_framework,
                                 string_framework_length,
                                 language_id_framework,
                                 language_id_framework_length,
                                 UX_NULL) != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_DEVICE_INITIALIZE_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_DEVICE_INITIALIZE_ERROR */
  }

  /* Initialize the storage class parameters for the device */
  storage_parameter.ux_slave_class_storage_instance_activate   = USBD_STORAGE_Activate;
  storage_parameter.ux_slave_class_storage_instance_deactivate = USBD_STORAGE_Deactivate;

  /* Store the number of LUN in this device storage instance */
  storage_parameter.ux_slave_class_storage_parameter_number_lun = STORAGE_NUMBER_LUN;

  /* Initialize the storage class parameters for reading/writing to the Flash Disk */
  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_last_lba = USBD_STORAGE_GetMediaLastLba();

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_block_length = USBD_STORAGE_GetMediaBlocklength();

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_type = 0;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_removable_flag = STORAGE_REMOVABLE_FLAG;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_read_only_flag = STORAGE_READ_ONLY;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_read = USBD_STORAGE_Read;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_write = USBD_STORAGE_Write;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_flush = USBD_STORAGE_Flush;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_status = USBD_STORAGE_Status;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_notification = USBD_STORAGE_Notification;

  /* USER CODE BEGIN STORAGE_PARAMETER */
  // 用户自定义MSC类的参数绑定代码
  /* USER CODE END STORAGE_PARAMETER */

  /* Get storage configuration number */
  storage_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_MSC, 0);

  /* Find storage interface number */
  storage_interface_number = USBD_Get_Interface_Number(CLASS_TYPE_MSC, 0);

  /* Initialize the device storage class */
  if (ux_device_stack_class_register(_ux_system_slave_class_storage_name,
                                     ux_device_class_storage_entry,
                                     storage_configuration_number,
                                     storage_interface_number,
                                     &storage_parameter) != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_DEVICE_STORAGE_REGISTER_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_DEVICE_STORAGE_REGISTER_ERROR */
  }

  /* Initialize the cdc acm class parameters for the device */
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate   = USBD_CDC_ACM_Activate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = USBD_CDC_ACM_Deactivate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change    = USBD_CDC_ACM_ParameterChange;

  /* USER CODE BEGIN CDC_ACM_PARAMETER */
  /* 用户自定义CDC类的参数绑定代码 */

  // 绑定 USB CDC ACM 收发中断回调函数
  cdc_acm_callback_parameter.ux_device_class_cdc_acm_parameter_read_callback = USBD_CDC_ACM_Read_Callback;
  cdc_acm_callback_parameter.ux_device_class_cdc_acm_parameter_write_callback = USBD_CDC_ACM_Write_Callback;

  /* ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &cdc_acm_callback_parameter);
   * ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~
   * 这之后，需要执行启动带回调的传输功能 IOCTL 命令，需在 USBD_CDC_ACM_Activate() 中执行，
   * 因为当 USB 设备枚举完成，由 USBX 调用激活回调函数 USBD_CDC_ACM_Activate() 时， cdc_acm 实例才创建并初始化
   */

  /* USER CODE END CDC_ACM_PARAMETER */

  /* Get cdc acm configuration number */
  cdc_acm_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_CDC_ACM, 0);

  /* Find cdc acm interface number */
  cdc_acm_interface_number = USBD_Get_Interface_Number(CLASS_TYPE_CDC_ACM, 0);

  /* Initialize the device cdc acm class */
  if (ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name,
                                     ux_device_class_cdc_acm_entry,
                                     cdc_acm_configuration_number,
                                     cdc_acm_interface_number,
                                     &cdc_acm_parameter) != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_DEVICE_CDC_ACM_REGISTER_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_DEVICE_CDC_ACM_REGISTER_ERROR */
  }

  /* Allocate the stack for device application main thread */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, UX_DEVICE_APP_THREAD_STACK_SIZE,
                       TX_NO_WAIT) != TX_SUCCESS)
  {
    /* USER CODE BEGIN MAIN_THREAD_ALLOCATE_STACK_ERROR */
    return TX_POOL_ERROR;
    /* USER CODE END MAIN_THREAD_ALLOCATE_STACK_ERROR */
  }

  /* Create the device application main thread */
  if (tx_thread_create(&ux_device_app_thread, UX_DEVICE_APP_THREAD_NAME, app_ux_device_thread_entry,
                       0, pointer, UX_DEVICE_APP_THREAD_STACK_SIZE, UX_DEVICE_APP_THREAD_PRIO,
                       UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD, UX_DEVICE_APP_THREAD_TIME_SLICE,
                       UX_DEVICE_APP_THREAD_START_OPTION) != TX_SUCCESS)
  {
    /* USER CODE BEGIN MAIN_THREAD_CREATE_ERROR */
    return TX_THREAD_ERROR;
    /* USER CODE END MAIN_THREAD_CREATE_ERROR */
  }

  /* USER CODE BEGIN MX_USBX_Device_Init1 */
  /* 初始化 CDC 监视线程 */
  /* Allocate the stack for usbx cdc acm thread */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, UX_DEVICE_APP_THREAD_STACK_SIZE, TX_NO_WAIT);
  if (ret != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Create the usbx_cdc_acm_thread_entry thread */
  ret = tx_thread_create(&ux_cdc_acm_thread, "cdc_acm_usbx_app_thread_entry",
          usbx_cdc_acm_thread_entry, 1, pointer,
          UX_DEVICE_APP_THREAD_STACK_SIZE, 20, 20, TX_NO_TIME_SLICE,
          TX_AUTO_START);
  if (ret != TX_SUCCESS)
  {
	return TX_THREAD_ERROR;
  }

  /* 初始化 CDC 命令拆包解析线程 */
  /* Allocate the stack for usbx cdc acm test thread */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, UX_DEVICE_APP_THREAD_STACK_SIZE, TX_NO_WAIT);
  if (ret != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Create the usbx_cdc_acm_thread_entry thread */
  ret = tx_thread_create(&ux_cdc_acm_cmd_parse_thread, "cdc_acm_cmd_parse_thread_entry",
		  usbx_cdc_acm_cmd_parse_thread_entry, 1, pointer,
          UX_DEVICE_APP_THREAD_STACK_SIZE, 20, 20, TX_NO_TIME_SLICE,
          TX_AUTO_START);
  if (ret != TX_SUCCESS)
  {
	return TX_THREAD_ERROR;
  }

  /* 初始化 MSC 变量*/
  ret = USBD_STORAGE_Pre_Init();
  if (ret != TX_SUCCESS)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /* 初始化 CDC 变量 */
  ret = USBD_CDC_ACM_Pre_Init();
  if (ret != TX_SUCCESS)
  {
	  _Error_Handler(__FILE__, __LINE__);
  }

  /* USER CODE END MX_USBX_Device_Init1 */

  return ret;
}

/**
  * @brief  Function implementing app_ux_device_thread_entry.
  * @param  thread_input: User thread input parameter.
  * @retval none
  */
static VOID app_ux_device_thread_entry(ULONG thread_input)
{
  /* USER CODE BEGIN app_ux_device_thread_entry */
  TX_PARAMETER_NOT_USED(thread_input);

  /* Initialization of USB device */
  USBX_APP_Device_Init();

  /* USER CODE END app_ux_device_thread_entry */
}

/* USER CODE BEGIN 2 */

/**
  * @brief  USBX_APP_Device_Init
  *         Initialization of USB device.
  * @param  none
  * @retval none
  */
VOID USBX_APP_Device_Init(VOID)
{
  /* USER CODE BEGIN USB_Device_Init_PreTreatment_0 */

  /* USER CODE END USB_Device_Init_PreTreatment_0 */

  /* USB_OTG_HS init function */
  MX_USB_OTG_HS_PCD_Init();

  /* USER CODE BEGIN USB_Device_Init_PreTreatment_1 */

  /**
   * USB FIFO 配置
   *   最大FIFO合计容量 0x1000 字，当前大小合计：0x412 字
   * 开启USB DMA后，FIFO 要留有余量
   *   https://forum.anfulai.cn/forum.php?mod=viewthread&tid=109916&highlight=USB%2BHS
   *   默认0x200参数过大，但修改为0x80小了也无响应，
   *   修改为0x180后DMA传输正常，1000个获取浮点数都能相应
   *   每个FIFO需要分配 24Btye DMA 描述符，三个FiFo需要72Byte，
   *   则能分配收发缓冲区最大为4096-72=4024Byte
   *
   * 通过CubeMX生成的宏配置端点FIFO大小：https://community.st.com/t5/stm32-mcus-products/nucleo-u5a5zj-q-usb-cdc-acm-issue-with-ux-device-class-cdc-acm/td-p/632086
   */
  // MSC 端点 FIFO 配置参考：案例 3：大容量存储：https://community.st.com/t5/stm32-mcus/practical-use-cases-to-manage-fifo-in-usb-otg-controllers-in/ta-p/839963
  /* Set Rx FIFO to accommodate 512 words*/
  HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200);                             //!<           大小：0x200 字：RX FIFO

  /* Set Tx FIFO 0 size to 16 words (64 bytes) for Control IN endpoint (EP0). */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, USBD_MAX_EP0_SIZE / 4);          //!< 端点号0 ：大小：0x10  字：IN 控制端点 FIFO
  
  /* Set Tx FIFO 1 to 256 words (1KB) for Bulk IN endpoint */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, USBD_MSC_EPIN_HS_MPS / 4);       //!< 端点号1 ：大小：0x100 字：MSC_ENDPOINT_IN / OUT FIFO

  // CDC ACM 端点 FIFO 配置参考：https://github.com/STMicroelectronics/x-cube-azrtos-h7/blob/main/Projects/NUCLEO-H723ZG/Applications/USBX/Ux_Device_CDC_ACM/USBX/App/app_usbx_device.c
  /* Set Tx FIFO 2 */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, USBD_CDCACM_EPINCMD_HS_MPS / 4); //!< 端点号2： 大小：0x2   字：CDC ACM CMD FIFO

  /* Set Tx FIFO 3 */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, USBD_CDCACM_EPIN_HS_MPS / 4);    //!< 端点号3： 大小：0x100 字：CDC ACM BULK FIFO


  /* USER CODE END USB_Device_Init_PreTreatment_1 */

  /* Initialize and link controller HAL driver */
  // 注册STM32 HS 到USBX协议栈并初始化
  ux_dcd_stm32_initialize((ULONG)USB_OTG_HS, (ULONG)&hpcd_USB_OTG_HS);

  /* Start USB device */
  HAL_PCD_Start(&hpcd_USB_OTG_HS);

  /* USER CODE BEGIN USB_Device_Init_PostTreatment */

  /* USER CODE END USB_Device_Init_PostTreatment */
}

UINT USBD_STORAGE_Pre_Init(VOID)
{
    UINT ret;

    /* 首次启动 USBX MSC    初始化事件标志组 */
    ret = tx_event_flags_create(&EventFlagMsc, "Event MSC Flag");
    if (ret != TX_SUCCESS)
    {
      _Error_Handler(__FILE__, __LINE__);
    }
    return ret;
}


UINT USBX_MSC_Pause(VOID)
{
    UINT status = UX_SUCCESS;

    status = ux_device_stack_class_unregister(_ux_system_slave_class_storage_name,
                                              ux_device_class_storage_entry);

    return status;
}

UINT USBX_MSC_Resume(VOID)
{
    UINT status = UX_SUCCESS;

    status = ux_device_stack_class_register(_ux_system_slave_class_storage_name,
                                            ux_device_class_storage_entry,
                                            storage_configuration_number,
                                            storage_interface_number,
                                            &storage_parameter);

    return status;
}

/**
  * @brief  强制重新枚举 USB 设备（断开并重新连接）
  *         注意：这会导致 CDC 也短暂断开，但会自动恢复
  */
VOID USBX_Force_Reconnect(VOID)
{
    extern PCD_HandleTypeDef hpcd_USB_OTG_HS;
    
    /* 停止 USB 设备 */
    HAL_PCD_Stop(&hpcd_USB_OTG_HS);
    
    /* 等待主机检测到断开 */
    tx_thread_sleep(200);  // 200 ticks，约 2 秒（假设 1 tick = 10ms）
    
    /* 重新启动 USB 设备 */
    HAL_PCD_Start(&hpcd_USB_OTG_HS);
}

/* USER CODE END 2 */
