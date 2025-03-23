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

#ifndef OPENHD_OPENHD_INTERFACE_H
#define OPENHD_OPENHD_INTERFACE_H

#include <memory>
#include <utility>

#include "ethernet_manager.h"
#include "networking_settings.h"
#include "openhd_action_handler.h"
#include "openhd_external_device.h"
#include "openhd_led.h"
#include "openhd_link.hpp"
#include "openhd_platform.h"
#include "openhd_profile.h"
#include "openhd_settings_imp.h"
#include "openhd_spdlog.h"
#include "usb_tether_listener.h"
#include "wifi_hotspot.h"

class WBLink;
class MicrohardLink;
class EthernetLink;
/**
 * Takes care of everything networking related, like wifibroadcast, usb /
 * tethering / WiFi-hotspot usw. In openhd, there is an instance of this class
 * on both air and ground with partially similar, partially different
 * functionalities.
 */
/**
 * 负责处理所有与网络相关的事务，例如 wifibroadcast、USB / 网络共享 / WiFi
 * 热点等。 在 OpenHD中，空中单元和地面单元都有一个此类的实例，功能部分相似，部分不同。
 */
class OHDInterface {
   public:
    /**
     * @param platform platform we are running on
     * @param profile air or ground
     * @param opt_action_handler r.n used to propagate rate control from wb_link
     * to ohd_video
     */
    explicit OHDInterface(OHDProfile profile);
    OHDInterface(const OHDInterface&) = delete;
    OHDInterface(const OHDInterface&&) = delete;
    ~OHDInterface();
    // Get all (mavlink) settings ohd_interface exposes on the air or ground unit,
    // respective
    std::vector<openhd::Setting> get_all_settings();
    // easy access without polluting the headers
    static void print_internal_fec_optimization_method();
    /**
     * If a password.txt file exists, generate the key(s) from it, store them, and
     * then delete the password.txt file. Does nothing if no password.txt file
     * exists.
     */
    /**
     * 如果存在password.txt文件，从中生成密钥并存储，
     * 然后删除password.txt文件。如果没有password.txt文件，则不执行任何操作。
     */
    static void generate_keys_from_pw_if_exists_and_delete();
    // Agnostic of the link, even though r.n we only have a wifibroadcast
    // implementation (but this might change).
    // 与链接无关，尽管目前我们只有wifibroadcast实现
    // （但这可能会有所改变）。
    std::shared_ptr<OHDLink> get_link_handle();

   private:
    void update_wifi_hotspot_enable();

   private:
    const OHDProfile m_profile;
    std::shared_ptr<spdlog::logger> m_console;
    std::shared_ptr<WBLink> m_wb_link;
    std::shared_ptr<MicrohardLink> m_microhard_link;
    std::unique_ptr<USBTetherListener> m_usb_tether_listener;
    std::unique_ptr<EthernetManager> m_ethernet_manager;
    std::unique_ptr<WifiHotspot> m_wifi_hotspot;
    std::shared_ptr<EthernetLink> m_ethernet_link;
    std::vector<WiFiCard> m_monitor_mode_cards{};
    std::optional<WiFiCard> m_opt_hotspot_card = std::nullopt;
    NetworkingSettingsHolder m_nw_settings;
};

#endif  // OPENHD_OPENHD_INTERFACE_H
