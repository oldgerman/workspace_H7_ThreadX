## H723ZGTx144_ThreadX_USBX_MSC_01

## 关于

软件

- RTOS：ThreadX

- USBX：MSC类

- FileX：SD卡驱动

- 编译：GCC，Os优化

- 使用 SD 卡 DMA 读写 API

  > 在 ux_device_msc.c 中
  >
  > USBD_STORAGE_Read() 调用 HAL_SD_ReadBlocks_DMA()
  >
  > USBD_STORAGE_Write() 调用 HAL_SD_WriteBlocks_DMA()

- SDMMC2：使能了硬件流控，时钟频率 50MHz

- AXISRAM：[参考硬汉教程配置为最低性能NORMAL](https://www.cnblogs.com/armfly/p/16245867.html)

  ```c
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x24000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.SubRegionDisable = 0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  ```

硬件

- USB高速PHY：USB3300
- SD卡：三星 64G EVO（C10）

SD卡格式化工具

- SD Card Formatter 格式化为 exFAT

下载到开发板后，WIN11 25H2 正常识别到一块 59.4G的U盘

## Demo

可以正常CRUD各种文件，例如打开文本文件，编辑内容后保存、重命名文件等

| ![Demo：UsbTreeView](Images/Demo：UsbTreeView.png)           | ![Demo：DiskGenius](Images/Demo：DiskGenius.png)             |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| UsbTreeView                                                  | DiskGenius                                                   |
| ![Demo：U盘存放文件后正常显示剩余容量](Images/Demo：U盘存放文件后正常显示剩余容量.png) | ![Demo：拷贝各种后缀名文件到U盘](Images/Demo：拷贝各种后缀名文件到U盘.png) |
| U盘存放文件后正常显示剩余容量                                | 拷贝各种后缀名文件到U盘                                      |
| ![Demo：从U盘剪切100+MB视频到电脑](Images/Demo：从U盘剪切100+MB视频到电脑.png) | ![Demo：从电脑拷贝700MB视频到U盘](Images/Demo：从电脑拷贝700MB视频到U盘.png) |
| 从U盘剪切100+MB视频到电脑                                    | 从电脑拷贝700MB视频到U盘                                     |
| ![](Images/Demo：U盘打开100+MB正常播放并快进.png)            | ![Demo：U盘打开mp3正常播放并快进](Images/Demo：U盘打开mp3正常播放并快进.png) |
| U盘打开100+MB视频正常播放并快进                              | U盘打开mp3正常播放并快进                                     |

