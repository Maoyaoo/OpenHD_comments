## Summary

This subdirectory contains all the code that is needed to build the OpenHD executable
that is then run on the air and ground platform to create OpenHD.
Note that this executable assumes that some modifications have been applied to the underlying linux OS,
like patched wifi drivers and installed linux utility programs.

此子目录包含构建 OpenHD 可执行文件所需的所有代码，该可执行文件随后将在空地平台上运行以创建 OpenHD。
请注意，该可执行文件假定底层 Linux 操作系统已进行了一些修改，例如打了补丁的 Wi-Fi 驱动程序和已安装的 Linux 实用程序。

## List of Assumptions about the linux system we are running on that are non-standard:
关于我们正在运行的非标准 Linux 系统的假设列表
(This might be incomplete)
1) Wifi drivers are patched to support monitor mode for all WiFi cards OpenHD supports
(Or at least the wifi card you then connect to the system and that is used by OpenHD for wifibroadcast)
This sounds simple, but it is not. Also, stuff like disabling weird services that would interfer with monitor
mode or similar falls into this category.
2) The directory /tmp for writing temporary files exists
3) The directory "SETTINGS_BASE_PATH" for writing persistent (setting) files exists / can be created by OpenHD and is read/writeable
(Make sure it is read / writeable on embedded devices)
4) The directory /home/openhd/Videos for storing video recording(s) on the air unit can be created by OpenHD
5) UART: if the platform has an UART connector (like rpi GPIO), the UART is enabled and has a corresponding linux file handle
（该列表可能不完整）
1）Wi-Fi 驱动程序已打补丁，以支持 OpenHD 所支持的所有 Wi-Fi 卡的监控模式
（或者至少支持你随后连接到系统且 OpenHD 用于 Wi-Fi 广播的 Wi-Fi 卡）。
这听起来简单，但实则不然。此外，诸如禁用可能干扰监控模式或类似情况的奇怪服务也属于这一类。
2)用于存储临时文件的目录 /tmp 存在。
3) 用于存储持久（设置）文件的目录 “SETTINGS_BASE_PATH” 存在 / 可由 OpenHD 创建，并且是可读写的
（确保在嵌入式设备上它是可读写的）。
4) 空中单元上用于存储视频录制的目录 /home/openhd/Videos 可由 OpenHD 创建。
5) UART：如果平台有一个 UART 连接器（如树莓派的 GPIO），则 UART 已启用且有相应的 Linux 文件句柄。

## Note about connected Hardware:
While it is nice for the user to Hotplug new Hardware, this is not always feasible. For example,to allow easier debugging, we discover connected
cameras on startup if running as air and if no camera is found, emulate the primary camera in SW.
Similar to interface, where we detect the connected wifi cards at startup.
This can be generally described by having a discovery step at startup - once the discovery step has
been performed, we cannot check for changes on this discovered hardware. However, hardware that needs
to be hot-pluggable (for example the FC) in general uses a different pattern - check in regular
intervals if the HW configuration has changed, then react to these changes

关于所连接硬件的说明：
虽然用户热插拔新硬件很不错，但这并不总是可行的。例如，为了便于调试，如果作为空中端运行时，我们会在启动时发现所连接的摄像头，如果未找到摄像头，则在软件中模拟主摄像头。
与接口类似，我们会在启动时检测所连接的 Wi-Fi 卡。
这通常可以描述为在启动时有一个发现步骤 —— 一旦执行了发现步骤，我们就无法检查此已发现硬件的变化。然而，需要热插拔的硬件（例如飞行控制器）通常使用不同的模式 —— 定期检查硬件配置是否已更改，然后对这些更改做出反应。