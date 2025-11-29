## H723ZGTx144_ThreadX_USBX_MSC_01

## 关于

软件

- RTOS：ThreadX
- USBX：MSC类
- FileX：SD卡驱动
- 编译：GCC，Os优化

硬件

- USB高速PHY：USB3300
- SD卡：三星64G EVO （C10）

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
| U盘打开100+MB正常播放并快进                                  | U盘打开mp3正常播放并快进                                     |

