/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "app_threadx.h"
#include "main.h"
#include "octospi.h"
#include "sdmmc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "aps512xx.h"
#include "dynamic_ram.h"
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

/* USER CODE BEGIN PV */
HAL_SD_CardInfoTypeDef                     USBD_SD_CardInfo;

__attribute__((section (".psram_nold"))) uint8_t OSPI_Array[1024];
uint8_t aTxBuffer[] = "& OCTOSPI/Quad-spi PSRAM Memory-mapped base on H723ZGTx144 v1.0$";

// DTCM 前 0x400 空间用于存放从 FLASH 拷贝的中断向量表副本
__attribute__((section(".dtcmvtor_nold"), aligned(4))) uint8_t dtcm_isr_vector[1024];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
static void Configure_APMemory(void);
void InitDWT(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  UINT status;
  /* Operation interrupt vector table begin ----------------------------------*/
//#define INTERNAL_FLASH_BOOTLOADER_JUMP_TO_QSPI_FLASH_APP

#if defined(INTERNAL_FLASH_BOOTLOADER_JUMP_TO_QSPI_FLASH_APP)
	/*将当前内部闪存中使用的中断向量表更改为外部 QSPI 闪存中的中断向量表 */
	SCB->VTOR = (uint32_t *)QSPI_BASE;
#endif

#define COPY_VECTORTABLE_TO_DTCM

#if defined(COPY_VECTORTABLE_TO_DTCM)
#if defined(INTERNAL_FLASH_BOOTLOADER_JUMP_TO_QSPI_FLASH_APP)
	uint32_t *SouceAddr = (uint32_t *)QSPI_BASE;
#else
	uint32_t *SouceAddr = (uint32_t *)FLASH_BANK1_BASE;
#endif
	uint32_t *DestAddr = (uint32_t *)dtcm_isr_vector;

	/**
	 * memcpy 拷贝大小 0x400，即1KB，因为通过构建分析器查看 .isr_vector 大小为 716B，留有一定余量
	 * 此操作需要在链接文件中将 DTCM 从 0x200000000 偏移 1KB 开始 0x2000400;
	 */
	memcpy(DestAddr, SouceAddr, 0x400);

	/** 将当前中断向量表设置为从FLASH复制到DITCM中的副本 */
	SCB->VTOR = D1_DTCMRAM_BASE;
#endif
#if PREV_TEST_PSRAM
	__IO uint8_t *mem_addr;
	uint32_t address = 0;
	uint16_t index1;/*index1 counter of bytes used when reading/
	writing 256 bytes buffer */
	uint16_t index2;/*index2 counter of 256 bytes buffer used when reading/
	writing the 1Mbytes extended buffer */
#endif
  /* Operation interrupt vector table end ------------------------------------*/
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_TIM7_Init();
  MX_OCTOSPI1_Init();
  MX_SDMMC2_SD_Init();
  /* USER CODE BEGIN 2 */

  InitDWT();

  Configure_APMemory();


	if (APS512XX_EnableMemoryMappedMode(&hospi1, 8, 4, 0U) != HAL_OK) // 线性突发模式，执行跨边界，解决2K后地址回卷（Wrap）
	//	if (APS512XX_EnableMemoryMappedMode(&hospi1, 8, 4, APS512XX_WRITE_CMD) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

#if PREV_TEST_PSRAM //此段代码可能导致USB读写缓冲区中混入脏数据让电脑识别不到U盘
	// H723ZGTx144：测试：OCTOSPI_CLK=90MHz DTR MODE 1MB 误码率 0，若提高到100MHz误码率飙升
	int led = 0;
    uint64_t cnt_charW = 0;
	/*Writing 1Mbyte (256Byte BUFFERSIZE x 4096 times) */
	mem_addr = (__IO uint8_t *)(OCTOSPI1_BASE + address);
	for (index2 = 0; index2 < EXTENDEDBUFFERSIZE/BUFFERSIZE; index2++) {
		for (index1 = 0; index1 < BUFFERSIZE; index1++) {
			*mem_addr = aTxBuffer[index1];
			mem_addr++;
			cnt_charW++;
		}
	}

	/*Writing 1Mbyte (256Byte BUFFERSIZE x 4096 times) */
	uint64_t cnt_charR = 0;
    mem_addr = (__IO uint8_t *)(OCTOSPI1_BASE + address);
    for (index2 = 0; index2 < EXTENDEDBUFFERSIZE/BUFFERSIZE; index2++) {
		for (index1 = 0; index1 < BUFFERSIZE; index1++) {
			if (*mem_addr != aTxBuffer[index1]) {
				led++;
			}
			mem_addr++;
			cnt_charR++;
		}
    }
	uint8_t* p = OSPI_Array;
	p++;
	p--;
#endif

  /* Check if SD card is present */
  //if(HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_12) == GPIO_PIN_SET) { Error_Handler(); }
  /* Get SD card info 存到 hsd2 里
   * 供后续 USBD_STORAGE_GetMediaLastLba() 和 USBD_STORAGE_GetMediaBlocklength() 获取信息
   */
  status = HAL_SD_GetCardInfo(&hsd2, &USBD_SD_CardInfo);

  if (status != HAL_OK)
  {
    Error_Handler();
  }

  /* 内核开启前关闭HAL的时间基准 */
  HAL_SuspendTick();
  /* USER CODE END 2 */

  MX_ThreadX_Init();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 55;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 11;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void _Error_Handler(const char * file, uint32_t line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

/**
* @brief  Switch from Octal Mode to Hexa Mode on the memory
* @param  None
* @retval None
*/
uint8_t aps_ID[2] = {0, 0};
static void Configure_APMemory(void)
{
	/* MR0 register for read and write
	                                     MR8[7:0] = 0x24  =  B00100100
			   N/A                       MR0[7:6] = 00  --> constraint value
			   Read Latency Type         MR0[5]   = 1   --> LT               : Fixed ---> FL Codes
			   Read Latency Code         MR0[4:2] = 001 --> FL Codes (MR0[5]=1) Latency (LCx2) = 8
			   Drive Strength Codes      MR0[1:0] = 00  --> Drive StrengthFull  (25Ω default)
	*/
	uint8_t regW_MR0[2] = {0x24, 0x8D}; //!< To configure AP memory Latency Type and drive Strength
	uint8_t regR_MR0[2] = {0};
	/* MR8 register for read and write
	                                     MR8[7:0] = 0x0B  =  B00001011
       IO X8/X16 Mode                    MR8[6]   = 0    --> X8 Mode
       Row Boundary Crossing Read Enable MR8[3]   = 1    --> Allow reads cross page (row) boundary
       Burst Type                        MR8[2]   = 0    -->
       Burst Length                      MR8[1:0] = 11   --> 2K Byte/1K Word Wrap，对应 “2K Byte Wrap 突发模式”（地址超过 2K 会回卷）
	 */
	uint8_t regW_MR8[2] = {0x0B, 0x08};  //!< To configure AP memory Burst Type
	uint8_t regR_MR8[2] = {0};

	//复位所有寄存器配置
	if(APS512XX_Reset(&hospi1) != HAL_OK)
	{
	_Error_Handler(__FILE__, __LINE__);
	}
#if 1
	/*Configure Read Latency and drive Strength */
	if (APS512XX_WriteReg(&hospi1, APS512XX_MR0_ADDRESS, regW_MR0) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Check MR0 configuration */
	if (APS512XX_ReadReg(&hospi1, APS512XX_MR0_ADDRESS, regR_MR0, 5) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Check MR0 configuration */
	if (regR_MR0[0] != regW_MR0[0])
	{
		_Error_Handler(__FILE__, __LINE__);
	}
#endif
	/* Configure Burst Length */
	if (APS512XX_WriteReg(&hospi1, APS512XX_MR8_ADDRESS, regW_MR8) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Check MR8 configuration */
	if (APS512XX_ReadReg(&hospi1, APS512XX_MR8_ADDRESS, regR_MR8, 5) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	if (regR_MR8[0] != regW_MR8[0])
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	if(0){
		__NOP();
	}
}

/**
 * @brief 使能DWT时钟周期计时器
 */
#define  DWT_CYCCNT  *(volatile unsigned int *)0xE0001004
#define  DWT_CR      *(volatile unsigned int *)0xE0001000
#define  DEM_CR      *(volatile unsigned int *)0xE000EDFC
#define  DBGMCU_CR   *(volatile unsigned int *)0xE0042004

#define  DEM_CR_TRCENA               (1 << 24)
#define  DWT_CR_CYCCNTENA            (1 <<  0)

void InitDWT(void)
{
	DEM_CR         |= (unsigned int)DEM_CR_TRCENA;
	DWT_CYCCNT      = (unsigned int)0u;
	DWT_CR         |= (unsigned int)DWT_CR_CYCCNTENA;
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x6000000;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0x30004000;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64MB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.BaseAddress = 0x24010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
