## Summary

OpenHD core (main.cpp) is a c++ executable that creates a working openhd air
or ground unit. While dependencies and more can be validated / checked at compile time,
there are a few 'funky' ways where openhd interacts with the underlying OS. Here is a quick
list of those things:

OpenHD 核心（main.cpp）是一个 C++ 可执行程序，用于创建可用的 OpenHD 空中或地面单元。虽然依赖项等内容可以在编译时进行验证 / 检查，但 OpenHD 与底层操作系统的交互存在一些 “独特” 的方式。以下是这些情况的简要列表：

1) The correct wifi drivers need to be installed
2) All settings (changeable via mavlink and/or used internally) are stored under
    /usr/local/share/openhd/. This directory needs to be write / readable
3) To change CSI cameras on rpi, /usr/local/bin/ohd_camera_setup.sh is called
4) For all wifi hotspot / client functionalities, network manager needs to be installed.
    The wifibroadcast card(s) are taken from nw at start and given back on proper termination
5) For wifi card discovery, iw needs to be installed
6) For usb camera(s), some v4l2 commands might be called

1需要安装正确的 WiFi 驱动程序。
2所有设置（可通过 MAVLink 更改和 / 或内部使用）存储在 /usr/local/share/openhd/ 目录下。该目录需要具有读写权限。
3要在树莓派（Raspberry Pi）上更改 CSI 摄像头，会调用 /usr/local/bin/ohd_camera_setup.sh。
4对于所有 WiFi 热点 / 客户端功能，需要安装网络管理器。在启动时会从网络中获取无线广播卡，并在正常终止时归还。
5对于 WiFi 卡的发现，需要安装 iw。
6对于 USB 摄像头，可能会调用一些 v4l2 命令。