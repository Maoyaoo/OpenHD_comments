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

//  该代码定义了与网络相关的设置管理功能，包括 WiFi 热点模式和以太网操作模式的配置。通过 NetworkingSettingsHolder
//  类，实现了网络设置的持久化存储和加载，确保配置在程序重启后仍然有效。

#ifndef OPENHD_OPENHD_OHD_INTERFACE_INC_NETWORKING_SETTINGS_H_
#define OPENHD_OPENHD_OHD_INTERFACE_INC_NETWORKING_SETTINGS_H_

#include <cstdint>

#include "openhd_settings_directories.h"  // OpenHD 设置目录相关
#include "openhd_settings_persistent.h"   // OpenHD 持久化设置相关

// On by default, disabled when the FC is armed,
// re-enabled if the FC is disarmed.
// WiFi 热点模式常量定义
static constexpr auto WIFI_HOTSPOT_AUTO = 0;        // 热点模式为自动（默认开启，飞行器解锁时关闭）
static constexpr auto WIFI_HOTSPOT_ALWAYS_OFF = 1;  // 热点模式为始终关闭
static constexpr auto WIFI_HOTSPOT_ALWAYS_ON = 2;   // 热点模式为始终开启

// 以太网操作模式常量定义
// OpenHD does not touch the ethernet
static constexpr auto ETHERNET_OPERATING_MODE_UNTOUCHED = 0;  // OpenHD 不修改以太网配置
// OpenHD configures the ethernet, such that it acts as a 'hotspot'
// In hotspot mode, the IP of the ground station is always fixed and a unlimited
// amount of devices can connect to it.
static constexpr auto ETHERNET_OPERATING_MODE_HOTSPOT = 1;  // 以太网热点模式，地面单元 IP 固定，支持多个设备连接
// OpenHD does not touch the ethernet, but it starts forwarding data to whoever
// provides internet. a bit complicated :/
static constexpr auto ETHERNET_OPERATING_MODE_EXTERNAL_DEVICE = 2;  // OpenHD 不修改以太网，但启动数据转发到提供互联网的设备

// 网络相关设置结构体，与 wb_link 分离
// Networking related settings, separate from wb_link
struct NetworkingSettings {
    // Only used if a wifi hotspot card has been found
    int wifi_hotspot_mode = WIFI_HOTSPOT_AUTO;  // WiFi 热点模式，仅在检测到 WiFi 热点卡时使用
    // Ethernet operating mode (changes networking,might require reboot)
    int ethernet_operating_mode = ETHERNET_OPERATING_MODE_UNTOUCHED;  // 太网操作模式（可能会改变网络配置，可能需要重启）
};

// 判断 WiFi 热点模式是否有效的函数
static bool is_valid_wifi_hotspot_mode(int mode) {
    return mode == 0 || mode == 1 || mode == 2;
}

// 网络设置持久化管理类，继承自 PersistentSettings 模板类
class NetworkingSettingsHolder : public openhd::PersistentSettings<NetworkingSettings> {
   public:
    // 构造函数，初始化设置目录并调用 init 方法
    NetworkingSettingsHolder() : openhd::PersistentSettings<NetworkingSettings>(openhd::get_interface_settings_directory()) { init(); }

   private:
    [[nodiscard]] std::string get_unique_filename() const override { return "networking_settings.json"; }  // 获取唯一文件名，用于持久化存储
    [[nodiscard]] NetworkingSettings create_default() const override { return NetworkingSettings{}; }      // 创建默认设置
    std::optional<NetworkingSettings> impl_deserialize(const std::string& file_as_string) const override;  // 实现反序列化方法，从字符串解析设置
    std::string imp_serialize(const NetworkingSettings& data) const override;                              // 实现序列化方法，将设置转换为字符串
};

#endif  // OPENHD_OPENHD_OHD_INTERFACE_INC_NETWORKING_SETTINGS_H_
