## H723ZGTx144_ThreadX_USBX_PIMA_CDC_01

在 H723ZGTx144_ThreadX_USBX_MSC_CDC_01 的基础上修改，删除 MSC 类，加入 CDC ACM 类，与 PIMA 实现 USBX 复合设备

## 参考

### PIMA相关帖子

[安富莱论坛：FileX+USBX_MSC_双边同步问题](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=129472&page=1#pid346429)

> *发表于 2025-8-20 09:50:20* | [只看该作者](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=129472&page=1&authorid=84914) |[倒序浏览](https://forum.anfulai.cn/forum.php?mod=viewthread&tid=129472&extra=&ordertype=1) |[阅读模式](javascript:;)
>
> [JasonChen](https://forum.anfulai.cn/home.php?mod=space&uid=84914)
>
> > 目前使用的存储介质是SD卡，进行**fx_media_format**格式化之后，可以被PC正常识别，写入读取都是正常的。 但是存在本地和PC端不同步的问题，比如在本地创建或写入文件，创建或写入后立即调用了**fx_media_flush**，本地已经生效，PC端也不会立即刷新出文件，需要重新枚举才能正常显示。 同样通过PC端写入文件，需要重新**fx_media_open**本地才能看到PC端上次写入的文件。  造成这样的现象，貌似是两边在维护不同的FAT表。请问有什么好的解决方法吗？
> >
> > 弃用MSC了，改为PIMA MTP，由设备端统一管理文件系统，可以完美兼容
>
> [eric2013](https://forum.anfulai.cn/home.php?mod=space&uid=58)：
>
> > 是的mtp是非常好的方案
>
> OldGerman：
>
> > 楼主非常感谢你提到PIMA，其实手机和电脑传文件就在用这个协议，但是我之前一直以为是MSC哈哈
> >
> > 我目前卡壳儿的情况和楼主非常相似，ThreadX USBX整了一个MSC和CDC ACM的复合设备，电脑可以读写SD卡，通过虚拟串口发送一个FileX 读写测速SD卡的命令，安富莱的测速例程也能跑完，但是这之后电脑就不能打开SD卡的任何文件了（点击后一直转圈），又过了一会儿windows就把USBX整个设备都卸载了，尝试了FileX读写测速期间仅取消注册USBX 复合设备中的MSC线程，测速完毕后重新注册线程，但情况依然，表现为电脑右键U盘弹出，U盘仍然在电脑上，感觉复合设备是不能热卸载和热重载其中的一个设备类的，wiki搜索了一下PIMA确实相比MSC优势很大，那么就换PIMA MTP开整
> >
> > ![MTP协议相比MSC协议的优势](Images/MTP协议相比MSC协议的优势.png)
>
> [walk](https://forum.anfulai.cn/home.php?mod=space&uid=20152)
>
> > mtp有demo吗，官方的demo怎么把默认目录下的文件罗列出来，没用文件夹的层次信息。
>
> [eric2013](https://forum.anfulai.cn/home.php?mod=space&uid=58)
>
> > 我们自己没做过，只有官方的那个Demo

