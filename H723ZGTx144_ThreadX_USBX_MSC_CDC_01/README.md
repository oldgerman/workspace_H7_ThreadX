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
> 我需要在MSC类之后注册一个CDC类实现复合设备，而 CubeMX 里 UX_MAX_SLAVE_CLASS_DRIVER  还是默认值 1，需要改为 2 解决





- USBD_CDCACM_ENDPOINT_IN_CMD_ADDR：2

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

- USBD_CDCACM_ENDPOINT_IN_ADDR：3

- USBD_CDCACM_ENDPOINT_IN_FS_MPS：64

- USBD_CDCACM_ENDPOINT_IN_HS_MPS：512

- USBD_CDCACM_ENDPOINT_OUT_ADDR：3

- USBD_CDCACM_ENDPOINT_OUT_FS_MPS：64

- USBD_CDCACM_ENDPOINT_OUT_HS_MPS：512

- UX_DEVICE_CLASS_CDC_ACM_ZERO_COPY：DISABLED

  > 零拷贝暂不开启，待研究

- UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE：ENABLED

- UX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP：ENABLED

[分享基于安富莱ThreadX全家桶2.0版本实现的USBX CDC ACM+PPP连接服务](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=123737)

## 参考例程

Ux_Device_CDC_ACM

> ST的USBX设备例程：CDC_ACM
>
> [x-cube-azrtos-h7-main\x-cube-azrtos-h7-main\Projects\NUCLEO-H723ZG\Applications\USBX\Ux_Device_CDC_ACM](https://github.com/STMicroelectronics/x-cube-azrtos-h7/tree/main/Projects/NUCLEO-H723ZG/Applications/USBX/Ux_Device_CDC_ACM)
>
> 此例程实现 USB CDC ACM 转 USART 桥，使用了事件标志组 EventFlag 与 H723ZGTx144_ThreadX_USBX_MSC_02 工程 MSC 使用的事件标志组同名，COPY代码时需要区分！

Ux_Device_HID_CDC_ACM

> ST的USBX设备例程：HID和CDC_ACM复合
>
> [x-cube-azrtos-h7-main\x-cube-azrtos-h7-main\Projects\STM32H747I-DISCO\Applications\USBX\Ux_Device_HID_CDC_ACM](https://github.com/STMicroelectronics/x-cube-azrtos-h7/tree/main/Projects/STM32H747I-DISCO/Applications/USBX/Ux_Device_HID_CDC_ACM)
>
> PS：还有一个 H735 的主机HID和CDC_ACM复合例程 Ux_Host_HID_CDC_ACM，以后要是用得上可以看看

