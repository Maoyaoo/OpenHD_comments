# Summary

This submodule is responsible for starting and configuring all the OpenHD
interfaces - aka all OpenHD links like wifibroadcast (communication between air and ground),
ground hotspot, ...

For now, it requires at least one wifi card connected to the system - this might change in the 
future when we add other hardware types for data communication between ground and air pi
like LTE cards.

Note that some modules handle HW connection(s) themselves, for example telemetry does the UART
connection to the FC (even though one could reason UART is a HW interface).

## Created WB links for openhd-telemetry and openhd-video:
1) Bidirectional link made up of 2 wifibroadcast instances for telemetry up / down, both on air and ground
2) 2 Unidirectional links for video down from air pi to ground pi (primary and secondary video stream)
   -> NOTE: Video only goes from air pi to ground pi, so we need 2 tx instances on air pi and 2 rx instances on ground
   pi

look at the code to find out the ports usw, they should come from openhd-constants.hpp

## Keys
The purpose of encryption is less about security and more about avoiding packet collisions.
R.n encryption is mandatory for wifibroadcast, but to make development easier, we just use a default seed
to create the tx and rx files. We are in the progress in changing that, The OpenHD image writer already allows
the user to set a bind phrase.

You can use [wifibroadcast repo](https://github.com/openhd/wifibroadcast) to generate a key pair. The keys should be placed in $SETTINGS_BASE_PATH$/interface


## 总结
此子模块负责启动和配置所有 OpenHD 接口，即所有 OpenHD 链路，如 wifibroadcast（空中与地面之间的通信）、地面热点等。
目前，它至少需要系统连接一张无线网卡。未来，当我们添加诸如 LTE 卡等其他用于地面与空中 Pi 之间数据通信的硬件类型时，这种情况可能会改变。
请注意，有些模块自行处理硬件连接，例如，遥测模块通过 UART 连接到飞控（尽管有人可能认为 UART 是一种硬件接口）。
## 为 openhd-telemetry and openhd-video创建的 WB 链路：
双向链路，由 2 个 wifibroadcast 实例组成，用于空中和地面的遥测数据上传 / 下载。
两条单向链路，用于将视频从空中 Pi 下行传输到地面 Pi（主视频流和次视频流）。
-> 注意：视频仅从空中 Pi 传输到地面 Pi，因此我们在空中 Pi 上需要 2 个发送实例，在地面 Pi 上需要 2 个接收实例。
查看代码以了解端口等信息，它们应该来自 openhd-constants.hpp。
## 密钥
加密的目的与其说是为了安全，不如说是为了避免数据包冲突。对于 wifibroadcast，加密是必需的，但为了简化开发，我们仅使用默认种子来创建发送和接收文件。我们正在对此进行改进，OpenHD 镜像写入器已经允许用户设置绑定短语。
你可以使用 wifibroadcast 仓库(https://github.com/openhd/wifibroadcast) 生成密钥对。密钥应放置在$SETTINGS_BASE_PATH$/interface 中。