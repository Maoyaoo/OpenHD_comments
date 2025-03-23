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

#ifndef OPENHD_OPENHD_OHD_INTERFACE_INC_USBHOTSPOT_H_
#define OPENHD_OPENHD_OHD_INTERFACE_INC_USBHOTSPOT_H_

#include <openhd_external_device.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

#include "openhd_spdlog.h"
#include "openhd_util.h"
#include "openhd_util_filesystem.h"
// USBTetherListener 类，用于监听和管理 USB 网络共享设备的连接和断开事件。通过持续检查设备的连接状态，通知上层关于设备的 IP 地址变化，以便开始或停止向该 IP 地址转发数据。
/**
 * USB hotspot (USB Tethering).
 * Since the USB tethering is always initiated by the user (when he switches USB
 * Tethering on on his phone/tablet) we don't need any settings or similar, and
 * checking once every second barely uses any CPU resources. This was created by
 * translating the tether_functions.sh script from wifibroadcast-scripts into
 * c++. This class configures and forwards the connect and disconnect event(s)
 * for a USB tethering device, such that we can start/stop forwarding to the
 * device's ip address. Only supports one USB tethering device connected at the
 * same time. Also, assumes that the usb tethering device always shows up under
 * /sys/class/net/usb0. Note that we do not have to perform any setup action(s)
 * here - network manager does that for us We really only listen to the event's
 * device connected / device disconnected and forward them.
 */
/**
 * USB 热点（USB 网络共享）。
 * 由于 USB 网络共享始终由用户发起（例如用户在手机或平板上启用 USB 网络共享），
 * 因此我们不需要任何设置或类似的功能，每隔一秒检查一次几乎不会占用 CPU 资源。
 * 该类的实现是将 wifibroadcast-scripts 中的 tether_functions.sh 脚本翻译为 C++。
 * 该类配置并转发 USB 网络共享设备的连接和断开事件，以便我们可以开始/停止向设备的 IP 地址转发数据。
 * 仅支持同时连接一个 USB 网络共享设备。同时假设 USB 网络共享设备始终出现在 /sys/class/net/usb0。
 * 注意：我们不需要在此执行任何设置操作，网络管理器会为我们处理这些。
 * 我们实际上只是监听设备的连接/断开事件并转发它们。
 */
class USBTetherListener {
   public:
    /**
     * Creates a new USB tether listener which notifies the upper level with the
     * IP address of a connected or disconnected USB tether device.
     * @param external_device_manager connect / disconnect events are forwarded
     * using this handle
     */
    /**
     * 创建一个新的 USB 网络共享监听器，通知上层关于连接或断开连接的 USB 网络共享设备的 IP 地址。
     * @param external_device_manager 使用此句柄转发连接/断开事件
     */
    explicit USBTetherListener();
    ~USBTetherListener();

   private:
    std::shared_ptr<spdlog::logger> m_console;                 // 日志记录器
    std::unique_ptr<std::thread> m_check_connection_thread;    // 检查连接状态的线程
    std::atomic<bool> m_check_connection_thread_stop = false;  // 制线程停止的原子变量
    /**
     * Continuously checks for connected or disconnected USB tether devices.
     * Does not return as long as there is no fatal error or a stop is requested.
     * Use startLooping() to not block the calling thread.
     */
    /**
     * 持续检查连接或断开的 USB 网络共享设备。
     * 只要没有发生致命错误或未请求停止，该方法就不会返回。
     * 使用 startLooping() 以避免阻塞调用线程。
     */
    void loopInfinite();
    /**
     * @brief simple state-based method that performs the following sequential
     * steps: 1) Wait until a tethering device is connected 2) Get the IP
     * -> if success, forward the IP address of the connected device.
     * 3) Wait until the device disconnects
     * 4) forward the now disconnected IP address.
     * Nr. 3) might never become true during run time as long as the user does not
     * disconnect his tethering device.
     */
    /**
     * @brief 基于状态的简单方法，执行以下顺序步骤：
     * 1) 等待网络共享设备连接
     * 2) 获取 IP 地址 -> 如果成功，转发连接设备的 IP 地址。
     * 3) 等待设备断开连接
     * 4) 转发已断开连接的 IP 地址。
     * 注意：只要用户未断开其网络共享设备，步骤 3) 可能永远不会为真。
     */
    void connectOnce();
};

#endif  // OPENHD_OPENHD_OHD_INTERFACE_INC_USBHOTSPOT_H_
