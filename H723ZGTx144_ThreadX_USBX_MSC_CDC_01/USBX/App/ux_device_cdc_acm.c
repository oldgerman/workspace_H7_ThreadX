/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_cdc_acm.c
  * @author  MCD Application Team
  * @brief   USBX Device CDC ACM applicative source file
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
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "app_usbx_device.h"
#include <stdint.h>
#include <errno.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* defines for EVENT flags */
// 复用同一个 EventFlagCdcAcm，不同位区分收发
#define CDC_ACM_WRITE_DONE  (0x01UL) // 写完成（_write 等待）
#define CDC_ACM_READ_AVAIL  (0x02UL) // 读可用（_read 等待）

/* define for TX_WAIT_FOREVER */
#define USB_CDC_WRITE_TIMEOUT   ( 1 * TX_TIMER_TICKS_PER_SECOND )

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

UX_SLAVE_CLASS_CDC_ACM  *cdc_acm;


/* User defined structure and variables for application, to buffer any received data */
typedef struct st_read_buffer {
    uint8_t data[USBD_CDCACM_EPIN_HS_MPS];     /* 适配HS模式，FS模式则配置为 USBD_CDCACM_EPIN_FS_MPS */
    uint8_t length;                            /* 实际接收长度（≤512） */
}read_buffer_t;

/* Create double buffer for data reception */
__attribute__((section(".axisram2_bss"), aligned(4)))
read_buffer_t g_rec_buffer[2];                 /* 双缓冲区，每个缓冲区都能容纳HS最大包512B */

/* Global variable for switching data buffers */
uint8_t       g_rec_buffer_index = 0;          /* 双缓冲区切换标志 */

TX_MUTEX g_cdc_tx_mutex;       // 发送缓冲互斥锁（保护多线程写）
TX_QUEUE g_usb_receive_queue;  // 需要在 app_usbx_device里初始化后使用，需要根据实际需求调整队列大小等参数



// 默认 LineCoding
__attribute__((section(".axisram2_data"), aligned(4), used))
UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER CDC_VCP_LineCoding =
{
  115200, /* baud rate */
  0x00,   /* stop bits-1 */
  0x00,   /* parity - none */
  0x08    /* nb. of bits 8 */
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  USBD_CDC_ACM_Activate
  *         This function is called when insertion of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Activate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  UINT ret = UX_SUCCESS;

  /* Save the CDC instance */
  cdc_acm = (UX_SLAVE_CLASS_CDC_ACM*) cdc_acm_instance;
  if (cdc_acm == UX_NULL)
  {
    _Error_Handler(__FILE__, __LINE__);
  }


  /* Set device class_cdc_acm with default CDC_VCP_LineCoding parameters */
  ret = ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
                                    &CDC_VCP_LineCoding);
  if (ret != UX_SUCCESS)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /* 启动 CDC ACM 传输（TRANSMISSION_START） */
  ret = ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START,
                                      &cdc_acm_callback_parameter);
  if (ret != UX_SUCCESS)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

//  // 创建队列：名称、存储区、消息大小（每个消息是 ULONG 类型）、队列长度
//  ret = tx_queue_create(
//      &g_usb_receive_queue,          // 队列控制块
//      "USB Receive Queue",           // 队列名称（调试用）
//      sizeof(ULONG),                 // 每个消息的大小（这里传递的是缓冲区地址，类型为 ULONG）
//      g_usb_queue_storage,           // 队列存储区（必须是静态/全局内存）
//      sizeof(g_usb_queue_storage)    // 存储区总大小（= 消息大小 × 队列长度）
//  );
//  if (ret != TX_SUCCESS) {
//	  _Error_Handler(__FILE__, __LINE__);
//  }

//  /* 首次启动 USBX CDC ACM 释放事件标志组 */
//  ret = tx_event_flags_create(&EventFlagCdcAcm, "Event CDC ACM Flag");
//  if (ret != UX_SUCCESS)
//  {
//	// 事件标志组创建失败，回滚传输状态（停止传输）
//	(VOID)ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL);
//	cdc_acm = UX_NULL; // 重置实例
//	_Error_Handler(__FILE__, __LINE__);
//  }
  /* USER CODE END USBD_CDC_ACM_Activate */

  return;
}

/**
  * @brief  USBD_CDC_ACM_Deactivate
  *         This function is called when extraction of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Deactivate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  UINT ret = UX_SUCCESS;

  if (cdc_acm_instance == cdc_acm && cdc_acm != UX_NULL)
  {
      // 1. 停止传输
      ret = ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL);

      // 2. 释放互斥锁（若被占用，避免下次激活时死锁）
      ret = tx_mutex_put(&g_cdc_tx_mutex);

      // 3. 销毁事件标志组
      ret = tx_event_flags_delete(&EventFlagCdcAcm);

      // 4. 重置实例
      cdc_acm = UX_NULL;
  }
  UX_PARAMETER_NOT_USED(ret);
  /* USER CODE END USBD_CDC_ACM_Deactivate */

  return;
}

/**
  * @brief  USBD_CDC_ACM_ParameterChange
  *         This function is invoked to manage the CDC ACM class requests.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_ParameterChange */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  /* USER CODE END USBD_CDC_ACM_ParameterChange */

  return;
}

/* USER CODE BEGIN 2 */
/**
 * @brief comms_thread_entry 作为 USB CDC ACM 的后台线程，负责缓冲管理和异常处理（可选但推荐）：
 * 
 */
void usbx_cdc_acm_thread_entry(ULONG thread_input)
{
	ULONG  actual_flags;
    UX_PARAMETER_NOT_USED(thread_input);

    // 等待 CDC ACM 激活完成（可选，保证线程启动后 USB 已就绪）
    tx_event_flags_get(&EventFlagCdcAcm, 0x04UL, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

    while (1)
    {
        // 1. 后台检查 USB 连接状态（可选）
        if (cdc_acm == UX_NULL)
        {
            tx_thread_sleep(10); // USB 未就绪，休眠 10ms 重试
            continue;
        }

        // 2. 处理缓冲溢出（可选，清理异常未处理的接收缓冲）
        if (g_rec_buffer[0].length >= 512 || g_rec_buffer[1].length >= 512)
        {
            g_rec_buffer[0].length = 0;
            g_rec_buffer[1].length = 0;
        }

        // 3. 线程心跳（可选，打印/监控）
        tx_thread_sleep(100); // 降低线程占用率
    }
}

/**
 * @brief  USB CDC ACM 测试线程：每1秒执行一次读写测试
 * @param  thread_input: 线程输入参数（未使用）
 */
#if 0
void usbx_cdc_acm_test_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);
    char input_buf[32];  // 用于存储输入的缓冲区

    // 等待USB CDC ACM初始化完成（确保cdc_acm已就绪）
    while (cdc_acm == UX_NULL)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);  // 等待100ms重试
    }

    while (1)
    {
        // 每1秒执行一次测试
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);  // 休眠1秒

        // 测试逻辑：输出提示 -> 读取输入 -> 回显
        printf("USB CDC ACM Test: Input a string\r\n");  // 输出提示到USB

        // 读取输入（注意：scanf("%s")会在空格/回车处停止，且不检查缓冲区溢出）
        // scanf阻塞线程：scanf是阻塞函数，会暂停线程执行，直到从_read获取到有效输入（用户发送数据），
        // 这个等待时间不受 1 秒周期控制（比如用户 5 秒后输入，线程就卡 5 秒）
        if (scanf("%31s", input_buf) == 1)  // 限制最大输入31字符（留1位给'\0'）
        {
            printf("You input: %s\r\n", input_buf);  // 回显输入内容
        }
        else
        {
            printf("Read input failed\r\n");  // 读取失败提示
        }
    }
}
#else
void usbx_cdc_acm_test_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);
    char input_buf[32];
    ULONG buffer_addr;
    read_buffer_t *recv_buf;

    // 等待USB就绪
    while (cdc_acm == UX_NULL)
    {
        tx_thread_sleep(100);  // 100ms重试
    }

    // 关键：设置stdout无缓冲，避免printf内容滞留（可选但推荐）
    setvbuf(stdout, NULL, _IONBF, 0);

    while (1)
    {
        // 1. 每1秒打印一次提示（核心：先打印再处理输入，保证周期）
        printf("USB CDC ACM Test: Input a string\r\n");

        // 2. 非阻塞读取输入（超时1秒，避免阻塞线程）
        UINT ret = tx_queue_receive(&g_usb_receive_queue, &buffer_addr, 1000);  // 超时1秒（单位：tick，1秒=TX_TIMER_TICKS_PER_SECOND）
        if (ret == TX_SUCCESS)
        {
            // 从接收缓冲区获取数据
            recv_buf = (read_buffer_t *)buffer_addr;
            if (recv_buf->length > 0 && recv_buf->length < 32)  // 确保不溢出
            {
                // 复制数据并添加字符串结束符
                memcpy(input_buf, recv_buf->data, recv_buf->length);
                input_buf[recv_buf->length] = '\0';

                // 回显输入内容
                printf("You input: %s\r\n", input_buf);

                // 清空接收缓冲区
                recv_buf->length = 0;
            }
        }
        // else：超时（1秒内无输入），直接进入下一次循环，维持1秒周期

        // 3. 无需额外休眠！因为tx_queue_receive已超时1秒，刚好匹配周期
    }
}
#endif
/**
 *
	USB2.0 HS 半双工：_write/_read 必须共用 EventFlagCdcAcm，但需用不同标志位区分
	1. 核心结论
	✅ 必须共用同一个 EventFlagCdcAcm，但要给 “写完成” 和 “读可用” 分配独立的标志位（如 0x01 写、0x02 读），而非共用同一位。
	2. 核心原因（USB2.0 HS 半双工的关键约束）
	USB2.0 HS 是半双工：同一时间只能单向传输（要么收、要么发），无法同时收发；
	共用 EventFlagCdcAcm 能天然避免收发并行冲突：
	若 _write 正在等待 0x01（写完成），_read 等待 0x02（读可用），ThreadX 的事件标志组会保证 “同一时间只有一个事件被触发”，适配半双工特性；
	若分开创建两个事件标志组，可能导致 _write 和 _read 同时触发，违反半双工规则，引发 USB 传输错误（如包丢失、总线冲突）
 */


// 重定义 _write（补充互斥锁保护）
ssize_t _write(int fd, const void *buf, size_t nbytes)
{
    if (fd != 1 && fd != 2) { errno = EBADF; return -1; }
    if (nbytes == 0 || buf == NULL || cdc_acm == UX_NULL) { return 0; }

    // 加互斥锁：禁止 _read 同时执行（适配半双工）
    if (tx_mutex_get(&g_cdc_tx_mutex, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        errno = EIO;
        return -1;
    }

    // 原有写逻辑（等待 CDC_ACM_WRITE_DONE）
    ULONG actual_flags;
    size_t send_len = 0;
    const uint8_t *data = (const uint8_t *)buf;
    while (send_len < nbytes)
    {
        size_t chunk_len = (nbytes - send_len) > 512 ? 512 : (nbytes - send_len);
//        if (ux_device_class_cdc_acm_write(cdc_acm, (UCHAR *)(data + send_len), chunk_len, UX_NULL) != UX_SUCCESS)
        if (ux_device_class_cdc_acm_write_with_callback(cdc_acm, (UCHAR *)(data + send_len), chunk_len) != UX_SUCCESS)
        {
            tx_mutex_put(&g_cdc_tx_mutex); // 失败释放锁
            errno = EIO;
            return -1;
        }
        // 等待 写完成 标志（共用 EventFlagCdcAcm）
        if (tx_event_flags_get(&EventFlagCdcAcm, CDC_ACM_WRITE_DONE, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER) != TX_SUCCESS)
        {
            tx_mutex_put(&g_cdc_tx_mutex); // 失败释放锁
            errno = EIO;
            return -1;
        }
        send_len += chunk_len;
    }

    tx_mutex_put(&g_cdc_tx_mutex); // 释放锁
    return send_len;
}

// 重定义 _read（补充互斥锁保护）
#if 0
ssize_t _read(int fd, void *buf, size_t nbytes)
{
    if (fd != 0) { errno = EBADF; return -1; }
    if (nbytes == 0 || buf == NULL || cdc_acm == UX_NULL) { return 0; }

    // 加互斥锁：禁止 _write 同时执行（适配半双工）
    if (tx_mutex_get(&g_cdc_tx_mutex, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        errno = EIO;
        return -1;
    }

    // 原有读逻辑（等待 CDC_ACM_READ_AVAIL）
    ULONG actual_flags;
    if (tx_event_flags_get(&EventFlagCdcAcm, CDC_ACM_READ_AVAIL, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        tx_mutex_put(&g_cdc_tx_mutex); // 失败释放锁
        errno = EIO;
        return -1;
    }

    uint8_t curr_idx = 1 - g_rec_buffer_index;
    read_buffer_t *recv_buf = &g_rec_buffer[curr_idx];
    size_t read_len = recv_buf->length > nbytes ? nbytes : recv_buf->length;
    memcpy(buf, recv_buf->data, read_len);
    recv_buf->length = 0;

    tx_mutex_put(&g_cdc_tx_mutex); // 释放锁
    return read_len;
}
#else
ssize_t _read(int fd, void *buf, size_t nbytes)
{
    if (fd != 0) { errno = EBADF; return -1; }
    if (nbytes == 0 || buf == NULL || cdc_acm == UX_NULL) { return 0; }

    // ===================== 关键修改2：延长互斥锁超时为永久等待 =====================
    if (tx_mutex_get(&g_cdc_tx_mutex, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        errno = EIO;
        return -1;
    }

    // 原有读逻辑（等待 CDC_ACM_READ_AVAIL）
    ULONG actual_flags;
    // ===================== 关键修改3：延长事件超时为永久等待 =====================
    if (tx_event_flags_get(&EventFlagCdcAcm, CDC_ACM_READ_AVAIL, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        tx_mutex_put(&g_cdc_tx_mutex); // 失败释放锁
        errno = EIO;
        return -1;
    }

    uint8_t curr_idx = 1 - g_rec_buffer_index;
    read_buffer_t *recv_buf = &g_rec_buffer[curr_idx];

    // 校验缓冲区有有效数据
    if (recv_buf->length == 0)
    {
        tx_mutex_put(&g_cdc_tx_mutex);
        return 0;
    }

    // ===================== 关键修改4：预留\0空间，计算可读长度 =====================
    size_t read_len = 0;
    if (nbytes > 1) // 至少留1字节给\0，否则无法构成C字符串
    {
        read_len = (recv_buf->length < (nbytes - 1)) ? recv_buf->length : (nbytes - 1);
    }
    else
    {
        tx_mutex_put(&g_cdc_tx_mutex);
        return 0; // 缓冲区太小，无法存储有效字符串
    }

    // 复制数据到用户缓冲区
    memcpy(buf, recv_buf->data, read_len);
    // ===================== 关键修改5：添加字符串结束符\0 =====================
    ((char *)buf)[read_len] = '\0';

    // 清空已读取的缓冲区，避免重复读取
    recv_buf->length = 0;

    tx_mutex_put(&g_cdc_tx_mutex); // 释放锁
    return read_len;
}

#endif
/**
 * @brief 
 * 
 * @param cdc_acm 
 * @param status 
 * @param data_pointer 
 * @param length 
 * @return UINT 
 */
UINT USBD_CDC_ACM_Read_Callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length)
{
    UINT result = UX_SUCCESS;

    UX_PARAMETER_NOT_USED(cdc_acm);
    UX_PARAMETER_NOT_USED(status);

    /* Storage for the address value of the structure, passed into the queue */
    ULONG ptr_addr;

    /* Store the data into g_rec_buffer[n][] */
    for (uint32_t i = 0; i < length; i++)
    {
        g_rec_buffer[g_rec_buffer_index].data[i] = *data_pointer++;
    }

    /* Store the length of the data packet received */
    g_rec_buffer[g_rec_buffer_index].length = (uint8_t)length;

    // ===================== 关键修改1：触发读可用事件 =====================
    tx_event_flags_set(&EventFlagCdcAcm, CDC_ACM_READ_AVAIL, TX_OR);

    /* send the address of the buffer that has been populated */
    ptr_addr = (ULONG)&g_rec_buffer[g_rec_buffer_index];
    tx_queue_send(&g_usb_receive_queue, &ptr_addr, TX_NO_WAIT);

    /* switch buffer for next receive action */
    g_rec_buffer_index = (uint8_t)(1 - g_rec_buffer_index);


    return result;
}

/**
 * @brief usb cdc 发送完成回调函数
 * 
 * @param cdc_acm 
 * @param status 
 * @param length 
 * @return UINT 
 */
UINT USBD_CDC_ACM_Write_Callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, ULONG length)
{
    UINT result = UX_SUCCESS;
    UX_PARAMETER_NOT_USED(cdc_acm);
    UX_PARAMETER_NOT_USED(length);

    if (UX_SUCCESS == status)
    {
        result = tx_event_flags_set(&EventFlagCdcAcm,  CDC_ACM_WRITE_DONE, TX_OR);
    }

    return result;
}


/* USER CODE END 2 */
