## Summary

The responsibilities of this module depend on weather it us used on air or ground.
# AIR
1) Detect connected cameras (can be manually overridden)
2) Setup a pipeline that generates a continuous stream of encoded video data (h264 or h265) for a detected camera
   This data is forwarded via a callback (previously udp port) to ohd_interface for transmission via wb
3) Store and change camera/encoder-related settings
   Camera settings are stored in SETTINGS_BASE_PATH/video (one unique file for each camera)
   The Camera manifest (list of discovered cameras) can be found under /tmp/camera_manifest for debugging.

# Ground
1) Forward the received video data via RTP, UDP to other consuming applications 
   (e.g. QOpenHD for display). Those application(s) can run on localhost, or on externally connected devices
   (e.g. android smartphone via USB tethering)

## Note 
The code in this module must adhere to the following paradigms:
1) It only generates encoded video data,then forwards it. It doesn't know if the video data is actually picked up or makes it to the
   ground.
2) A camera and it's settings are located on the air unit - you can change / query param(s) via mavlink from the ground unit, but we keep the ground
   completely agnostic to the camera(s) and their settings for a good reason !
3) There are no code dependencies to other modules like ohd_interface.
4) to adhere with 1), for h264/h265 streaming, re-send the "configuration data" (aka SPS,PPS,key frame for h264, SPS,PPS,VPS,key frame for h265)
in regular intervals. This way the decoding application can start the video decoding after max. 1 interval size, assuming a connection
without packet drops


## 总结
此模块的职责取决于它是在空中还是地面使用。
# 空中
1) 检测已连接的摄像头（可手动覆盖检测结果）。
2) 搭建一个管道，为检测到的摄像头生成连续的编码视频数据流（H264 或 H265）。
这些数据通过回调（以前是通过 UDP 端口）转发给 ohd_interface，以便通过 wifibroadcast 进行传输。
3) 存储和更改与摄像头 / 编码器相关的设置。
摄像头设置存储在 SETTINGS_BASE_PATH/video 目录下（每个摄像头有一个唯一的文件）。
摄像头清单（发现的摄像头列表）可在 /tmp/camera_manifest 中找到，用于调试。
# 地面
通过 RTP、UDP 将接收到的视频数据转发给其他使用该数据的应用程序，
（例如用于显示的 QOpenHD）。这些应用程序可以在本地主机上运行，也可以在外部连接的设备上运行
（例如通过 USB 共享网络连接的安卓智能手机）。
## 注意
此模块中的代码必须遵循以下范式：
1) 它仅生成编码视频数据，然后转发。它不知道视频数据是否真的被接收或是否能到达地面。
2) 摄像头及其设置位于空中单元 - 你可以通过地面单元的 Mavlink 更改 / 查询参数，但我们让地面
完全不了解摄像头及其设置是有充分理由的！
代码与其他模块（如 ohd_interface）没有依赖关系。
为遵循第 1 点，对于 H264/H265 流，要定期重新发送 “配置数据”（即 H264 的 SPS、PPS、关键帧，H265 的 SPS、PPS、VPS、关键帧）。
这样，假设连接无丢包情况，解码应用程序最多在一个间隔时间后就能开始视频解码。