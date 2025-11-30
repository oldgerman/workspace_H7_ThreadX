## H723ZGTx144_ThreadX_USBX_MSC_CDC_01

在 H723ZGTx144_ThreadX_USBX_MSC_02 的基础上修，加入 CDC 类，实现 USBX 复合设备，并重定向标准输入输出流到 USB CDC

## CubeMX：USBX配置

### 端点号

参考：[ThreadX+USBX HID CDC复合设备失败](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=106191)

> 我的找到原因了。。垃圾CubeMX生成的程序有问题。
>
> USBD_CDCACM_EPINCMD_ADDR生成的是0x81，与USBD_CDCACM_EPIN_ADDR一样了
> 跟提供的教程比较了一下，发现这个值应该是0x82，改完就好了

在CubeMX配置MSC的IN和OUT端点地址都是1，实际上生成的代码对应的宏： 

```c
/* Device Storage Class */
#define USBD_MSC_EPOUT_ADDR                           0x01U
#define USBD_MSC_EPIN_ADDR                            0x81U
```

CubeMX 界面中配置的 “端点 1” 是「端点号」，生成代码时会自动补充「方向位」，最终`0x01U`（OUT）和`0x81U`（IN）是**同一个端点号的双向批量端点**，符合 USB 协议，不存在冲突。

由于已经将MSC的端点号配置为1，所以需要修改新增复合设备的 CMD IN 端点、BULK IN 和 BULK OUT默认端点号1，否则会冲突

### 免驱端点号

参考：[RL-USB V6.X复合设备的端点号设置真是考究](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=95861&highlight=%B6%CB%B5%E3)

> 使用免驱的HID和MSC，有些BULK IN，OUT以及中断**端点**的配置就可以用，而有些就不行。
>
> 真是邪门，看来这个东西的设计还比较讲究，后面再研究下

参考：[usb复合设备可以复用同一个控制端点吗？](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=123399&highlight=%B6%CB%B5%E3)

### 端点FIFO大小设置

参考：[STM32H7 USB的4KB专用FIFO的分配问题](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=103438&highlight=USB%2BFIFO)

> RL-USB里面有一个：
>
> ```c
> #ifndef USBD_FS_MAX_ENDPOINT_NUM
> #define USBD_FS_MAX_ENDPOINT_NUM   (8U)
> #endif
> #if    (USBD_FS_MAX_ENDPOINT_NUM > 8U)
> #error  Too many Endpoints, maximum IN/OUT Endpoint pairs that this driver supports is 8 !!!
> #endif
> 
> // FIFO sizes in bytes (total available memory for FIFOs is 4 kB)
> #ifndef OTG_FS_RX_FIFO_SIZE
> #define OTG_FS_RX_FIFO_SIZE        (1024U)
> #endif
> #ifndef OTG_FS_TX0_FIFO_SIZE
> #define OTG_FS_TX0_FIFO_SIZE       (64U)
> #endif
> #ifndef OTG_FS_TX1_FIFO_SIZE
> #define OTG_FS_TX1_FIFO_SIZE       (1024U)
> #endif
> #ifndef OTG_FS_TX2_FIFO_SIZE
> #define OTG_FS_TX2_FIFO_SIZE       (512U)
> #endif
> #ifndef OTG_FS_TX3_FIFO_SIZE
> #define OTG_FS_TX3_FIFO_SIZE       (256U)
> #endif
> #ifndef OTG_FS_TX4_FIFO_SIZE
> #define OTG_FS_TX4_FIFO_SIZE       (256U)
> #endif
> #ifndef OTG_FS_TX5_FIFO_SIZE
> #define OTG_FS_TX5_FIFO_SIZE       (256U)
> #endif
> #ifndef OTG_FS_TX6_FIFO_SIZE
> #define OTG_FS_TX6_FIFO_SIZE       (256U)
> #endif
> #ifndef OTG_FS_TX7_FIFO_SIZE
> #define OTG_FS_TX7_FIFO_SIZE       (256U)
> #endif
> #ifndef OTG_FS_TX8_FIFO_SIZE
> #define OTG_FS_TX8_FIFO_SIZE       (192U)
> ```

参考：[ST：在STM32上以设备模式管理USB OTG控制器中FIFO的实际应用案例](https://community.st.com/t5/stm32-mcus/practical-use-cases-to-manage-fifo-in-usb-otg-controllers-in/ta-p/839963)



参考：[ThreadX USB组合设备端点缓存设置问题](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=123050&highlight=%B6%CB%B5%E3)

> 单独读写MSC没问题。用的fx_media_write实现的。MSC读写的时候，CDC不正常。
>
> MSC读写的时候，不对CDC和HID操作，等MSC读写完成后，CDC收发正常，但HID就不行了
>
> [此贴作者还在另一个帖子分享了此BUG工程](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=111611&highlight=HID)
>
> MSC+CDC+HID，CDC和HID可以同时各自做回环测试，同时MSC也可以读，但是MSC只要一写，CDC和HID就会报错，如果写MSC时，不对CDC和HID进行操作，等MSC写完后，再打开CDC做回环测试没问题，HID也能打开，但只能从设备往电脑发送，电脑不能往设备发送。
>
> - [U5A9J_MSC.zip](https://forum.anfulai.cn/forum.php?mod=attachment&aid=OTI0NjJ8NjYwZDY4NTJ8MTc2NDQ0NDA2OHw0NzY3NnwxMTE2MTE%3D)11.13 MB, 下载次数: 29

### HID 复合设备收发通信

> [usbx hid Bus Hound不能发送数据](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=110051&highlight=HID)
>
> 15 个回复 - 4765 次查看
>
> STM32H7，根据en.x-cube-azrtos-h7例程，移植了一个USBX。 复合设备，一个CDC ACM，一个**HID**。 CDC ACM能正常收发。 **HID**能发，但不能收。用BUS HOUND向开发板发送数据，提示invalid command，然后没有任何反应
>
> [求助USBX有人用Custom HID实现了收发通信功能的吗？](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=120887&highlight=HID)
>
> > 他的问题已经解决了。
> >
> > stm32u5开发板 usbx调试
> > [https://forum.anfulai.cn/forum.p ... 1214&fromuid=58](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=121214&fromuid=58)
> >
> > > [**stm32u5开发板 usbx调试** ](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=121214&fromuid=58)
> > >
> > > 已经遇到如下问题：
> > >
> > > \1. 初始化完成后，可正常枚举设备信息，也可正常上报数据，但是第二帧上报的数据如果超过5s，就进入suspend模式，无法唤醒
> > >
> > >   --因为项目还需要用到CDC-ACM，使能CDC-ACM模块后，该问题解决，应该是CDC一直在工作，不会让总线休眠；
> > >
> > > \2. host(PC)通过bus Hound可以向device发送数据，但是只能发送一帧，第二次再发送bus Hound底部显示running，无法complete，一定时间后，device就停止工作了
> > >
> > > 硬汉：
> > >
> > > 1、Host向Device一次发送多少字节。
> > > 2、注意端点FIFO大小配置，这个非常重要。
> > > 2、运行模拟鼠标或键盘的例子是否正常，实在不行，可以用这个例子修改描述为自定义HID类测试。
> > >
> > > > \1. Host向Device双向的通信字节配置都是64字节，端点1
> > > > \2. HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x0200);
> > > >   HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
> > > >   HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x40);
> > > > \3. 鼠标的例子运行过了，配置的就是Custom HID类，然后将report描述符使用鼠标的，可以正常控制鼠标，并且发送间隔不会使总线休眠；目前report的描述符是复用的我们之前项目的描述符，一个input一个output。
> > > > 我也对比过我这个工程，以及单纯的鼠标demo，有差异的地方主要就是描述符的部分。其他的接口几乎一样
> > >
> > > 
> > >
> > > 刚调试中找到部分原因了
> > >
> > > ```c
> > > /**
> > >  \* @brief USBD_Custom_HID_SetReport
> > >  \*      This function is invoked when the host sends a HID SET_REPORT
> > >  \*      to the application over Endpoint OUT (Set Report).
> > >  \* @param hid_instance: Pointer to the hid class instance.
> > >  \* @retval none
> > >  */
> > > VOID USBD_Custom_HID_SetReport(struct UX_SLAVE_CLASS_HID_STRUCT *hid_instance)
> > > {
> > >  /* USER CODE BEGIN USBD_Custom_HID_SetReport */  
> > >   
> > >   ux_device_class_hid_receiver_event_get(hid_instance, &usb_hid_event_rec);
> > >   memcpy(hid_receive_buff, usb_hid_event_rec.ux_device_class_hid_received_event_data,
> > >        usb_hid_event_rec.ux_device_class_hid_received_event_length);
> > >   
> > >   ux_device_class_hid_receiver_event_free(hid_instance);
> > >  /* USER CODE END USBD_Custom_HID_SetReport */
> > > 
> > >  return;
> > > }
> > > ```
> > >
> > > 在这个接收的callback函数中，接收完一次数据，需要将slave的hid事件释放，才能进入到下一次的传输。
> > >
> > > 但是又有新问题出现，传输10次后，就又是这样了，一直卡在running的状态，估计和FIFO相关了。一步一个坎呀
> > >
> > > 确实是fifo配置影响的
> > >
> > > ```c
> > >  HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
> > >  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
> > >  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x40);
> > > ```
> > >
> > > 使用这样的配置，目前测试数据接收和发送，就没有问题了。
> > >
> > > ST Custom HID 示例：https://github.com/STMicroelectronics/stm32-usbx-examples/blob/main/Projects/STM32H743I-EVAL/Applications/USBX/Ux_Device_CustomHID/USBX/App/ux_device_customhid.c

### C# HID通信上位机

> [想用c#写一个usbhid的上位机，有没有可以参考的demo？](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=111944&highlight=HID)
>
> > github上随便搜都有一堆
> > https://github.com/jcoenraadts/hid-sharp
> > https://github.com/mikeobrien/HidLibrary

### 配置值

- UX_STANDALONE：不定义，以运行在 RTOS 模式

  > 是否使能整个USBX框架独立使用模式

- UX_DEVICE_STANDALONE：不定义，以运行在 RTOS 模式

  > 是否使能 USB Device独立使用模式，不影响 USB Host 的模式

- UX_MAX_SLAVE_CLASS_DRIVER：2（等于复合设备数量）

  > 此项配置不正确会导致的问题：
  >
  > 在MX_USBX_Device_Init() 内部调用第二个USB CDC类注册，即
  >
  > ```
  > ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name,
  >           ux_device_class_cdc_acm_entry,
  >           cdc_acm_configuration_number,
  >           cdc_acm_interface_number,
  >           &cdc_acm_parameter);
  > ```
  >
  > 一直返回 UX_MEMORY_INSUFFICIENT，字面意思是内存不足，尝试加大 USBX_DEVICE_MEMORY_STACK_SIZE 到 50KB，UX_DEVICE_APP_MEM_POOL_SIZE 到 100KB 都没能解决，原因是 ux_device_stack_class_register()会检查 UX_MAX_SLAVE_CLASS_DRIVER  宏是否为 1，如果是 1就返回  UX_MEMORY_INSUFFICIENT
  >
  > 本工程需要在MSC类之后注册一个CDC类实现复合设备，而 CubeMX 里 UX_MAX_SLAVE_CLASS_DRIVER  还是默认值 1，需要改为 2 解决

- USBD_CDCACM_ENDPOINT_IN_CMD_ADDR：2（IN CMD 端点号）

- USBD_CDCACM_ENDPOINT_IN_CMD_FS_MPS：8

- USBD_CDCACM_ENDPOINT_IN_CMD_HS_MPS：8

  > [来源：豆包](https://www.doubao.com/thread/w486e4506a16a90ff)
  >
  > MPS（最大包大小）只需**能容纳该端点的最大传输数据量**即可，无需配到上限：
  >
  > - **全速（FS）**：中断端点的 MPS 最大为 64 字节，但命令中断端点的实际数据仅 1~2 字节；
  > - **高速（HS）**：中断端点的 MPS 最大也为 64 字节（高速 USB 的中断端点 MPS 上限仍为 64），实际数据量与全速一致。
  >
  > 推荐配置：
  >
  > - 8 字节（行业标准值）
  >
  > 避坑说明
  >
  > - 无需配到上限（64 字节）：命令中断端点的数据量固定很小，配 64 字节只会浪费 USB 总线的端点带宽（无实际收益）；
  > - FS 和 HS 可统一配置为 8：两者传输的控制信号数据量完全一致，无需区分全速 / 高速单独调整。

- USBD_CDCACM_ENDPOINT_IN_CMD_FS_BINTERVAL：8

- USBD_CDCACM_ENDPOINT_IN_CMD_HS_BINTERVAL：8

  > [来源：豆包](https://www.doubao.com/thread/w5101946f32babe8b)
  >
  > 是 CDC ACM**命令中断 IN 端点**的「轮询间隔（bInterval）」，对应 USB 描述符中的`bInterval`字段，主机按照该间隔轮询设备的 CDC 命令中断端点，获取串口控制信号（如 DTR/RTS 状态、线状态变化等）
  >
  > 取值范围：1~255
  >
  > CDC ACM 命令中断端点的 bInterval 无需追求 “最快”，需平衡「响应速度」和「USB 总线负载」，以下是符合 USB 协议 + 实际工程的配置方案：
  >
  > | 场景                 | FS_BINTERVAL（全速） | HS_BINTERVAL（高速） | 说明                                                         |
  > | -------------------- | -------------------- | -------------------- | ------------------------------------------------------------ |
  > | 通用场景（推荐）     | 8                    | 8                    | FS=8ms 轮询，HS=1ms 轮询（1ms 是 CDC 控制信号的主流响应速度，总线负载低）； |
  > | 快速响应场景         | 1                    | 8                    | FS=1ms 轮询（最快），HS=1ms 轮询（与 FS 响应速度对齐）；     |
  > | 低总线负载场景       | 10~20                | 16                   | FS=10~20ms 轮询，HS=2ms 轮询（16×125μs），适合对控制信号响应无高要求的场景； |
  > | 极端低负载（不推荐） | ≤50                  | ≤16                  | FS 不建议超过 50ms（否则控制信号延迟明显）；HS 超过 16 无意义（USB 2.0 规范中 HS 中断端点 bInterval 实际有效范围仅 1~16，超过后主机仍按 16 微帧处理）； |
  >
  > HS 模式下，即使 CubeMX 允许设 1~255，**实际有效取值仅 1~16**（USB 2.0 协议规定），设为 17~255 时，主机仍会按 16 微帧（2ms）轮询

- USBD_CDCACM_ENDPOINT_IN_ADDR：3（IN BULK 端点号）

- USBD_CDCACM_ENDPOINT_IN_FS_MPS：64

- USBD_CDCACM_ENDPOINT_IN_HS_MPS：512

- USBD_CDCACM_ENDPOINT_OUT_ADDR：3（OUT BULK 端点号）

- USBD_CDCACM_ENDPOINT_OUT_FS_MPS：64

- USBD_CDCACM_ENDPOINT_OUT_HS_MPS：512

- UX_DEVICE_CLASS_CDC_ACM_ZERO_COPY：DISABLED

  > 零拷贝保持默认暂不开启，待研究

- UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE：ENABLED

- UX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP：ENABLED

[分享基于安富莱ThreadX全家桶2.0版本实现的USBX CDC ACM+PPP连接服务](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=123737)

## 参考

### 例程 Ux_Device_CDC_ACM

ST的USBX设备例程：CDC_ACM 转 UART 桥

[x-cube-azrtos-h7-main\x-cube-azrtos-h7-main\Projects\NUCLEO-H723ZG\Applications\USBX\Ux_Device_CDC_ACM](https://github.com/STMicroelectronics/x-cube-azrtos-h7/tree/main/Projects/NUCLEO-H723ZG/Applications/USBX/Ux_Device_CDC_ACM)

此例程**实现 USB CDC ACM 转 USART 桥**，使用了事件标志组 EventFlag 与 H723ZGTx144_ThreadX_USBX_MSC_02 工程 MSC 使用的事件标志组同名，COPY代码时需要区分！

### 例程 Ux_Device_HID_CDC_ACM

ST的USBX复合设备例程：HID 模拟鼠标设备 + CDC_ACM 转 UART 桥

[x-cube-azrtos-h7-main\x-cube-azrtos-h7-main\Projects\STM32H747I-DISCO\Applications\USBX\Ux_Device_HID_CDC_ACM](https://github.com/STMicroelectronics/x-cube-azrtos-h7/tree/main/Projects/STM32H747I-DISCO/Applications/USBX/Ux_Device_HID_CDC_ACM)

> 系统会定期在 usbx_cdc_acm_write_thread_entry 中检查缓冲区“UserTxBufferFS”的状态。如果有可用数据，则会响应输入 (IN) 请求发送数据；否则，该缓冲区将被标记为 NAK

PS：还有一个 H735 的主机HID和CDC_ACM复合例程 Ux_Host_HID_CDC_ACM，以后要是用得上可以看看

## printf 线程安全问题

### 方案一：在 printf 外面封装一个 App_Printf 加互斥锁（不推荐）

例程 V7-2401_ThreadX USBX Template

[安富莱 STM32-V7 开发板 ThreadX USBX 教程,pdf](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=108546)

App_Printf 函数做了信号量的互斥操作，解决资源共享问题

main.c

```c
/**
 * @brief 用于 printf 打印的互斥锁（线程安全打印）
 */
static TX_MUTEX AppPrintfSemp;

/**
 * @brief 创建任务通讯相关的内核对象
 * @details 初始化 printf 打印所需的互斥锁，保证多线程打印不混乱
 * @return 无
 */
static void AppObjCreate(void)
{
    /* 创建 printf 互斥锁：不启用优先级继承 */
    tx_mutex_create(&AppPrintfSemp, "AppPrintfSemp", TX_NO_INHERIT);
}

/**
 * @brief 线程安全的 printf 封装函数
 * @details 基于 ThreadX 互斥锁实现多线程打印互斥，避免打印内容交织乱码；
 *          内部使用固定缓冲区，需注意打印内容长度不超过缓冲区上限
 * @param fmt 格式化输出字符串（同 printf 格式）
 * @param ... 可变参数列表（同 printf 可变参数）
 * @return 无
 */
static void App_Printf(const char *fmt, ...)
{
    /* 打印缓冲区：预留 1 字节用于字符串结束符，避免越界 */
    char buf_str[200 + 1];
    va_list v_args;

    /* 解析可变参数并格式化到缓冲区 */
    va_start(v_args, fmt);
    (void)vsnprintf(buf_str, sizeof(buf_str), fmt, v_args);
    va_end(v_args);

    /* 互斥锁保护：独占 printf 打印，防止多线程抢占 */
    tx_mutex_get(&AppPrintfSemp, TX_WAIT_FOREVER);
    printf("%s", buf_str);
    tx_mutex_put(&AppPrintfSemp);
}
```

增加缓冲区溢出检查的版本

```c

static void App_Printf(const char *fmt, ...)
{
    char buf_str[200 + 1];
    va_list v_args;
    int ret;

    va_start(v_args, fmt);
    ret = vsnprintf((char       *)&buf_str[0],
                    (size_t      ) sizeof(buf_str),
                    (char const *) fmt,
                                   v_args);
    va_end(v_args);

    /* 增加缓冲区溢出提示 */
    if (ret >= (int)sizeof(buf_str)) {
        /* 互斥printf */
        tx_mutex_get(&AppPrintfSemp, TX_WAIT_FOREVER);
        printf("[PRINTF WARN] Buffer overflow! Content truncated.\n");
        tx_mutex_put(&AppPrintfSemp);
    }
    /* 互斥 printf */
    tx_mutex_get(&AppPrintfSemp, TX_WAIT_FOREVER);
    printf("%s", buf_str);
    tx_mutex_put(&AppPrintfSemp);
}
```



### 方案二：在 _write / _read 中加互斥锁（最佳实践）

> PS：不推荐在更底层的 _gets / _puts 加互斥锁，效率太低

帖子：printf、fprintf、scanf 等函数在线程 X 中是否线程安全？

[2025年2月11日：printf、fprintf、scanf 等函数在线程 X 中是否线程安全？](https://community.st.com/t5/stm32-mcus-embedded-software/are-printf-fprintf-scanf-etc-threadsafe-in-threadx/td-p/771846)

> > [ABasi.2](https://community.st.com/t5/user/viewprofilepage/user-id/52099)
> >
> > 你好
> >
> > 我有一个关于 printf 函数的问题。还有 fprintf、scanf 等函数。
> >
> > 我正在将这些函数重定向到使用 UART，替换int _write(int file, char *ptr, int len) 和 int _read(int file, char *ptr, int len) 函数。
> >
> > ```c
> > int _write(int file, char *ptr, int len)
> > { 
> >   HAL_UART_Transmit(&huart2,(uint8_t*)ptr, len, 1000); 
> >   return len;
> > }
> > 
> > int _read(int file, char *ptr, int len)
> > {    
> >   __HAL_UART_CLEAR_OREFLAG(&huart2);
> >   HAL_UART_Receive(&huart2,(uint8_t*)ptr, len,HAL_MAX_DELAY);
> >   HAL_UART_Transmit(&huart2,(uint8_t*)ptr,len, 1000);  
> >   return len;
> > }
> > ```
> >
> > 现在我正在使用 threadX，并且可能会从多个线程调用 printf 或其他函数。
> >
> > 这种方法本身是否线程安全，还是我需要为UART实现互斥锁？
> >
> > 如果我必须使用互斥锁，是否可以在 _write 或 _read 函数内部获取并放置互斥锁，还是必须在调用 printf 函数之前获取互斥锁？
> >
> > 谢谢
>
> >  [Saket_Om](https://community.st.com/t5/user/viewprofilepage/user-id/1708) ST员工
> >
> > **已接受的解决方案**
> >
> > 为了确保线程安全，您需要使用互斥锁来保护 UART 访问。您可以在 `_write` 和 `_read` 函数内部实现互斥锁，也可以在调用 `printf` 或其他相关函数之前获取互斥锁。
>
> > [ABasi.2](https://community.st.com/t5/user/viewprofilepage/user-id/52099)
> >
> > 谢谢你！
> >
> > 我已将代码修改如下：
> >
> > ```c
> > int _write(int file, char *ptr, int len)
> > {     
> >   if(tx_mutex_get(&consoleMutex,TX_WAIT_FOREVER) == TX_SUCCESS)
> >   {
> >     HAL_UART_Transmit(&huart2,(uint8_t*)ptr, len, 1000);
> >     tx_mutex_put(&consoleMutex);
> >     return len;
> >   }
> >   return 0;  
> > }
> > ```
> >
> > 运行正常
> >
> > 致以最诚挚的问候

[豆包：在\_write/\_read中加互斥锁](https://www.doubao.com/thread/w6c1426625fc9471d)

>### 核心结论：**完全可以（且是更优方案）**
>
>在 `_write`/`_read` 中加互斥锁是 ThreadX 下实现 UART 底层线程安全的**最佳实践**——相比在上层 `App_Printf` 加锁，这种方式能让所有依赖 `_write`/`_read` 的标准库函数（`printf`/`fprintf`/`puts`/`scanf`/`gets` 等）都自动具备线程安全，无需逐个封装上层函数。
>
>
>### 一、为什么推荐在 `_write`/`_read` 中加锁？
>
>| 加锁方式                 | 优点                                                         | 缺点                                                         |
>| ------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
>| 底层（_write/_read）加锁 | 1. 覆盖所有标准IO函数（printf/scanf等）；<br>2. 代码简洁，无需封装上层函数；<br>3. 互斥逻辑集中，易维护 | 持有锁期间若UART操作阻塞，其他线程等待时间略长（可优化）     |
>| 上层（App_Printf）加锁   | 持有锁时间短（仅格式化字符串后打印）                         | 1. 仅覆盖封装后的函数，原生printf/scanf仍不安全；<br>2. 需逐个封装所有IO函数，代码冗余 |
>
>
>### 二、完整实现代码（ThreadX + STM32 HAL）
>
>#### 1. 头文件 & 全局互斥锁定义
>
>```c
>#include "tx_api.h"
>#include "stm32h7xx_hal.h"
>
>/* 声明UART句柄（需和你的工程一致，如huart1/huart2） */
>extern UART_HandleTypeDef huart2;
>
>/* 定义UART互斥锁（全局可见，供_write/_read使用） */
>static TX_MUTEX uart_mutex;
>```
>
>#### 2. 初始化互斥锁（必须在使用前创建！）
>
>在 `tx_application_define`（ThreadX 任务创建入口）中初始化互斥锁：
>
>```c
>void tx_application_define(void *first_unused_memory)
>{
>    UINT status;
>
>    /* 创建UART互斥锁：不启用优先级继承（简单场景足够） */
>    status = tx_mutex_create(&uart_mutex, "UART_Mutex", TX_NO_INHERIT);
>    if (status != TX_SUCCESS)
>    {
>        /* 初始化失败处理（如LED报错） */
>        while(1);
>    }
>
>    /* 后续：创建业务线程... */
>}
>```
>
>#### 3. 改造 `_write` 函数（输出printf/puts等）
>
>```c
>/* 重定向标准输出：printf/fprintf等最终调用此函数 */
>int _write(int file, char *ptr, int len)
>{
>    /* 仅处理标准输出/标准错误（避免其他文件描述符干扰） */
>    if (file != 1 && file != 2) 
>    {
>        errno = EBADF;
>        return -1;
>    }
>
>    /* 获取互斥锁（无限等待，确保必能获取） */
>    if (tx_mutex_get(&uart_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
>    {
>        /* 阻塞发送UART数据（超时1000ms，适配HAL库） */
>        HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
>        
>        /* 释放互斥锁（必须！否则其他线程永久阻塞） */
>        tx_mutex_put(&uart_mutex);
>        return len;
>    }
>    
>    /* 锁获取失败（理论上不会走到，因TX_WAIT_FOREVER） */
>    errno = EIO;
>    return 0;
>}
>```
>
>#### 4. 改造 `_read` 函数（输入scanf/gets等）
>
>```c
>/* 重定向标准输入：scanf/gets等最终调用此函数 */
>int _read(int file, char *ptr, int len)
>{
>    /* 仅处理标准输入 */
>    if (file != 0) 
>    {
>        errno = EBADF;
>        return -1;
>    }
>
>    /* 获取互斥锁 */
>    if (tx_mutex_get(&uart_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
>    {
>        /* 清除UART溢出标志（避免接收异常） */
>        __HAL_UART_CLEAR_OREFLAG(&huart2);
>        
>        /* 阻塞接收数据（HAL_MAX_DELAY表示无限等待） */
>        HAL_UART_Receive(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
>        
>        /* 回显（可选：输入的字符再发回串口，方便调试） */
>        HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
>        
>        /* 释放互斥锁 */
>        tx_mutex_put(&uart_mutex);
>        return len;
>    }
>    
>    errno = EIO;
>    return 0;
>}
>```
>
>
>### 三、关键避坑注意事项
>
>#### 1. 死锁防范（重中之重）
>
>- **持有锁时避免耗时操作**：`HAL_UART_Transmit/Receive` 的超时不要设过大（如 `HAL_MAX_DELAY` 仅在 `_read` 中用），否则持有锁时间过长，其他线程阻塞严重；
>
>- **禁止中断中调用标准IO函数**：中断上下文不能调用 `tx_mutex_get`（ThreadX 互斥锁仅支持线程级调用），若中断需打印，改用“环形缓冲区 + 线程处理”：
>
>  ```c
>  // 示例：中断中写入缓冲区，线程中读取并打印
>  uint8_t uart_buf[128];
>  uint16_t buf_idx = 0;
>  void USART2_IRQHandler(void)
>  {
>      if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET)
>      {
>          uart_buf[buf_idx++] = huart2.Instance->DR;
>          buf_idx %= 128; // 防止溢出
>          __HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_RXNE);
>      }
>  }
>  ```
>
>#### 2. 互斥锁初始化时机
>
>必须在**第一个线程调用 `printf`/`scanf` 前**创建互斥锁（推荐在 `tx_application_define` 中创建），否则 `tx_mutex_get` 会返回 `TX_ERROR`，导致UART操作无保护。
>
>#### 3. 优化 `_read` 的阻塞问题
>
>`HAL_UART_Receive(&huart2, ..., HAL_MAX_DELAY)` 会让线程永久阻塞，且持有互斥锁期间其他线程无法使用UART。建议改为**中断/DMA接收**：
>
>```c
>// 示例：DMA接收（非阻塞）
>uint8_t rx_dma_buf[64];
>int _read(int file, char *ptr, int len)
>{
>    if (file != 0) { errno = EBADF; return -1; }
>
>    if (tx_mutex_get(&uart_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
>    {
>        // 启动DMA接收（非阻塞）
>        HAL_UART_Receive_DMA(&huart2, (uint8_t*)rx_dma_buf, len);
>        // 等待DMA接收完成（可通过信号量/标志位）
>        while (HAL_DMA_GetState(&hdma_usart2_rx) != HAL_DMA_STATE_READY);
>        
>        memcpy(ptr, rx_dma_buf, len);
>        HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
>        
>        tx_mutex_put(&uart_mutex);
>        return len;
>    }
>    return 0;
>}
>```
>
>#### 4. 错误处理增强（可选）
>
>将 `TX_WAIT_FOREVER` 改为超时等待，避免极端情况死锁：
>
>```c
>// _write中示例：1秒超时
>if (tx_mutex_get(&uart_mutex, TX_TIMER_TICKS_PER_SECOND * 1) == TX_SUCCESS)
>{
>    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
>    tx_mutex_put(&uart_mutex);
>    return len;
>}
>else
>{
>    /* 锁超时：记录日志或报错 */
>    return -1;
>}
>```
>
>
>### 四、最终效果
>
>改造后，所有线程直接调用原生 `printf`/`scanf` 即可保证线程安全，无需封装 `App_Printf`：
>
>```c
>// 线程1
>void thread1_entry(ULONG thread_input)
>{
>    while(1)
>    {
>        printf("线程1：%d\n", tx_time_get()); // 自动线程安全
>        tx_thread_sleep(100);
>    }
>}
>
>// 线程2
>void thread2_entry(ULONG thread_input)
>{
>    char buf[32];
>    while(1)
>    {
>        scanf("%s", buf); // 自动线程安全
>        printf("线程2收到：%s\n", buf);
>        tx_thread_sleep(200);
>    }
>}
>```
>
>这种方式既简洁又能覆盖所有标准IO函数，是 ThreadX 下 UART 重定向的工业级实现方案。
>
>

###  实际操作

（采用方案二）

之前问豆包的是基于UART的API 在 `_write`/`_read` 中加互斥锁，我这里需要使用 USB CDC 的API 

注意到在 ux_device_class_cdc_acm.h 中有以下CDC Class API 可供调用

```c
......
/* Define CDC Class structure.  */
// 其中定义了 cdc acm 读写完成后的回调函数：
// ux_device_class_cdc_acm_write_callback 和 ux_device_class_cdc_acm_read_callback
typedef struct UX_SLAVE_CLASS_CDC_ACM_STRUCT
{
......
#ifndef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
#if !defined(UX_DEVICE_STANDALONE)
    UX_THREAD                           ux_slave_class_cdc_acm_bulkin_thread;
    UX_THREAD                           ux_slave_class_cdc_acm_bulkout_thread;
    UX_EVENT_FLAGS_GROUP                ux_slave_class_cdc_acm_event_flags_group;
    UCHAR                               *ux_slave_class_cdc_acm_bulkin_thread_stack;
    UCHAR                               *ux_slave_class_cdc_acm_bulkout_thread_stack;
#endif
    UINT                                (*ux_device_class_cdc_acm_write_callback)(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, ULONG length);
    UINT                                (*ux_device_class_cdc_acm_read_callback)(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length);
    ULONG                               ux_slave_class_cdc_acm_transmission_status;
    ULONG                               ux_slave_class_cdc_acm_scheduled_write;
#if !defined(UX_DEVICE_STANDALONE)
    ULONG                               ux_slave_class_cdc_acm_callback_total_length;
    UCHAR                               *ux_slave_class_cdc_acm_callback_data_pointer;
    UCHAR                               *ux_slave_class_cdc_acm_callback_current_data_pointer;
#endif
#endif
} UX_SLAVE_CLASS_CDC_ACM;

......

/* Define Device CDC Class API prototypes.  */
#define ux_device_class_cdc_acm_entry               _ux_device_class_cdc_acm_entry

#if defined(UX_DEVICE_CLASS_CDC_ACM_ENABLE_ERROR_CHECKING) //!< 注：CubeMX默认不使能
// 使能错误检查可调用的 API
#define ux_device_class_cdc_acm_read                _uxe_device_class_cdc_acm_read
#define ux_device_class_cdc_acm_write               _uxe_device_class_cdc_acm_write
#define ux_device_class_cdc_acm_ioctl               _uxe_device_class_cdc_acm_ioctl
#define ux_device_class_cdc_acm_write_with_callback _uxe_device_class_cdc_acm_write_with_callback

#define ux_device_class_cdc_acm_read_run            _uxe_device_class_cdc_acm_read_run
#define ux_device_class_cdc_acm_write_run           _uxe_device_class_cdc_acm_write_run

#else
// 不使能错误检查可调用的 API
#define ux_device_class_cdc_acm_read                _ux_device_class_cdc_acm_read
#define ux_device_class_cdc_acm_write               _ux_device_class_cdc_acm_write
#define ux_device_class_cdc_acm_ioctl               _ux_device_class_cdc_acm_ioctl
#define ux_device_class_cdc_acm_write_with_callback _ux_device_class_cdc_acm_write_with_callback

#define ux_device_class_cdc_acm_read_run            _ux_device_class_cdc_acm_read_run
#define ux_device_class_cdc_acm_write_run           _ux_device_class_cdc_acm_write_run

#endif
    
......
```

#### MSC+CDC类有多少个任务被创建？（修改前）

在应用程序启动时，ThreadX 调用入口函数 tx_application_define()，在此阶段，所有 USBx 资源都将被初始化，MSC 类和 CDC_ACM 类的驱动程序将被注册，并且应用程序创建 3 个优先级不尽相同的线程：

- app_ux_device_thread_entry（优先级：10；抢占优先级：10）用于初始化 USB OTG HAL PCD 驱动程序并启动设备。
- usbx_cdc_acm_read_thread_entry（优先级：20；抢占优先级：20）用于从虚拟 COM 端口读取接收到的数据。
- usbx_cdc_acm_write_thread_entry（优先级：20；抢占优先级：20）用于通过 UART 发送接收到的数据。

此外，USBX MSC 设备还需要两个回调函数：

- USBD_STORAGE_Read 用于通过 DMA 从大容量存储设备读取数据。
- USBD_STORAGE_Write 用于通过 DMA 将数据写入大容量存储设备。

#### 涉及 Device CDC Class API 的相关文章

> 参考：[深入解析Azure RTOS USBX：从虚拟串口实现到FreeRTOS集成](http://new.guyuehome.com/wap/detail?id=1881922124472438786)
>
> > 使用单独模 式 （STANDALONE ） 时 ， 也需要创建一个任务 ， 不断 运行 “_ux_system_tasks_run ”函数
>
> 单独模式下的调用顺序：
>
> ```c
> ux_device_cdc_acm_send 启动传输 -> //!< 自行实现的函数，不在USBX库中
> ux_device_class_cdc_acm_write_with_callback 调用此回调函数 -> 
> ux_device_class_cdc_acm_write_callback 发送完成回调函数会被调用并释放信号量 -> 
> pdTRUE == xSemaphoreTake(g_xUSBUARTSend, timeout) 得到信号量被唤醒 -> 
> ux_device_class_cdc_acm_read_callback 此回调函数会被调用 当读取到数据时将数据写入队列中 -> 
> ux_device_cdc_acm_getchar 此函数获取队列中的内容于是主函数调用并打印。
> 
> USB 串口收到数据后， ux_device_class_cdc_acm_read_callback 函数被调用
> ```
>
> > 分析了 ux_device_class_cdc_acm_read_callback 和 ux_device_class_cdc_acm_write_callback 结合二值信号量和队列的使用方法
> >
> > 宏 UX_STANDALONE 是否定义决定使用单独模式或 RTOS 模式：
> >
> > > ```c
> > > /* Defined, this macro will enable the standalone mode of usbx.  */
> > > #define UX_STANDALONE
> > > ```
> >
> > 阻塞式：
> >
> > > 当没有定义UX_STANDALONE 时，就是使用 RTOS 模式
> > >
> > > RTOS 模式下的 USBX，可以使用 ThreadX 提供的互斥量函数实现阻塞式读写（“blocking”）
> > >
> > > 比如对于 USB 虚拟串口， 可以使用如下阻塞函数：
> > >
> > > ```c
> > > UINT _ux_device_class_cdc_acm_read(UX_SLAVE_CLASS_CDC_ACM *cdc_acm, UCHAR *buffer,
> > > ULONG requested_length, ULONG *actual_length);
> > > 
> > > UINT _ux_device_class_cdc_acm_write(UX_SLAVE_CLASS_CDC_ACM *cdc_acm, UCHAR *buffer, ULONG requested_length, ULONG *actual_length);
> > > ```
> > >
> > > 这 2 个函数发起的数据传输，在传输过程中线程阻塞，传输完成后线程被唤醒
> >
> > 非阻塞式：
> >
> > > 当定义了 UX_STANDALONE 时，就是使用单独模式
> > >
> > >  不能再使用上面的阻塞函数，而要使用非阻塞的函数（non-blocke）：
> > >
> > > ```c
> > > UINT _ux_device_class_cdc_acm_read_run(UX_SLAVE_CLASS_CDC_ACM *cdc_acm,
> > > UCHAR *buffer, ULONG requested_length, ULONG *actual_length);
> > > 
> > > UINT _ux_device_class_cdc_acm_write_run(UX_SLAVE_CLASS_CDC_ACM *cdc_acm,
> > > UCHAR *buffer, ULONG requested_length, ULONG *actual_length);
> > > ```
> > >
> > > 它们只是发起传输，然后就即刻返回。需要提供回调函数，在回调函数里分辨数据是 否传输完成。
> > >
> > > ```c
> > > /* Defined, this macro disables CDC ACM non-blocking transmission support. */ //#define UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
> > > ```
> > >
> > > 定义 UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE，就禁止了“非阻塞模式”， 这时只能使用基于 RTOS 的阻塞函数。
> > >
> > > 在单独模式下需要非阻塞函数， 不能定义 UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE 这个配置项
>
> 参考：[韦东山：4.7 移植 USBX 实现虚拟串口](https://zhuanlan.zhihu.com/p/706030891)
>
> > 注意，USBX是独立使用模式，定义了 UX_STANDALONE
> >
> > [project/App/DshanMCU_H7R_LVGL_Desktop/Appli/Middlewares_100ask/usbx/app/ux_user.h](https://github.com/100askTeam/DshanMCU_H7R/blob/a7baf7c150c002f6b5f96001fa30c3d10e08fb5a/project/App/DshanMCU_H7R_LVGL_Desktop/Appli/Middlewares_100ask/usbx/app/ux_user.h#L473)
> >
> > ```c
> > /* Defined, this macro will enable the standalone mode of usbx.  */
> > #define UX_STANDALONE
> > ```
> >
> > **4.8.2 数据收发函数**
> >
> > 涉及文件为：demo\Middlewares\Third_Party\usbx\app\ux_device_cdc_acm.c
> >
> >  开发板通过 USB 串口发出数据时， 使用以下函数：
> >
> > ```c
> > /* 启动发送：... _write_with_callback  */
> > UINT ux_device_class_cdc_acm_write_with_callback(UX_SLAVE_CLASS_CDC_ACM *cdc_acm, UCHAR *buffer, ULONG requested_length);
> > 
> > /* 发送完毕的回调函数 ... _write_callback */
> > static UINT ux_device_class_cdc_acm_write_callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, ULONG length);
> > ```
> >
> > 需要实现 ux_device_cdc_acm_send 函数，它调用了 ux_device_class_cdc_acm**_write_with_callback** 启动发送，然后等待 ux_device_class_cdc_acm**_write_callback** 唤醒：
> >
> > ```c
> > int ux_device_cdc_acm_send(uint8_t *datas, uint32_t len, uint32_t timeout);
> > ```
> >
> > 开发板接收到 USB 串口数据时，以下回调函数被调用：
> >
> > ```c
> > static UINT ux_device_class_cdc_acm_read_callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length);
> > ```
> >
> > 可以改造这个函数， 把接收到的数据写入队列
> >
> > > 个人注：
> > >
> > > 只有发送有 `ux_device_class_cdc_acm_write_with_callbackh()` ，接收是没有 `ux_device_class_cdc_acm_read_with_callback()` 的，想想为什么
> >
> > ### **4.8.3 使用 FreeRTOS 改造代码**
> >
> > 对于发送， 实现以下函数：启动发送之后阻塞，等待回调函数唤醒或超时。
> >
> > ```c
> > static UINT ux_device_class_cdc_acm_write_callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length);
> > ```
> >
> > 对于接收， 实现以下函数：把接收到的数据写入队列。
> >
> > ```c
> > static UINT ux_device_class_cdc_acm_read_callback(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length);
> > ```
> >
> > 然后提供这个函数：
> >
> > ```c
> > int ux_device_cdc_acm_getchar(uint8_t *pData, uint32_t timeout);
> > ```
>
> 参考：[USB 设备类型之 CDC (Communication Device Class)](https://100ask.net/article/919)
>
> > 源码出处（开源裸机工程）：[github.com\100askTeam\DshanMCU_H7R](https://github.com/100askTeam/DshanMCU_H7R/blob/a7baf7c150c002f6b5f96001fa30c3d10e08fb5a/project/App/DshanMCU_H7R_LVGL_Desktop/Appli/Middlewares_100ask/usbx/app/ux_device_cdc_acm.c#L161)
> >
> > 视频Demo：[0-4_LVGL入门教程之课程Demo演示](https://www.bilibili.com/video/BV1Tw4m1k7C3/?vd_source=e6ad3ca74f59d33bf575de5aa7ceb52e)
> >
> > 注意，USBX是独立使用模式，定义了 UX_STANDALONE
> >
> > [project/App/DshanMCU_H7R_LVGL_Desktop/Appli/Middlewares_100ask/usbx/app/ux_user.h](https://github.com/100askTeam/DshanMCU_H7R/blob/a7baf7c150c002f6b5f96001fa30c3d10e08fb5a/project/App/DshanMCU_H7R_LVGL_Desktop/Appli/Middlewares_100ask/usbx/app/ux_user.h#L473)
> >
> > ```c
> > /* Defined, this macro will enable the standalone mode of usbx.  */
> > #define UX_STANDALONE
> > ```
> >
> > ## ux_device_cdc_acm.c 源码分析
> >
> > ### `USBD_CDC_ACM_Activate`
> >
> > 在设备插入时被调用，用于激活 CDC ACM 类设备。
> >
> > ```c
> > VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance)
> > ```
> >
> > 参数 `cdc_acm_instance` 指向 CDC ACM 类实例。
> >
> > ```c
> > UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER parameter;
> > ```
> >
> > 这里定义了一个 `UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER` 类型的变量 `parameter`。该变量将用来存储 CDC ACM 类设备相关的回调函数。
> >
> > ```c
> > cdc_acm = (UX_SLAVE_CLASS_CDC_ACM*) cdc_acm_instance;
> > ```
> >
> > 通过传入的 `cdc_acm_instance` 指针将其转换成 `UX_SLAVE_CLASS_CDC_ACM` 类型，并保存到全局变量 `cdc_acm` 中。这表示当前设备的 CDC ACM 类实例已经被成功保存，可以在后续的代码中进行操作。
> >
> > ```c
> > parameter.ux_device_class_cdc_acm_parameter_write_callback = ux_device_class_cdc_acm_write_callback;
> > parameter.ux_device_class_cdc_acm_parameter_read_callback = ux_device_class_cdc_acm_read_callback;
> > ```
> >
> > - 在`parameter`结构体中设置了写和读的回调函数。具体来说：
> >   - `ux_device_class_cdc_acm_write_callback`：写操作的回调函数。
> >   - `ux_device_class_cdc_acm_read_callback`：读操作的回调函数。
> >
> > ```c
> > ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, (VOID *)&parameter);
> > ```
> >
> > 调用 `ux_device_class_cdc_acm_ioctl` 函数，传递 `cdc_acm` 类实例和一个命令 `UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START`，并将 `parameter` 作为参数。这个函数的作用是启动传输，并传递相关的回调函数，用于后续的数据读写。
> >
> > - **`USBD_CDC_ACM_Activate()`函数在 CDC ACM 设备插入时调用，主要负责：**
> >   1. **激活 CDC ACM 类设备。**
> >   2. **保存设备实例。**
> >   3. **配置写和读的回调函数。**
> >   4. **启动数据传输。**
> >   5. **创建 FreeRTOS 信号量和队列用于数据传输的同步与接收。**
> > - **`USBD_CDC_ACM_ParameterChange()` 函数用于处理来自主机的 CDC ACM 类控制请求。主要操作包括：**
> >   - **根据请求类型，处理设置和获取线路编码（`SET_LINE_CODING` 和 `GET_LINE_CODING`）。**
> >   - **在设置线路编码时，检查波特率是否符合最低要求，并更新配置。**
> >   - **当主机请求设置或获取线路编码时，通过 `ux_device_class_cdc_acm_ioctl` 函数读取或设置相应的参数。**
> >   - **该函数为虚拟串口功能提供了对主机请求的响应机制，并实现了 CDC ACM 协议的一部分。**

以上文章中 USBX 都是独立模式下运行，但 ThreadX + USBX 默认配置 UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE 是 ENABLE，相应的在代码中，C预处理器 ux_device_class_cdc_acm_write_callback()、ux_device_class_cdc_acm_read_callback() 这两个函数指针检测到没有定义UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE就被注释掉了，ThreadX会对USBX CDC ACM 设备类创建两个任务。直接是 _ux_device_class_cdc_acm_write_with_callback() 调用发送后发送事件标志组，任务\_ux_device_class_cdc_acm_bulkin_thread() 获取事件标志组

#### 在RTOS下 也要使用 USBX 非阻塞收发 API

所以 USBX 在 RTOS 模式下就不需要修改CubeMX配置：`UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE` 从`ENABLE`改为 `DISABLE`不可取？

不对，有这篇文章：

[ST论坛：STM32U5A5 USB CDC 问题](https://community.st.com/t5/stm32-mcus-products/stm32u5a5-usb-cdc-issue/td-p/641916)

> 此帖子参考了这个帖子：[NUCLEO-U5A5ZJ-Q USB CDC ACM 与 ux_device_class_cdc_acm_write() 函数存在问题](https://community.st.com/t5/stm32-mcus-products/nucleo-u5a5zj-q-usb-cdc-acm-issue-with-ux-device-class-cdc-acm/td-p/632086)
>
> > 这个帖子解决的问题：**数据包超过128字节一直等待中断**
> >
> > 开发板：NUCLEO-U5A5ZJ-Q
> >
> > 示例：Ux_Device_CDC_ACM
> >
> > 我正在尝试测试通过 VCP 从设备向主机发送数据的吞吐量。为此，我**修改了发送线程函数** usbx_cdc_acm_write_thread_entry()，**移除了示例项目中的 UART-USB 桥接设置，只向主机发送批量数据**。以下是代码：  
> >
> > ```c
> > #define MAX_PKT_SIZE  128 /* 200 */ /* 512 */ /* 1024 */
> > 
> > VOID usbx_cdc_acm_write_thread_entry(ULONG thread_input)
> > {
> > 	ULONG actual_length;
> > 	uint32_t i = 0;
> > 	uint32_t total_bytes_to_send = APP_TX_DATA_SIZE*100;
> > 	uint32_t bytes_to_send = 0;
> > 	uint32_t buf_indx = 0;
> > 
> > 	UX_PARAMETER_NOT_USED(thread_input);
> > 
> > 	for (i = 0; i < APP_TX_DATA_SIZE; i++)
> > 	{
> > 		UserTxBufferFS[i] = i;
> > 	}
> > 
> > 	tx_thread_sleep(MS_TO_TICK(10000));
> > 
> > 	while (1)
> > 	{
> > 		while(total_bytes_to_send)
> > 		{
> > 			if (total_bytes_to_send > MAX_PKT_SIZE)
> > 			{
> > 				bytes_to_send = MAX_PKT_SIZE;
> > 			}
> > 			else
> > 			{
> > 				bytes_to_send = total_bytes_to_send;
> > 			}
> > 
> > 			/* Send data over the class cdc_acm_write */
> > 			if (ux_device_class_cdc_acm_write(cdc_acm, (UCHAR *)(&UserTxBufferFS[buf_indx]),
> > 					bytes_to_send, &actual_length) == UX_SUCCESS)
> > 			{
> > 				total_bytes_to_send -= actual_length;
> > 				buf_indx += actual_length;
> > 				if (buf_indx >= APP_TX_DATA_SIZE)
> > 				{
> > 					buf_indx = 0;
> > 				}
> > 			}
> > 		}
> > 
> > 	    /* Sleep thread for 10ms */
> > 	    tx_thread_sleep(MS_TO_TICK(10));
> > 	}
> > }
> > ```
> >
> >  总共有 204800 字节的数据准备发送。MAX_PKT_SIZE 是我在每个循环中发送的数据包大小。当**我将其配置为略小于或等于 128 字节时，事务处理成功。但超过这个值，API 内部会无限期地等待中断，并一直卡在那里**，即使该 API 已根据内部配置的 UX_SLAVE_REQUEST_DATA_MAX_LENGTH（512 字节）对数据缓冲区进行分片。此示例中的其他配置均未更改。有人知道可能是什么地方出了问题吗？
> >
> > 当我在 CubeIDE 中暂停执行时，通常是在 USB_ReadInterrupts() 或 HAL_PCD_IRQHandler() 函数中。
> >
> > 我也尝试启用 UX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP，但效果相同。
> >
> > 我的最终目标是测试通过 USB CDC VCP 将数据从设备发送到 PC 的速度。 
> >
> > ST回复：
> >
> > 感谢您报告此事。
> >
> > 我们的团队非常清楚这个问题，并已提交内部工单号 171673 来修复最大数据包大小的问题：
> >
> > ```c
> > HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200);
> > HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, USBD_MAX_EP0_SIZE/4); 
> > HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, USBD_CDCACM_EPIN_HS_MPS/4);
> > HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, USBD_CDCACM_EPINCMD_HS_MPS/4);
> > ```
> >
> > > 个人注：
> > >
> > > 这个修改是修改CubeMX 自动生成代码中USB TX FiFo大小配置的问题，仅适用于USB高速，这几个宏在 ux_device_descriptors.h 中定义大小如下：
> > >
> > > ```
> > > #define USBD_MAX_EP0_SIZE                             64U
> > > #define USBD_CDCACM_EPIN_HS_MPS                       512U
> > > #define USBD_CDCACM_EPINCMD_HS_MPS                    8U
> > > ```
> > >
> > > 替换宏之后大小是：
> > >
> > > ```c
> > > HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200);    //!< 2048B 
> > > HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 64/4);  //!< 64B
> > > HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 512/4); //!< 512B
> > > HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, 8/4);   //!< 8B
> > >                                                  //!< 合计2632B
> > > ```
> > >
> > > 我是USB高速 MSC + CDC 复合设备，对 USB FIFO 大小配置有关系的宏如下
> > >
> > > ```c
> > > /* Device Storage Class */
> > > #define USBD_MSC_EPOUT_ADDR                           0x01U
> > > #define USBD_MSC_EPIN_ADDR                            0x81U
> > > #define USBD_MSC_EPOUT_FS_MPS                         64U  //!< 全速  \
> > > #define USBD_MSC_EPOUT_HS_MPS                         512U //!< 高速   |---这些大小ubeMX是锁死的不可修改
> > > #define USBD_MSC_EPIN_FS_MPS                          64U  //!< 全速  /
> > > #define USBD_MSC_EPIN_HS_MPS                          512U //!< 高速 /
> > > 
> > > /* Device CDC-ACM Class */
> > > #define USBD_CDCACM_EPINCMD_ADDR                      0x82U
> > > #define USBD_CDCACM_EPINCMD_FS_MPS                    8U
> > > #define USBD_CDCACM_EPINCMD_HS_MPS                    8U
> > > #define USBD_CDCACM_EPIN_ADDR                         0x83U
> > > #define USBD_CDCACM_EPOUT_ADDR                        0x03U
> > > #define USBD_CDCACM_EPIN_FS_MPS                       64U
> > > #define USBD_CDCACM_EPIN_HS_MPS                       512U
> > > #define USBD_CDCACM_EPOUT_FS_MPS                      64U
> > > #define USBD_CDCACM_EPOUT_HS_MPS                      512U
> > > #define USBD_CDCACM_EPINCMD_FS_BINTERVAL              5U
> > > #define USBD_CDCACM_EPINCMD_HS_BINTERVAL              5U
> > > ```
> > >
> > > 那么使用宏配置FIFO大小如下：
> > >
> > > ```c
> > >   // MSC 端点 FIFO 配置参考：案例 3：大容量存储：https://community.st.com/t5/stm32-mcus/practical-use-cases-to-manage-fifo-in-usb-otg-controllers-in/ta-p/839963
> > >   /* Set Rx FIFO to accommodate 512 words*/
> > >   HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200);                             //!<           大小：0x200 字：RX FIFO
> > > 
> > >   /* Set Tx FIFO 0 size to 16 words (64 bytes) for Control IN endpoint (EP0). */
> > >   HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, USBD_MAX_EP0_SIZE / 4);          //!< 端点号0 ：大小：0x10  字：IN 控制端点 FIFO
> > >   
> > >   /* Set Tx FIFO 1 to 256 words (1KB) for Bulk IN endpoint */
> > >   HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, USBD_MSC_EPIN_HS_MPS / 4);       //!< 端点号1 ：大小：0x100 字：MSC_ENDPOINT_IN / OUT FIFO
> > > 
> > >   // CDC ACM 端点 FIFO 配置参考：https://github.com/STMicroelectronics/x-cube-azrtos-h7/blob/main/Projects/NUCLEO-H723ZG/Applications/USBX/Ux_Device_CDC_ACM/USBX/App/app_usbx_device.c
> > >   /* Set Tx FIFO 2 */
> > >   HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, USBD_CDCACM_EPINCMD_HS_MPS / 4); //!< 端点号2： 大小：0x2   字：CDC ACM CMD FIFO
> > > 
> > >   /* Set Tx FIFO 3 */
> > >   HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, USBD_CDCACM_EPIN_HS_MPS / 4);    //!< 端点号3： 大小：0x100 字：CDC ACM BULK FIFO
> > > ```
> > >
> > > 实测，使用上述FIFO大小，在CubeMX配置中仅以下一个配置不同： 
> > >
> > > ```c
> > > UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE ENABLE //!< 可以正常枚举 MSC 和 CDC ACM （CDC ACM 阻塞式）
> > >  UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE ENABLE //!< 不能正常枚举 MSC 和 CDC ACM （CDC ACM 非阻塞式）
> > > ```

**RTOS 要想 CDC ACM 非阻塞发送，将 UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE  改为 DISABLE：**

然后参考这个帖子：https://community.st.com/t5/stm32-mcus-products/stm32u5a5-usb-cdc-issue/td-p/641916

> UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE 在 CubeMX中USBX 配置里默认是 ENABLE，意思是**禁用 USBX CDC ACM 类的「非阻塞（异步）传输」功能**，强制所有 CDC ACM 数据发送操作只能使用「阻塞（同步）模式」。
>
> ![](Images/USBX_MSC_CDC复合设备配置：03：UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE设置为DISABLE.png)

之后让 CubeMX 生成一次代码，那么 `ux_device_class_cdc_acm.h` 的 `UX_SLAVE_CLASS_CDC_ACM` 结构体中，以下成员都变成可使用的状态：

```c
typedef struct UX_SLAVE_CLASS_CDC_ACM_STRUCT
{
......
#ifndef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
#if !defined(UX_DEVICE_STANDALONE)
    UX_THREAD                           ux_slave_class_cdc_acm_bulkin_thread;
    UX_THREAD                           ux_slave_class_cdc_acm_bulkout_thread;
    UX_EVENT_FLAGS_GROUP                ux_slave_class_cdc_acm_event_flags_group;
    UCHAR                               *ux_slave_class_cdc_acm_bulkin_thread_stack;
    UCHAR                               *ux_slave_class_cdc_acm_bulkout_thread_stack;
#endif
    UINT                                (*ux_device_class_cdc_acm_write_callback)(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, ULONG length); //!< 发送完成后的回调函数
    UINT                                (*ux_device_class_cdc_acm_read_callback)(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length); //!< 接收完成后的回调函数
    ULONG                               ux_slave_class_cdc_acm_transmission_status;
    ULONG                               ux_slave_class_cdc_acm_scheduled_write;
#if !defined(UX_DEVICE_STANDALONE)
    ULONG                               ux_slave_class_cdc_acm_callback_total_length;
    UCHAR                               *ux_slave_class_cdc_acm_callback_data_pointer;
    UCHAR                               *ux_slave_class_cdc_acm_callback_current_data_pointer;
#endif
#endif
} UX_SLAVE_CLASS_CDC_ACM;

```

其中 CDC 收发完成的回调函数指针 `ux_device_class_cdc_acm_write_callback` 和 `ux_device_class_cdc_acm_read_callback` 在 `ux_device_class_cdc_acm_ioctl.c` 中的 `_ux_device_class_cdc_acm_ioctl()` 中， 当传入参数 `ioctl_function` 为 `UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START` 时，绑定传入的 `parameter` 参数

```c
UINT _ux_device_class_cdc_acm_ioctl(UX_SLAVE_CLASS_CDC_ACM *cdc_acm, ULONG ioctl_function,
                                    VOID *parameter)
{
......
    switch (ioctl_function)
    {
        ......  
        case UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START:
            /* Properly cast the parameter pointer.  */
            callback = (UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER *) parameter;

            /* Save the callback function for write.  */
            cdc_acm -> ux_device_class_cdc_acm_write_callback  = callback -> ux_device_class_cdc_acm_parameter_write_callback;

            /* Save the callback function for read.  */
            cdc_acm -> ux_device_class_cdc_acm_read_callback = callback -> ux_device_class_cdc_acm_parameter_read_callback;
        ......
    }
......
}
```

绑定回调函数的函数指针的在 UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER 结构体定义中有对应定义

```c
typedef struct UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER_STRUCT 
{
    UINT                                (*ux_device_class_cdc_acm_parameter_write_callback)(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, ULONG length);
    UINT                                (*ux_device_class_cdc_acm_parameter_read_callback)(struct UX_SLAVE_CLASS_CDC_ACM_STRUCT *cdc_acm, UINT status, UCHAR *data_pointer, ULONG length);

} UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER;
```

在 `ux_device_class_cdc_acm.h` 中宏 `ux_device_class_cdc_acm_ioctl` 定义为 `_ux_device_class_cdc_acm_ioctl`，因此实际要调用 `ux_device_class_cdc_acm_ioctl` 进行绑定回调函数操作

```
#define ux_device_class_cdc_acm_ioctl               _uxe_device_class_cdc_acm_ioctl
```



在 `_ux_device_class_cdc_acm_write_with_callback()` 中，通过发送事件组 `ux_slave_class_cdc_acm_event_flags_group` 来调用 BULK IN 线程

```c
UINT _ux_device_class_cdc_acm_write_with_callback(UX_SLAVE_CLASS_CDC_ACM *cdc_acm, UCHAR *buffer,
                                ULONG requested_length)
{
......
    /* Invoke the bulkin thread by sending a flag .  */
    status = _ux_device_event_flags_set(&cdc_acm -> ux_slave_class_cdc_acm_event_flags_group, UX_DEVICE_CLASS_CDC_ACM_WRITE_EVENT, UX_OR);
......
}
```

在 `_ux_device_class_cdc_acm_bulkin_thread()` 中，需要获取事件组 `ux_slave_class_cdc_acm_event_flags_group` 解除阻塞

解除阻塞后，如果获取事件的值的 `UX_SUCCESS`，则调用发送完成回调函数 `ux_device_class_cdc_acm_write_callback`

```c
VOID  _ux_device_class_cdc_acm_bulkin_thread(ULONG cdc_acm_class)
{
......
    /* This thread runs forever but can be suspended or resumed.  */
    while(1)
    {
...... // 第155行
            /* Wait until we have a event sent by the application. */
            status =  _ux_utility_event_flags_get(&cdc_acm -> ux_slave_class_cdc_acm_event_flags_group, UX_DEVICE_CLASS_CDC_ACM_WRITE_EVENT, UX_OR_CLEAR, &actual_flags, UX_WAIT_FOREVER);
......
            /* Check the completion code. */
            if (status == UX_SUCCESS)
            {
...... // 第264行
                /* Schedule of transmission was completed.  */
                cdc_acm -> ux_slave_class_cdc_acm_scheduled_write = UX_FALSE;

                /* We get here when the entire user data payload has been sent or if there is an error. */
                /* If there is a callback defined by the application, send the transaction event to it.  */
                if (cdc_acm -> ux_device_class_cdc_acm_write_callback != UX_NULL)

                    /* Callback exists. */
                    cdc_acm -> ux_device_class_cdc_acm_write_callback(cdc_acm, status, sent_length);

                /* Now we return to wait for an event from the application or the user to stop the transmission.  */
            }
......
}
```

**那么我需要在哪里写入绑定CDC类回调函数的代码呢？**

注意到，在 `app_usbx_device.c` 的 `MX_USBX_Device_Init()` 中有将参数绑定 `UX_SLAVE_CLASS_CDC_ACM_PARAMETER` 结构体类型变量 `cdc_acm_parameter` 成员的代码，MSC类和CDC类都在此进行绑定参数

```c
......
static UX_SLAVE_CLASS_STORAGE_PARAMETER storage_parameter; // MSC类参数
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter; // CDC类参数
......
UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
......
  /* Initialize the storage class parameters for reading/writing to the Flash Disk */
  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_last_lba = USBD_STORAGE_GetMediaLastLba();
......
  /* USER CODE BEGIN STORAGE_PARAMETER */
  // 用户自定义MSC类的参数绑定代码
  /* USER CODE END STORAGE_PARAMETER */
......
  /* Initialize the cdc acm class parameters for the device */
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate   = USBD_CDC_ACM_Activate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = USBD_CDC_ACM_Deactivate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change    = USBD_CDC_ACM_ParameterChange;
    
  /* USER CODE BEGIN CDC_ACM_PARAMETER */
  // 用户自定义CDC类的参数绑定代码
  /* USER CODE END CDC_ACM_PARAMETER */
......
}
```

`UX_SLAVE_CLASS_STORAGE_PARAMETER`、`UX_SLAVE_CLASS_CDC_ACM_PARAMETER` 结构体类型都在 `ux_device_class_cdc_acm.h` 定义，注意到上位提到的 `UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER`也在此定义，所以将绑定回调函数的代码在 `MX_USBX_Device_Init()` 的 `/* USER CODE BEGIN CDC_ACM_PARAMETER */` 和  `/* USER CODE END CDC_ACM_PARAMETER */` 之间写，并在此文件中定义一个 `UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER` 类型的静态变量

```c
......
static UX_SLAVE_CLASS_STORAGE_PARAMETER storage_parameter; // MSC类参数
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter; // CDC类参数
static UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER cdc_acm_callback_parameter; //!< 参数绑定 CDC ACM 回调函数
......
UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
......
  /* Initialize the storage class parameters for reading/writing to the Flash Disk */
  storage_parameter.ux_slave_class_storage_parameter_lun[0].
    ux_slave_class_storage_media_last_lba = USBD_STORAGE_GetMediaLastLba();
......
  /* USER CODE BEGIN STORAGE_PARAMETER */
  // 用户自定义MSC类的参数绑定代码
  /* USER CODE END STORAGE_PARAMETER */
......
  /* Initialize the cdc acm class parameters for the device */
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate   = USBD_CDC_ACM_Activate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = USBD_CDC_ACM_Deactivate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change    = USBD_CDC_ACM_ParameterChange;
    
  /* USER CODE BEGIN CDC_ACM_PARAMETER */
  // 用户自定义CDC类的参数绑定代码
  /* USER CODE END CDC_ACM_PARAMETER */
......
}
```

`MX_USBX_Device_Init()` 被 `tx_application_define()` 中调用

`tx_application_define()`  被 `_tx_initialize_kernel_enter()` 调用