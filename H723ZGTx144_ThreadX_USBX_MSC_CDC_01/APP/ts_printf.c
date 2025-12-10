/**
  ******************************************************************************
  * @file        ts_printf.c
  * @author      OldGerman
  * @created on  2025年11月30日
  * @brief       出处：安富莱 STM32-V7 开发板 ThreadX USBX 教程
  *              https://forum.anfulai.cn/forum.php?mod=viewthread&tid=108546)
  *              例程 V7-2401_ThreadX USBX Template
  *              App_Printf 函数做了信号量的互斥操作，解决资源共享问题
  ******************************************************************************
  * @attention
  *              本文件代码片段源于安富莱电子（www.armfly.com）开源示例工程修改
  *
  * Copyright (C), 2021-2030, 安富莱电子 www.armfly.com
  * Copyright (C) 2025 OldGerman
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
/* 标准库头文件（按C标准顺序） */
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

/* ThreadX 内核头文件（嵌入式RTOS依赖） */
#include "tx_api.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Function implementations --------------------------------------------------*/
/**
 * @brief     用于 printf 打印的互斥锁（线程安全打印）
 */
static TX_MUTEX AppPrintfSemp;

/**
 * @brief     线程安全的 printf 封装函数
 * @details   基于 ThreadX 互斥锁实现多线程打印互斥，避免打印内容交织乱码；
 *            内部使用固定缓冲区，需注意打印内容长度不超过缓冲区上限
 * @param fmt 格式化输出字符串（同 printf 格式）
 * @param ... 可变参数列表（同 printf 可变参数）
 * @return    无
 */
void AppPrintf(const char *fmt, ...)
{
	/* 打印缓冲区：预留 1 字节用于字符串结束符，避免越界 */
    char buf_str[200 + 1];
    va_list v_args;
    int ret;

    /* 解析可变参数并格式化到缓冲区 */
    va_start(v_args, fmt);
    ret = vsnprintf((char       *)&buf_str[0],
                    (size_t      ) sizeof(buf_str),
                    (char const *) fmt,
                                   v_args);
    va_end(v_args);

    /* 增加缓冲区溢出提示 */
    if (ret >= (int)sizeof(buf_str)) {
    	/* 互斥锁保护：独占 printf 打印，防止多线程抢占 */
        tx_mutex_get(&AppPrintfSemp, TX_WAIT_FOREVER);
        printf("[PRINTF WARN] Buffer overflow! Content truncated.\n");
        tx_mutex_put(&AppPrintfSemp);
    }
    /* 互斥锁保护：独占 printf 打印，防止多线程抢占 */
    tx_mutex_get(&AppPrintfSemp, TX_WAIT_FOREVER);
    printf("%s", buf_str);
    tx_mutex_put(&AppPrintfSemp);
}

