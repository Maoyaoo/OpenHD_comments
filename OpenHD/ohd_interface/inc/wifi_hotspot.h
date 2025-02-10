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

#ifndef OPENHD_OPENHD_OHD_INTERFACE_SRC_WIFIHOTSPOT_H_
#define OPENHD_OPENHD_OHD_INTERFACE_SRC_WIFIHOTSPOT_H_

#include <openhd_profile.h>

#include <future>
#include <string>
#include <vector>

#include "openhd_settings_imp.h"
#include "wifi_card.h"

/**
 * Wifi hotspot refers to creating a WiFi Access point on the device we are
 * running on. External clients like QOpenHD running on a tablet can then
 * connect to the hotspot. Note that auto video and telemetry forwarding is not
 * implemented for WiFi hotspot - On the one hand, this is prone to errors
 * anyways, on the other hand, it is hard to do to actively search for connected
 * devices and their IPs. TCP mavlink (perhaps also video in the future) is the
 * way to go here. Change Nov4 2022: Uses network manager - we already have
 * network manager installed and enabled by default on the rpi on the openhd
 * images, but the default raspbian images from pi foundation have it only
 * installed, but disabled by default (they'l use it eventually)
 */
/*WiFi 热点是指在我们运行的设备上创建一个 WiFi 接入点。外部客户端，如在平板上运行的 QOpenHD，
可以连接到该热点。请注意，WiFi 热点并未实现自动的视频和遥测转发——一方面，这种方法容易出错，
另一方面，主动搜索连接设备及其 IP 是很难实现的。TCP mavlink（未来可能还包括视频）是这里的最佳选择。
2022年11月4日更新：使用网络管理器——我们已经在 OpenHD 镜像中默认安装并启用了网络管理器，
而树莓派基金会的默认 Raspbian 镜像仅安装了它，但默认是禁用的（他们最终会启用它）。 */
class WifiHotspot {
 public:
  /**
   * Utility for starting, stopping WIFI AP (Hotspot) and forwarding the client
   * connect/disconnect events.
   */
  explicit WifiHotspot(OHDProfile profile, WiFiCard wifiCard,
                       const openhd::WifiSpace& wifibroadcast_frequency_space);
  WifiHotspot(const WifiHotspot&) = delete;
  WifiHotspot(const WifiHotspot&&) = delete;
  ~WifiHotspot();
  // Use opposite frequency band to wfb if possible
  static bool get_use_5g_channel(
      const WiFiCard& wifiCard,
      const openhd::WifiSpace& wifibroadcast_frequency_space);
  //
  void set_enabled_async(bool enable);
  uint16_t get_frequency();
  static bool util_delete_nm_file();

 private:
  // NOTE: might block, use async
  // just runs the appropriate network manager (nmcli) command to start an
  // already created wifi hotspot connection
  void start();
  // NOTE: might block,use async
  // just runs the appropriate network manager (nmcli) command to stop an
  // already created wifi hotspot connection
  void stop();
  void start_async();
  void stop_async();
  const OHDProfile m_profile;
  const WiFiCard m_wifi_card;
  bool started = false;
  std::shared_ptr<spdlog::logger> m_console;
  bool m_use_5G_channel;
  bool m_is_enabled = false;
};

#endif  // OPENHD_OPENHD_OHD_INTERFACE_SRC_WIFIHOTSPOT_H_
