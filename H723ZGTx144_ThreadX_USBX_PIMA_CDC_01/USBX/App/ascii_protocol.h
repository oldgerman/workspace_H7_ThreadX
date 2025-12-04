/**
  ******************************************************************************
  * @file        ascii_protocol.h
  * @author      OldGerman
  * @created on  2025年12月3日
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
#ifndef APP_ASCII_PROTOCOL_H_
#define APP_ASCII_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
// 回应通道结构体（绑定USB发送接口，传给OnAsciiCmd）
typedef struct {
    // 回应函数（通过USB发送回应内容）
    void (*send)(const char *fmt, ...);
} StreamSink;

/* Exported define -----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern StreamSink usb_response_channel;  // USB回应通道
/* Exported functions --------------------------------------------------------*/
void OnAsciiCmd(const char* _cmd, size_t _len, StreamSink _responseChannel);
void initCommandRegistry();

#ifdef __cplusplus
}
#endif

#endif /* APP_ASCII_PROTOCOL_H_ */
