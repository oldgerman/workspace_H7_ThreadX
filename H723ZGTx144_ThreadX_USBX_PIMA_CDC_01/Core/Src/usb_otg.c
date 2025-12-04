/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usb_otg.c
  * @brief   This file provides code for the configuration
  *          of the USB_OTG instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usb_otg.h"

/* USER CODE BEGIN 0 */
void USB_Status_Init(void);
__attribute__((section(".axisram2_bss"), aligned(4))) //!< 给下面的 hpcd_USB_OTG_HS 用

/* USER CODE END 0 */

PCD_HandleTypeDef hpcd_USB_OTG_HS;

/* USB_OTG_HS init function */

void MX_USB_OTG_HS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_HS_Init 0 */

  /* USER CODE END USB_OTG_HS_Init 0 */

  /* USER CODE BEGIN USB_OTG_HS_Init 1 */

  /* USER CODE END USB_OTG_HS_Init 1 */
  hpcd_USB_OTG_HS.Instance = USB_OTG_HS;
  hpcd_USB_OTG_HS.Init.dev_endpoints = 9;
  hpcd_USB_OTG_HS.Init.speed = PCD_SPEED_HIGH;
  hpcd_USB_OTG_HS.Init.dma_enable = ENABLE;
  hpcd_USB_OTG_HS.Init.phy_itface = USB_OTG_ULPI_PHY;
  hpcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_HS.Init.use_external_vbus = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_HS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_HS_Init 2 */

  /* USER CODE END USB_OTG_HS_Init 2 */

}

void HAL_PCD_MspInit(PCD_HandleTypeDef* pcdHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(pcdHandle->Instance==USB_OTG_HS)
  {
  /* USER CODE BEGIN USB_OTG_HS_MspInit 0 */

  /* USER CODE END USB_OTG_HS_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.PLL3.PLL3M = 25;
    PeriphClkInitStruct.PLL3.PLL3N = 96;
    PeriphClkInitStruct.PLL3.PLL3P = 2;
    PeriphClkInitStruct.PLL3.PLL3Q = 4;
    PeriphClkInitStruct.PLL3.PLL3R = 2;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_1;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

  /** Enable USB Voltage detector
  */
    HAL_PWREx_EnableUSBVoltageDetector();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USB_OTG_HS GPIO Configuration
    PC0     ------> USB_OTG_HS_ULPI_STP
    PC2_C     ------> USB_OTG_HS_ULPI_DIR
    PC3_C     ------> USB_OTG_HS_ULPI_NXT
    PA3     ------> USB_OTG_HS_ULPI_D0
    PA5     ------> USB_OTG_HS_ULPI_CK
    PB0     ------> USB_OTG_HS_ULPI_D1
    PB1     ------> USB_OTG_HS_ULPI_D2
    PB10     ------> USB_OTG_HS_ULPI_D3
    PB11     ------> USB_OTG_HS_ULPI_D4
    PB12     ------> USB_OTG_HS_ULPI_D5
    PB13     ------> USB_OTG_HS_ULPI_D6
    PB5     ------> USB_OTG_HS_ULPI_D7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_HS;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USB_OTG_HS clock enable */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

    /* USB_OTG_HS interrupt Init */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
  /* USER CODE BEGIN USB_OTG_HS_MspInit 1 */
    USB_Status_Init();
  /* USER CODE END USB_OTG_HS_MspInit 1 */
  }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef* pcdHandle)
{

  if(pcdHandle->Instance==USB_OTG_HS)
  {
  /* USER CODE BEGIN USB_OTG_HS_MspDeInit 0 */

  /* USER CODE END USB_OTG_HS_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_DISABLE();

    /**USB_OTG_HS GPIO Configuration
    PC0     ------> USB_OTG_HS_ULPI_STP
    PC2_C     ------> USB_OTG_HS_ULPI_DIR
    PC3_C     ------> USB_OTG_HS_ULPI_NXT
    PA3     ------> USB_OTG_HS_ULPI_D0
    PA5     ------> USB_OTG_HS_ULPI_CK
    PB0     ------> USB_OTG_HS_ULPI_D1
    PB1     ------> USB_OTG_HS_ULPI_D2
    PB10     ------> USB_OTG_HS_ULPI_D3
    PB11     ------> USB_OTG_HS_ULPI_D4
    PB12     ------> USB_OTG_HS_ULPI_D5
    PB13     ------> USB_OTG_HS_ULPI_D6
    PB5     ------> USB_OTG_HS_ULPI_D7
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_3|GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_5);

    /* USB_OTG_HS interrupt Deinit */
    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
  /* USER CODE BEGIN USB_OTG_HS_MspDeInit 1 */

  /* USER CODE END USB_OTG_HS_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
// USB ULPI的读写操作实现
// 硬汉论坛：https://forum.anfulai.cn/forum.php?mod=viewthread&tid=127602&highlight=USB%2BULPI%B5%C4%B6%C1%D0%B4%B2%D9%D7%F7%CA%B5%CF%D6
// 【使用】ST论坛H750+USB3300：https://community.st.com/t5/stm32-mcus-embedded-software/stm32h750-usb3300-no-signals-on-ulpi/td-p/225293

#define USBULPI_PHYCR     ((uint32_t)(0x40040000 + 0x034))
#define USBULPI_D07       ((uint32_t)0x000000FF)
#define USBULPI_New       ((uint32_t)0x02000000)
#define USBULPI_RW        ((uint32_t)0x00400000)
#define USBULPI_S_BUSY    ((uint32_t)0x04000000)
#define USBULPI_S_DONE    ((uint32_t)0x08000000)

#define USB_OTG_READ_REG32(reg)  (*(__IO uint32_t *)(reg))
#define USB_OTG_WRITE_REG32(reg,value) (*(__IO uint32_t *)(reg) = (value))

uint32_t USB_ULPI_Read(uint32_t Addr)
{
    __IO uint32_t val = 0;
    __IO uint32_t timeout = 1000; /* Can be tuned based on the Clock or etc... */
    __IO uint32_t bussy = 0;
    UNUSED(bussy);
    USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | (Addr << 16));
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
    while (((val & USBULPI_S_DONE) == 0) && (timeout--))
    {
        val = USB_OTG_READ_REG32(USBULPI_PHYCR);
        bussy = val & USBULPI_S_BUSY;
    }
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
    return  (val & 0x000000ff);
}

/**
  * @brief  Write CR value
  * @param  Addr the Address of the ULPI Register
  * @param  Data Data to write
  * @retval Returns value of PHY CR register
  */
uint32_t USB_ULPI_Write(uint32_t Addr, uint32_t Data)   /* Parameter is the Address of the ULPI Register & Date to write */
{
    __IO uint32_t val;
    __IO uint32_t timeout = 100;   /* Can be tuned based on the Clock or etc... */
    __IO uint32_t bussy = 0;

    USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | USBULPI_RW | (Addr << 16) | (Data & 0x000000ff));
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
    while (((val & USBULPI_S_DONE) == 0) && (timeout--))
    {
        val = USB_OTG_READ_REG32(USBULPI_PHYCR);
        bussy = val & USBULPI_S_BUSY;
    }
    UNUSED(bussy);  // 标记变量刻意未使用，消除警告
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
    return 0;
}

/**
  * @brief  Customize the disconnect function before USB initialization to reduce the trouble of reset
  * @notice Called within the "USER CODE BEGIN SysInit" section of main()
  */
void USB_Status_Init(void)
{
/*
《USB3300 Hardware Design Checklist》.PDF
    7.1 复位电路
    RESET 引脚是一个高电平有效的收发器复位。 RESET 引脚的使用是可选的。
    RESET 引脚上的逻辑高电平与写入功能控制寄存器的复位（地址 04h，bit5）相同。
    这不会重置 ULPI 寄存器组。 该引脚包括一个集成的接地下拉电阻。
    如果不使用，该引脚可以悬空，但建议将该引脚接地。
    如果 RESET 从外部源驱动为高电平，则逻辑高电平必须持续至少一个 CLKOUT 信号的完整周期。
    在 RESET 信号取反后的两个 CLKOUT 时钟周期内，没有其他 PHY 数字输入信号可以改变状态。

《https://ww1.microchip.com/downloads/en/DeviceDoc/00001783C.pdf》.PDF P21
    Field Name: Reset
    Bit:5
    Default: 0b
    Active high transceiver reset. This reset does not reset the ULPI
    interface or register set. Automatically clears after reset is
    complete.

 */
    uint32_t reg_val;  // 存储04h寄存器当前值

    // 步骤1：读取功能控制寄存器（04h）的当前值
    reg_val = USB_ULPI_Read(0x04);

    // 步骤2：置位bit5（触发复位，高电平有效）
    USB_ULPI_Write(0x04, reg_val | (1 << 5));

    // 步骤3：延迟，经测试0ms不识别，500ms不识别，1000ms正常
    // 若上电骇逝不识别出USB设备，则需要增加此延迟
    HAL_Delay(2000);
}
/* USER CODE END 1 */
