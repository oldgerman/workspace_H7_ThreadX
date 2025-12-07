/**
  ******************************************************************************
  * @file        dynamic_ram.h
  * @author      OldGerman
  * @created on  Mar 20, 2023
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RAM_DYNAMIC_RAM_H_
#define RAM_DYNAMIC_RAM_H_

/* 调整包含顺序：先包含标准库头文件，再包含其他头文件 */
#include <stdint.h>  // 先包含标准库，确保size_t、uint32_t等类型已定义
#include "rtx_memory.h"

/* 修正extern "C"结构：C和C++编译器都能看到函数声明 */
#ifdef __cplusplus
extern "C" {  // C++编译器下，用extern "C"避免名字修饰（保证C文件能链接）
#endif

/* Exported functions --------------------------------------------------------*/
uint32_t DRAM_Init();
void* DRAM_SRAM1_aligned_4_malloc(size_t size);
uint32_t DRAM_SRAM1_aligned_free(void* ptr_aligned);

#ifdef __cplusplus
}  // 关闭extern "C"
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
#ifdef __cplusplus
//extern osRtxMemory DRAM_SRAM1;  // 仅C++需要的变量声明（C文件用不到）
#endif

#endif /* RAM_DYNAMIC_RAM_H_ */
