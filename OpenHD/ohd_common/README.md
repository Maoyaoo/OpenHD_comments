This library serves the following purposes:
1) Hold code that is used commonly throughout openhd 
2) Build and publicly expose spdlog and nlohmann::json to openhd (we use them both
   throughout the project)
3) Since ohd_interface, ohd_telemetry and ohd_video are allowed to depend on ohd_common
    but not on each other, glue them together in case they need to talk to each other
   (for example, when wb_link from ohd_interface wants to tell a camera in ohd_video to change the bitrate)

It is also a bit of a dump for things that don't really have their own submodule -
please take care to not pollute ohd_common with weird dependencies though.

这个库有以下几个用途：
1、存放整个 OpenHD 项目中通用的代码。
2、构建 spdlog 和 nlohmann::json 并将它们公开提供给 OpenHD（我们在整个项目中都会使用它们）。
3、由于 ohd_interface、ohd_telemetry 和 ohd_video 允许依赖于 ohd_common，但不允许相互依赖，因此在它们需要相互通信时将它们连接在一起（例如，当 ohd_interface 中的 wb_link 想要告知 ohd_video 中的摄像头更改比特率时）。
它也有点像一个存放那些没有自己子模块的东西的仓库 —— 不过请务必注意不要用奇怪的依赖项污染 ohd_common。