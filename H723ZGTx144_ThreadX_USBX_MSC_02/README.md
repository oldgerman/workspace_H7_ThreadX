## H723ZGTx144_ThreadX_USBX_MSC_02

## 关于

在 H723ZGTx144_ThreadX_USBX_MSC_02 基础上将主内存从 AXISRAM 切换为 DTCM，

使用 attribute 将放在DTCM中导致不能正常工作的变量放到 .axisram2_bss和 .axisram2_data，MPU不开cache和buffer

已经修改 .ld 链接脚本文件和 .s启动文件

main函数中已经复制中断向量表到DTCM

## 配置

参考：[STM32H735G-DK/Applications/USBX/Ux_Device_MSC](https://github.com/STMicroelectronics/x-cube-azrtos-h7/tree/main/Projects/STM32H735G-DK/Applications/USBX/Ux_Device_MSC)

> #### **USBX 使用提示**
>
> - 如果启用了 USB DMA，则应用程序不应使用 DTCM (0x20000000) 内存区域。
> - 应确保将 **USB 内存池**区域配置为“**不可缓存**（Non-Cacheable）”属性，以确保 CPU 和 USB DMA 之间的一致性。

## 一些思考

