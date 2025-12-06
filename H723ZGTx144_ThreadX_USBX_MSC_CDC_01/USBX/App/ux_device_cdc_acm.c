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
#include <stdarg.h>  // 新增：包含可变参数头文件
#include "ascii_protocol.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* define for TX_WAIT_FOREVER */
#define USB_CDC_WRITE_TIMEOUT   ( 1 * TX_TIMER_TICKS_PER_SECOND )

#define CMD_END_CHAR '\n'  // 命令结束符（根据上位机配置，也可设为'\r'或"\r\n"）
#define MAX_CMD_LEN 64     // 最大命令长度（适配你的命令格式，足够存储"$TEST_PSRAM"等）

// 新增：环形缓冲配置（根据需求调整大小）
#define RING_BUF_SIZE 1024  // 环形缓冲总大小（能存2个HS最大包+额外数据）

/* defines for EVENT flags */
/* 复用同一个 EventFlagCdcAcm，不同位区分收发 */
typedef enum
{
	CDC_ACM_NO_EVENT   = 0x00UL, /* 无事件（初始状态） */
    CDC_ACM_WRITE_DONE = 0x01UL, /* 写完成（_write 等待） */
    CDC_ACM_READ_AVAIL = 0x02UL  /* 读可用（_read 等待） */
} CDC_ACM_EVENT_FLAG_E;

/* 发送状态枚举 */
typedef enum
{
    TX_NO_EVENT = 0x00, /* 无事件（初始状态） */
    TX_COMPLETE = 0x01, /* 已发送完毕 */
} TX_BUF_STATE_E;

/* 接收缓冲区状态枚举 */
typedef enum
{
    RX_BUF_FREE = 0x00, /* 接收缓冲区空闲 */
    RX_BUF_USED = 0x01  /* 接收缓冲区已存数据 */
} RX_BUF_STATE_E;



/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

UX_SLAVE_CLASS_CDC_ACM  *cdc_acm;  //!< CDC Class object


/* User defined structure and variables for application, to buffer any received data */
TX_MUTEX    g_tx_buf_mutex;     //!< 送缓冲区互斥锁，保护多线程写，保护发送缓冲区指针操作

// 新增：环形缓冲变量
typedef struct {
    uint8_t buf[RING_BUF_SIZE];
    uint16_t head;  // 写入指针
    uint16_t tail;  // 读取指针
    uint16_t len;   // 当前数据长度
} ring_buf_t;
ring_buf_t g_ring_buf = {0};
TX_MUTEX g_ring_buf_mutex;  // 环形缓冲互斥锁

// 定义全局事件（用于 bulkin 内部 线程 等待写完成，替代之前的EventFlagCdcAcm）
TX_EVENT_FLAGS_GROUP CDC_Write_Complete_Event;


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
ssize_t _write(int fd, const void *buf, size_t nbytes);
ssize_t _read(int fd, void *buf, size_t count);
extern void OnAsciiCmd(const char* _cmd, size_t _len, StreamSink _responseChannel);  // 新增声明
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 新增：环形缓冲写入函数（接收数据时调用）
static void ring_buf_write(uint8_t *data, uint16_t len)
{
    tx_mutex_get(&g_ring_buf_mutex, TX_WAIT_FOREVER);
    for (uint16_t i = 0; i < len; i++)
    {
        g_ring_buf.buf[g_ring_buf.head] = data[i];
        g_ring_buf.head = (g_ring_buf.head + 1) % RING_BUF_SIZE;
        if (g_ring_buf.len < RING_BUF_SIZE)
        {
            g_ring_buf.len++;
        }
        else
        {
            g_ring_buf.tail = (g_ring_buf.tail + 1) % RING_BUF_SIZE;  // 溢出时覆盖 oldest 数据
        }
    }
    tx_mutex_put(&g_ring_buf_mutex);
}

// 新增：环形缓冲读取函数（测试线程中调用）
static uint16_t ring_buf_read(uint8_t *buf, uint16_t max_len)
{
    tx_mutex_get(&g_ring_buf_mutex, TX_WAIT_FOREVER);
    uint16_t read_len = 0;
    while (g_ring_buf.len > 0 && read_len < max_len)
    {
        buf[read_len] = g_ring_buf.buf[g_ring_buf.tail];
        g_ring_buf.tail = (g_ring_buf.tail + 1) % RING_BUF_SIZE;
        g_ring_buf.len--;
        read_len++;
    }
    tx_mutex_put(&g_ring_buf_mutex);
    return read_len;
}

/**
  * @brief  USBD_CDC_ACM_Pre_Init
  * @param  none
  * @retval status
  */
UINT USBD_CDC_ACM_Pre_Init(VOID)
{
    UINT ret;

    // 初始化 CDC 事件标志组
    ret = tx_event_flags_create(&EventFlagCdcAcm, "Event CDC ACM Flag");
    if (ret != UX_SUCCESS)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    // 初始 发送互斥锁
    ret = tx_mutex_create(&g_tx_buf_mutex, "TX Buf Mutex", TX_NO_INHERIT);
    if (ret != UX_SUCCESS)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    // 新增：初始化环形接收缓冲区和互斥锁
    ret = tx_mutex_create(&g_ring_buf_mutex, "Ring Buf Mutex", TX_NO_INHERIT);
    if (ret != UX_SUCCESS)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
    // 初始化写完成事件组
    ret = tx_event_flags_create(&CDC_Write_Complete_Event, "CDC_Write_Complete");
    if (ret != UX_SUCCESS)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /* 初始化命令解析注册 */
    initCommandRegistry();
    return ret;
}


// 新增：USB回应函数（实现StreamSink的send接口）
static void usb_response_send(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, args);  // 格式化回应内容
    va_end(args);

    // 通过USB发送回应（调用你的零拷贝_write函数）
    _write(1, buf, strlen(buf));
}

// 新增：命令拆包 + 解析线程入口
void usbx_cdc_acm_cmd_parse_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);
    char cmd_buf[MAX_CMD_LEN] = {0};
    uint16_t cmd_idx = 0;  // 命令缓冲区指针
    uint8_t read_byte;
    uint16_t read_len;

    // 初始化回应通道（绑定USB发送）
    usb_response_channel.send = usb_response_send;

    // 等待USB和环形缓冲就绪
    while (cdc_acm == UX_NULL)
    {
        tx_thread_sleep(100);
    }

    while (1)
    {
        // 非阻塞读取环形缓冲（1字节）
        read_len = ring_buf_read(&read_byte, 1);
        if (read_len == 1)
        {
            // 按结束符拆包（忽略'\r'，只认'\n'为结束）
            if (read_byte == '\r') continue;
            if (read_byte == CMD_END_CHAR || cmd_idx >= MAX_CMD_LEN - 1)
            {
                // 命令完整或缓冲区满，添加结束符
                cmd_buf[cmd_idx] = '\0';
                // 调用你的命令解析函数
                OnAsciiCmd(cmd_buf, cmd_idx, usb_response_channel);
                // 重置命令缓冲区
                cmd_idx = 0;
                memset(cmd_buf, 0, sizeof(cmd_buf));
            }
            else
            {
                // 未到结束符，继续填充命令缓冲区
                cmd_buf[cmd_idx++] = read_byte;
            }
        }
        else
        {
            // 无数据时休眠，降低CPU占用
            tx_thread_sleep(10);
        }
    }
}
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
      ret = tx_mutex_put(&g_tx_buf_mutex);

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
 * @brief comms_thread_entry 作为 USB CDC ACM 的后台线程，
 * 负责USB 连接状态监控
 * 零拷贝仅优化数据传输路径，无法避免 USB 意外断开（如物理拔插、总线错误）。
 * 线程中对 cdc_acm == UX_NULL 的检查仍有必要，可用于：
 * 检测到断开后，及时清理缓冲区状态（如将 BUSY 状态强制置为 FREE，避免资源泄漏）；
 * 通知应用层暂停数据传输，防止无效的指针操作（避免野指针访问）。
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
    	// 后台检查 USB 连接状态（零拷贝中仍需保留，增加状态清理）
    	if (cdc_acm == UX_NULL)
    	{

    	    tx_thread_sleep(10); // 休眠重试
    	    continue;
    	}

#if 0 // 以后考虑实现
        // 新增：检测发送缓冲区是否长期 busy（如超过 1s 未释放）

        tx_mutex_get(&g_tx_buf_mutex, TX_WAIT_FOREVER);
        for (uint8_t i = 0; i < 2; i++)
        {
            if (g_tx_buf[i].status == TX_BUF_BUSY)
            {
                // 假设记录每个缓冲区进入 busy 的时间戳，超过阈值则强制释放
                if (tx_time_get() - g_tx_buf[i].busy_timestamp > 1000) // 1s 阈值
                {
                    g_tx_buf[i].status = TX_BUF_FREE; // 强制释放，避免死锁
                }
            }
        }
        tx_mutex_put(&g_tx_buf_mutex);

        // 新增：检测接收送缓冲区是否长期 busy（如超过 1s 未释放）
#endif

        // 3. 线程心跳，可通过日志或外部监控判断线程是否正常运行，便于调试
        tx_thread_sleep(100); // 降低线程占用率
    }
}

/**
 * @brief  USB CDC ACM 测试线程：每1秒执行一次读写测试
 * @param  thread_input: 线程输入参数（未使用）
 */
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

    const uint8_t *data = (const uint8_t *)buf;
    size_t total_sent = 0;
    UINT ret = TX_SUCCESS;

    // 若有多个线程调用_write，此处加互斥锁（否则删除以下2行）
    tx_mutex_get(&g_tx_buf_mutex, TX_WAIT_FOREVER);

    while (total_sent < nbytes) {
        // 栈上临时缓冲区（匹配高速端点512字节）
        uint8_t send_buf[512];
        size_t chunk_len = (nbytes - total_sent) > sizeof(send_buf) ? sizeof(send_buf) : (nbytes - total_sent);
        memcpy(send_buf, data + total_sent, chunk_len);

        // 发起写请求（USBX内部线程会处理发送）
        ret = _ux_device_class_cdc_acm_write_with_callback(cdc_acm, send_buf, chunk_len);
        if (ret != UX_SUCCESS) {
            printf("Write request failed! ret=%d\n", ret);
            errno = EIO;
            // 释放互斥锁
            tx_mutex_put(&g_tx_buf_mutex);
            return total_sent;
        }

        // 阻塞等待写完成（回调触发事件）
        ULONG actual_flags;
        ret = tx_event_flags_get(&CDC_Write_Complete_Event,
                                TX_COMPLETE,
                                TX_OR_CLEAR,
                                &actual_flags,
                                1000);
        if (ret != TX_SUCCESS) {
            printf("Wait write complete timeout! ret=%d\n", ret);
            errno = ETIMEDOUT;
            // 释放互斥锁
            tx_mutex_put(&g_tx_buf_mutex);
            return total_sent;
        }

        total_sent += chunk_len;
    }

    // 释放互斥锁
    tx_mutex_put(&g_tx_buf_mutex);
    return total_sent;
}


// 重定义 _read（补充互斥锁保护）
ssize_t _read(int fd, void *buf, size_t count)
{
    if (fd != 0 || buf == NULL || cdc_acm == UX_NULL) {
        errno = EBADF;
        return -1;
    }

    ULONG actual_flags;
    UINT ret;
    // 循环等待“读可用”事件（避免递归）
    while (1) {
        ret = tx_event_flags_get(&EventFlagCdcAcm,
                                CDC_ACM_READ_AVAIL,
                                TX_OR_CLEAR,
                                &actual_flags,
                                TX_WAIT_FOREVER);
        if (ret == TX_SUCCESS) {
            // 读取环形缓冲数据（返回实际读取长度）
            return ring_buf_read((uint8_t *)buf, count);
        }
        // 事件获取失败：短暂休眠后重试（避免CPU空转）
        tx_thread_sleep(10);
    }
}

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
    if (status != UX_SUCCESS || length == 0 || data_pointer == NULL) {
        return result;
    }

    // 1. 先将数据写入环形缓冲（保留原有逻辑）
    ring_buf_write(data_pointer, length);

    // 2. 新增：设置“数据到达”事件标志（通知线程处理）
    tx_event_flags_set(&EventFlagCdcAcm, CDC_ACM_READ_AVAIL, TX_OR);  // TX_OR：保留已有标志

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


// 写回调：仅触发“写完成事件”（线程已重置标志，无需额外操作）
UINT USBD_CDC_ACM_Write_Callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, ULONG length)
{
    UX_PARAMETER_NOT_USED(cdc_acm);
    UX_PARAMETER_NOT_USED(length);

    // 仅触发写完成事件，通知_write继续
    tx_event_flags_set(&CDC_Write_Complete_Event, TX_COMPLETE, TX_OR);
    return UX_SUCCESS;
}


/* USER CODE END 2 */
