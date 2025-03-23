/******************************************************************************
 * OpenHD
 *
 * Licensed under the GNU General Public License (GPL) Version 3.
 *
 * This software is provided "as-is," without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose, and non-infringement. For details, see the
 * full license in the LICENSE file provided with this source code.
 *
 * Non-Military Use Only:
 * This software and its associated components are explicitly intended for
 * civilian and non-military purposes. Use in any military or defense
 * applications is strictly prohibited unless explicitly and individually
 * licensed otherwise by the OpenHD Team.
 *
 * Contributors:
 * A full list of contributors can be found at the OpenHD GitHub repository:
 * https://github.com/OpenHD
 *
 * © OpenHD, All Rights Reserved.
 ******************************************************************************/

#ifndef OPENHD_ETHERNET_MANAGER_H
#define OPENHD_ETHERNET_MANAGER_H

#include <string>

#include "openhd_spdlog_include.h"

//
// See networking_settings for more info
//
// 定义了 EthernetManager 类，用于管理以太网相关的功能，特别是在 OpenHD 项目中。其主要目的是在以太网连接的情况下，自动配置和转发数据（如视频和遥测数据），以适应不同的操作模式。
// 热点模式：以太网作为热点，主动为其他设备提供 IP 地址和连接。
// 外部设备模式：以太网作为客户端，等待外部设备为其提供 IP 地址和连接，然后启动数据转发。
class EthernetManager {
   public:
    explicit EthernetManager();

    void async_initialize(int operating_mode);  // 异步初始化函数，根据操作模式启动以太网管理

    void stop();  // 停止以太网管理

   private:
    void loop(int operating_mode);                                       // 主循环函数，根据操作模式运行以太网配置任务
    void configure(int operating_mode, const std::string& device_name);  // 配置以太网设备的函数，根据操作模式和设备名称进行配置

    std::shared_ptr<spdlog::logger> m_console;  // 日志记录器，用于输出调试信息
    std::shared_ptr<std::thread> m_thread;      // 线程对象，用于运行异步任务
    std::atomic_bool m_terminate = false;       // 原子布尔变量，用于控制线程的终止

   private:
    // Same/Similar pattern as usb_tether_listener.h
    // For automatically forwarding data to device(s) connected via Ethernet when
    // the Ethernet is NOT a hotspot, but rather waits for someone to provide
    // internet / dhcpcd. Waits for someone to give the pi an ip / internet via
    // ethernet, and start / stop automatic video and telemetry forwarding. Not
    // really recommended - the ethernet hotspot functionality is much more
    // popular and easier to implement.
    // 与 usb_tether_listener.h 中类似的模式
    // 用于在以太网不是热点模式时，自动转发数据到通过以太网连接的设备
    // 该模式等待外部设备为树莓派提供 IP 地址或互联网连接，然后启动或停止视频和遥测数据的自动转发
    // 注意：此模式不推荐使用，以太网热点功能更受欢迎且更容易实现
    void loop_ethernet_external_device_listener(const std::string& device_name);
};

#endif  // OPENHD_ETHERNET_MANAGER_H
