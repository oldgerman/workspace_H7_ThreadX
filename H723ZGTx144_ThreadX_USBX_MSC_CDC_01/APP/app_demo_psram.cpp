/**
  ******************************************************************************
  * @file        app_demo_psram.cpp
  * @author      OldGerman
  * @created on  2025年12月5日
  * @brief       
  ******************************************************************************
  * @attention
  *
  * Copyright (C) 2022 OldGerman.
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see https://www.gnu.org/licenses/.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_demo_psram.h"
#include "main.h"
#include  <stdio.h>
#include  <stdlib.h>
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define PSRAM_Size 64*1024*1024  //32M字节 W9825G6KH
#define PSRAM_TEST_GET_TICK HAL_GetTick
#define PSRAM_TEST_NUM_BYTES 2
#if(PSRAM_TEST_NUM_BYTES == 1)
#define PSRAM_TEST_BUF_TYPE uint8_t
#elif(PSRAM_TEST_NUM_BYTES == 2)
#define PSRAM_TEST_BUF_TYPE uint16_t
#elif(PSRAM_TEST_NUM_BYTES == 4)
#define PSRAM_TEST_BUF_TYPE uint32_t
#elif(PSRAM_TEST_NUM_BYTES == 8)
#define PSRAM_TEST_BUF_TYPE uint64_t
#else
#endif
/* Private macro -------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Function implementations --------------------------------------------------*/
/******************************************************************************************************
*   函 数 名: SDRAM_Test
*   入口参数: 无
*   返 回 值: SUCCESS - 成功，ERROR - 失败
*   函数功能: SDRAM测试
*   说    明: 先以16位的数据写入数据，再读取出来一一进行比较，随后以8位的数据写入，
*                用以验证NBL0和NBL1两个引脚的连接是否正常。
*******************************************************************************************************/
uint8_t PSRAM_Test(uint32_t OCTOSPIx_BASE_ADDR)
{
    uint32_t i = 0;         // 计数变量
    PSRAM_TEST_BUF_TYPE ReadData = 0;  // 读取到的数据
    uint8_t  ReadData_8b;

    uint32_t ExecutionTime_Begin;       // 开始时间
    uint32_t ExecutionTime_End;         // 结束时间
    uint32_t ExecutionTime;             // 执行时间
    float    ExecutionSpeed;            // 执行速度

    printf("*******************************************************************\r\n");
    printf("进行速度测试>>>\r\n");

// 写入 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    ExecutionTime_Begin     = PSRAM_TEST_GET_TICK();    // 获取 systick 当前时间，单位ms

    for (i = 0; i < PSRAM_Size / PSRAM_TEST_NUM_BYTES; i++)
    {
        *(__IO PSRAM_TEST_BUF_TYPE*) (OCTOSPIx_BASE_ADDR + PSRAM_TEST_NUM_BYTES*i) = (PSRAM_TEST_BUF_TYPE)i;        // 写入数据
    }
    ExecutionTime_End       = PSRAM_TEST_GET_TICK();                                            // 获取 systick 当前时间，单位ms
    ExecutionTime  = ExecutionTime_End - ExecutionTime_Begin;               // 计算擦除时间，单位ms
    ExecutionSpeed = (float)PSRAM_Size /1024/1024 /ExecutionTime*1000 ;     // 计算速度，单位 MB/S

    printf("以%d字节写入数据，大小：%d MB，耗时: %ld ms, 写入速度：%.2f MB/s\r\n",
    		PSRAM_TEST_NUM_BYTES,
            PSRAM_Size/1024/1024,
            ExecutionTime,
            ExecutionSpeed);

//    printf("%.2f MB/s\r\n", ExecutionSpeed);

// 读取   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    ExecutionTime_Begin     = PSRAM_TEST_GET_TICK();    // 获取 systick 当前时间，单位ms

    for(i = 0; i < PSRAM_Size / PSRAM_TEST_NUM_BYTES; i++ )
    {
        ReadData = *(__IO PSRAM_TEST_BUF_TYPE*)(OCTOSPIx_BASE_ADDR + PSRAM_TEST_NUM_BYTES * i );  // 从SDRAM读出数据
    }
    ExecutionTime_End       = PSRAM_TEST_GET_TICK();                                            // 获取 systick 当前时间，单位ms
    ExecutionTime  = ExecutionTime_End - ExecutionTime_Begin;               // 计算擦除时间，单位ms
    ExecutionSpeed = (float)PSRAM_Size /1024/1024 /ExecutionTime*1000 ;     // 计算速度，单位 MB/S

    printf("读取数据完毕，大小：%d MB，耗时: %ld ms, 读取速度：%.2f MB/s\r\n",
            PSRAM_Size/1024/1024,
            ExecutionTime,
            ExecutionSpeed);
//    printf("%.2f MB/s\r\n", ExecutionSpeed);

// 数据校验 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    printf("*******************************************************************\r\n");
    printf("进行数据校验>>>\r\n");

    for(i = 0; i < PSRAM_Size / PSRAM_TEST_NUM_BYTES; i++ )
    {
        ReadData = *(__IO PSRAM_TEST_BUF_TYPE*)(OCTOSPIx_BASE_ADDR + PSRAM_TEST_NUM_BYTES * i );  // 从SDRAM读出数据
        if( ReadData != (PSRAM_TEST_BUF_TYPE)i )      //检测数据，若不相等，跳出函数,返回检测失败结果。
        {
            printf("PSRAM测试失败！！\r\n");
            return ERROR;    // 返回失败标志
        }
    }

    printf("%d字节读写通过，以1字节写入数据\r\n", PSRAM_TEST_NUM_BYTES);
    for (i = 0; i < 255; i++)
    {
        *(__IO uint8_t*) (OCTOSPIx_BASE_ADDR + i) =  (uint8_t)i;
    }
    printf("写入完毕，读取数据并比较...\r\n");
    for (i = 0; i < 255; i++)
    {
        ReadData_8b = *(__IO uint8_t*) (OCTOSPIx_BASE_ADDR + i);
        if( ReadData_8b != (uint8_t)i )      //检测数据，若不相等，跳出函数,返回检测失败结果。
        {
            printf("1字节读写测试失败！！\r\n");
            printf("请检查NBL0和NBL1的连接\r\n");
            return ERROR;    // 返回失败标志
        }
    }
    printf("1字节读写通过\r\n");
    printf("PSRAM读写测试通过，系统正常\r\n");
    return SUCCESS;  // 返回成功标志
}
