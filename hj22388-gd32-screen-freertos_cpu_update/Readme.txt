7寸，bandage-rk3399

2022-12-09，version1.0.2，by dazhi
1.7寸和5寸主要的问题是 设备型号不能区分，串口读设备型号指令0x88，返回值一个是0（5寸），一个是1（7寸）

2.5寸，7寸，new5寸，都是使用跟这个源码更新，需要修改bsp_commm_uart.c
中DEVICE_TYPE(113行)的对应值（0x05：5寸屏；0x04：7寸屏,0x06: new5寸屏）

3.VERSION(bsp_commm_uart.c，114行)，升级版本为2 （2022-12-09）

4.增加调试串口中编译时间的打印信息。

5.修改version.h 中版本号为 1.0.2

6.原5寸屏使用project-5inch中的工程，7寸屏使用project-5inch中的工程，新5寸屏使用project-new5inch中的工程
主要的区别就是bsp_commm_uart.c使用了不同的文件（看app目录中），不用再去修改了。

7.编译输出的文件全部在output 目录中






2022-12-15 导光面板
1.根据原7寸的单片机程序移植，更换freertos
2.原程序移植后，总是进入HardFault_Handler，原因未知，与FreeRTOSConfig.h里的配置有关
3.注意外设的中断优先级，太高了会导致程序异常，与FreeRTOSConfig.h里的配置configMAX_PRIORITIES有关，不能大于这个值
4.对几个任务的优先级进行了调整，freertos 数值越大，优先级越高
5.对两个串口任务的获取信号量，释放信号量进行了优化，提高系统执行效率
6.去除了原代码中systick的初始化，由freertos自动处理。
7.优化led闪烁任务的语句，更加简洁高效
8.通过延迟背光生效的时间，改善开机时lcd出现残影花屏的问题，目前现象已解决。
9.修改version.h 中版本号为 1.0.3


2022-12-30
1.5寸屏使用“project-old5inch”中的工程
2.7寸屏使用“project-7inch”中的工程
3.新5寸屏使用“project-new5inch”中的工程



2023-04-27
1.屏幕亮度初始值修改。s_old_level 默认为0，导致亮屏和息屏的操作有些问题，已经改进


2023-04-28
1.单片机串口升级功能已增加。与gd32f103-iap -2023-04-25-for-dg_lcd这个iap程序配合
2.单片机程序不能直接烧录，因为它的flash地址不是从0开始了。！！！！！
3.主要是参考hj22134的单片机程序改进的。





