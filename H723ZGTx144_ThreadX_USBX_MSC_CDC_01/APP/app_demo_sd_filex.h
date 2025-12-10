/**
  ******************************************************************************
  * @file        app_demo_sd_filex.h
  * @author      OldGerman
  * @created on  2025年12月4日
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
#ifndef APP_DEMO_SD_FILEX_H_
#define APP_DEMO_SD_FILEX_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "app_threadx.h"
/* Exported types ------------------------------------------------------------*/
/* 命令字符枚举 */
typedef enum {
    CMD_DEMO_FIEX_UNKNOWN           = 0,
    CMD_DEMO_FIEX_VIEW_ROOT_DIR     = '1', // 查看根目录
    CMD_DEMO_FIEX_CREATE_NEW_FILE   = '2', // 创建新文件
    CMD_DEMO_FIEX_READ_FILE_DATA    = '3', // 读取文件
    CMD_DEMO_FIEX_CREATE_DIR        = '4', // 创建目录
    CMD_DEMO_FIEX_DELETE_DIR_FILE   = '5', // 删除目录/文件
    CMD_DEMO_FIEX_TEST_SPEED        = '6', // 速度测试
    CMD_DEMO_FIEX_OPEN_USB_STORAGE  = 'a', // 打开模拟U盘
    CMD_DEMO_FIEX_CLOSE_USB_STORAGE = 'b'  // 关闭模拟U盘
} Cmd_DemoFileXTypeDef;

/* Exported define -----------------------------------------------------------*/
#define SD_SHOW_HIDE 0x01   // 显示隐藏内容
#define FX_SD_MEDIA (&sdio_disk) // 放你的存储媒体结构体指针 比如 (&sdio_disk)
#define MAX_TRAVEL_DEPTH 256 // 最大遍历深度，深度越大，占用空间越大
#define _USE_UTF8_          // 使用UTF-8打印目录树，更美观

/* Exported macro ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
uint16_t sd_com_tree(char *path, uint8_t depth_max, uint8_t tree_opt, void *workspace_ptr, size_t workspace_size);
void SD_Tree_Root();
void fxSdTestSpeed(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_DEMO_SD_FILEX_H_ */
