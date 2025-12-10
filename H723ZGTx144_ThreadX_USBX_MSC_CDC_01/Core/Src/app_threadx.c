/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* USER CODE BEGIN 1 */
/* USER CODE END 1 */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
TX_THREAD tx_app_thread;
/* USER CODE BEGIN PV */

/* 静态全局变量 --------------------------------------------------------------*/
static volatile float OSCPUUsage; /* CPU百分比 */

/* TCB */
static TX_THREAD AppTaskDemoFileXTCB;
/* Stack */
static uint64_t AppTaskDemoFileXStack[APP_CFG_TASK_DEMO_FILEX_STACK_SIZE / 8];
/* 全局变量 ------------------------------------------------------------------*/
/* Semaphore */

/* Event Group */

/* Mutex */

/* Queue */
TX_QUEUE g_cmd_demo_filex_queue;

/* Queue Buffer */
uint8_t g_cmd_demo_filex_queue_buffer[10 * sizeof(uint8_t)]; /* 缓存10个命令 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void AppObjCreate(void);
static void AppTaskCreate(void);
static void AppTaskDemoFileX(ULONG thread_input);

/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */

	/* 创建命令队列：名称、元素大小、存储区、存储区大小 */
	ret = tx_queue_create(&g_cmd_demo_filex_queue,       // 队列控制块
			"FileX Cmd Queue",// 队列名称
			sizeof(uint8_t),// 单个元素大小（存储命令字符）
			g_cmd_demo_filex_queue_buffer,// 队列存储区
			sizeof(g_cmd_demo_filex_queue_buffer)// 存储区总大小
			);
	if (ret != TX_SUCCESS) {
		while(1) {

		}
	}

  /* USER CODE END App_ThreadX_MEM_POOL */
  CHAR *pointer;

  /* Allocate the stack for tx app thread  */
  if (tx_byte_allocate(byte_pool, (VOID**) &pointer,
                       TX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Create tx app thread.  */
  if (tx_thread_create(&tx_app_thread, "tx app thread", tx_app_thread_entry, 0, pointer,
                       TX_APP_STACK_SIZE, TX_APP_THREAD_PRIO, TX_APP_THREAD_PREEMPTION_THRESHOLD,
                       TX_APP_THREAD_TIME_SLICE, TX_APP_THREAD_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN App_ThreadX_Init */
  /* USER CODE END App_ThreadX_Init */

  return ret;
}
/**
  * @brief  Function implementing the tx_app_thread_entry thread.
  * @param  thread_input: Hardcoded to 0.
  * @retval None
  */
void tx_app_thread_entry(ULONG thread_input)
{
  /* USER CODE BEGIN tx_app_thread_entry */
	EXECUTION_TIME TolTime, IdleTime, deltaTolTime, deltaIdleTime;
	uint32_t uiCount = 0;
	(void) thread_input;

	/* 内核开启后，恢复HAL里的时间基准 */
	HAL_ResumeTick();

	/* 外设初始化 */
//    bsp_Init();
	/* 创建App任务 */
	AppTaskCreate();

	/* 创建任务间通信机制 */
	AppObjCreate();

	/* 计算CPU利用率 */
	IdleTime = _tx_execution_idle_time_total;
	TolTime = _tx_execution_thread_time_total + _tx_execution_isr_time_total
			+ _tx_execution_idle_time_total;
	while (1) {
		/* 需要周期性处理的程序，对应裸机工程调用的SysTick_ISR */
//		bsp_ProPer1ms();

		/* CPU利用率统计 */
		uiCount++;
		if (uiCount == 200) {
			uiCount = 0;
			deltaIdleTime = _tx_execution_idle_time_total - IdleTime;
			deltaTolTime = _tx_execution_thread_time_total
					+ _tx_execution_isr_time_total
					+ _tx_execution_idle_time_total - TolTime;
			OSCPUUsage = (double) deltaIdleTime / deltaTolTime;
			OSCPUUsage = 100 - OSCPUUsage * 100;
			IdleTime = _tx_execution_idle_time_total;
			TolTime = _tx_execution_thread_time_total
					+ _tx_execution_isr_time_total
					+ _tx_execution_idle_time_total;
		}

		tx_thread_sleep(1);
	}
  /* USER CODE END tx_app_thread_entry */
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */
  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */
  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 2 */

/**
 * @brief 创建任务通讯相关的内核对象，需要在 AppTaskStart 任务中调用
 * @details 初始化 printf 打印所需的互斥锁，保证多线程打印不混乱
 * @return 无
 */
void AppObjCreate(void) {
	/* Create Mutex */

	/* Create Queue */
}

/**
 * @brief 创建应用任务
 *
 */
static void AppTaskCreate(void) {
	/* 创建DemoFileX任务 */
	tx_thread_create(&AppTaskDemoFileXTCB, /* 任务控制块地址 */
	"App Demo FileX", /* 任务名 */
	AppTaskDemoFileX, /* 启动任务函数地址 */
	0, /* 传递给任务的参数 */
	&AppTaskDemoFileXStack[0], /* 堆栈基地址 */
	APP_CFG_TASK_DEMO_FILEX_STACK_SIZE, /* 堆栈空间大小 */
	APP_CFG_TASK_DEMO_FILEX_PRIO, /* 任务优先级*/
	APP_CFG_TASK_DEMO_FILEX_PRIO, /* 任务抢占阀值 */
	TX_NO_TIME_SLICE, /* 不开启时间片 */
	TX_AUTO_START); /* 创建后立即启动 */

}

/**
 * @brief 打印ThreadX任务信息
 *
 */
void DispTaskInfo(void) {
	TX_THREAD *p_tcb; /* 定义一个任务控制块指针 */

	p_tcb = &tx_app_thread;

	/* 打印标题 */
	printf(
			"===================================================================\r\n");
	printf("CPU利用率 = %5.2f%%\r\n", OSCPUUsage);
	printf("任务执行时间 = %.9fs\r\n",
			(double) _tx_execution_thread_time_total / SystemCoreClock);
	printf("空闲执行时间 = %.9fs\r\n",
			(double) _tx_execution_idle_time_total / SystemCoreClock);
	printf("中断执行时间 = %.9fs\r\n",
			(double) _tx_execution_isr_time_total / SystemCoreClock);
	printf("系统总执行时间 = %.9fs\r\n",
			(double) (_tx_execution_thread_time_total
					+ _tx_execution_idle_time_total
					+ _tx_execution_isr_time_total) / SystemCoreClock);
	printf(
			"===================================================================\r\n");
	printf(" 任务优先级 任务栈大小 当前使用栈  最大栈使用   运行次数    任务名\r\n");
	printf("   Prio     StackSize   CurStack    MaxStack    RunCount   Taskname\r\n");

	/* 遍历任务控制列表TCB list)，打印所有的任务的优先级和名称 */
	while (p_tcb != (TX_THREAD*) 0) {

		printf("   %2d        %5ld      %5d       %5d     %10ld   %s\r\n",
				p_tcb->tx_thread_priority,
				p_tcb->tx_thread_stack_size,
				(int) p_tcb->tx_thread_stack_end
						- (int) p_tcb->tx_thread_stack_ptr,
				(int) p_tcb->tx_thread_stack_end
						- (int) p_tcb->tx_thread_stack_highest_ptr,
				p_tcb->tx_thread_run_count,
				p_tcb->tx_thread_name);

		p_tcb = p_tcb->tx_thread_created_next;

		if (p_tcb == &tx_app_thread)
			break;
	}
}

extern void DemoFileX(void);
static void AppTaskDemoFileX(ULONG thread_input) {
	(void) thread_input;

	while (1) {
		DemoFileX();
	}
}

/* USER CODE END 2 */
