/**
  ******************************************************************************
  * @file    aps512xx_conf.h
  * @author  MCD Application Team
  * @brief   APS512XX 16bits-OSPI PSRAM memory configuration template file.
  *          This file should be copied to the application folder and renamed
  *          to aps512xx_conf.h
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APS512XX_CONF_H
#define APS512XX_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "binary.h"
/** @addtogroup BSP
  * @{
  */
#define CONF_HSPI_DS   APS512XX_MR0_DS_HALF
#define CONF_HSPI_PASR APS512XX_MR4_PASR_FULL
#define CONF_HSPI_RF   APS512XX_MR4_RF_4X

#define OSPI_CCR_SIOO
// Table 6: Operation Latency Code Table
#define DEFAULT_READ_LATENCY_CODE  APS512XX_READ_LATENCY_5
#define DEFAULT_WRITE_LATENCY_CODE APS512XX_WRITE_LATENCY_5

#define VL_CODES_LC   5  //!< Table 5 : Read Latency Codes: VL (default)Latency (LC) = 5 (default) @  Codes
#define VL_CODES_WLC  5  //!< Table 15: Write latency, WLC, is default to 5 after power up
// Default Latency after power up, see Table 6: Operation Latency Code Table
#define DEFAULT_MEM_READ_LATENCY_NO_REFRESH   VL_CODES_LC     //!<
#define DEFAULT_MEM_READ_LATENCY_REFRESH      (VL_CODES_LC*2) //!< Table 6: Operation Latency Code Table
#define DEFAULT_MEM_WRITE_LATENCY             VL_CODES_WLC    //!< Table 6: Operation Latency Code Table
#define DEFAULT_REG_READ_LATENCY              VL_CODES_LC     //!< Table 6: Operation Latency Code Table
#define DEFAULT_REG_WRITE_LATENCY             1               //!< Table 6: Operation Latency Code Table, Register Writes are always latency 1
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* APS512XX_CONF_H */
