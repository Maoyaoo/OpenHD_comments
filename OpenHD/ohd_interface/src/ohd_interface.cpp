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

#include "ohd_interface.h"

#include <wifi_card_discovery.h>
#include <wifi_client.h>

#include <utility>

#include "config_paths.h"
#include "ethernet_link.h"
#include "microhard_link.h"
#include "openhd_config.h"
#include "openhd_global_constants.hpp"
#include "openhd_util_filesystem.h"
#include "wb_link.h"
// Helper function to execute a shell command and return the output
std::string exec(const std::string& cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

// Helper function to check if a Microhard device is present
bool is_microhard_device_present() {
  if (!OHDFilesystemUtil::exists(std::string(getConfigBasePath()) +
                                 "wfb.txt") &&
      !OHDFilesystemUtil::exists(std::string(getConfigBasePath()) +
                                 "ethernet.txt")) {
    std::string output = exec("lsusb");
    return output.find("Microhard") != std::string::npos;
  }

  return false;
}

OHDInterface::OHDInterface(OHDProfile profile1)
    : m_profile(std::move(profile1)) {
      // 1. 创建日志对象
  m_console = openhd::log::create_or_get("interface");
  assert(m_console);
  // 2. 初始化成员变量
  m_monitor_mode_cards = {};
  m_opt_hotspot_card = std::nullopt;
  // 3. 加载配置并检查 Microhard 设备
  const auto config = openhd::load_config();
  bool microhard_device_present = is_microhard_device_present();

  // 4. 检查以太网连接
  if (OHDFilesystemUtil::exists(std::string(getConfigBasePath()) +
                                "ethernet.txt")) {
    m_ethernet_link = std::make_shared<EthernetLink>(m_profile);
    m_console->warn("eth found");
    return;
  }

  // 5. 检查 Microhard 设备
  if (microhard_device_present) {
    m_microhard_link = std::make_shared<MicrohardLink>(m_profile);
    m_console->warn("mc found");
    return;
  }

  // 6. 发现并处理 Wi-Fi 网卡
  DWifiCards::main_discover_an_process_wifi_cards(
      config, m_profile, m_console, m_monitor_mode_cards, m_opt_hotspot_card);
  // 7. 输出 Wi-Fi 网卡信息
  m_console->debug("monitor_mode card(s):{}",
                   debug_cards(m_monitor_mode_cards));
  if (m_opt_hotspot_card.has_value()) {
    m_console->debug("Hotspot card:{}", m_opt_hotspot_card.value().device_name);
  } else {
    m_console->debug("No WiFi hotspot card");
  }
  // We don't have at least one card for monitor mode, which means we cannot
  // instantiate wb_link (no wifibroadcast connectivity at all)
  // 8. 检查是否有监控模式网卡
  if (m_monitor_mode_cards.empty()) {
    // 如果没有支持监控模式的 Wi-Fi 网卡，输出警告日志并设置错误状态。
    // 提示用户重启设备。
    m_console->warn(
        "Cannot start ohd_interface, no wifi card for monitor mode");
    const std::string message_for_user = "No WiFi card found, please reboot";
    m_console->warn(message_for_user);
    openhd::LEDManager::instance().set_status_error();
    // TODO reason what to do. We do not support dynamically adding wifi cards
    // at run time, so somehow we need to signal to the user that something is
    // completely wrong. However, as an Ground pi, we can still run QOpenHD and
    // OpenHD, just it will never connect to an Air PI
  } else {
    // 9. 初始化 Wi-Fi 广播链路
    // Set the card(s) we have into monitor mode
    // 如果有支持监控模式的 Wi-Fi 网卡，将其设置为监控模式。
    openhd::wb::takeover_cards_monitor_mode(m_monitor_mode_cards, m_console);
    // 创建一个 WBLink 对象，用于管理 Wi-Fi 广播链路。
    m_wb_link = std::make_shared<WBLink>(m_profile, m_monitor_mode_cards);
  }
  // The USB tethering listener is always enabled on ground - it doesn't
  // interfere with anything
  // 11. 初始化以太网管理器（仅地面设备）
  if (m_profile.is_ground()) {
    // The USB tethering listener is always enabled on ground - it doesn't
    // interfere with anything
    // 如果设备是地面设备，创建一个 EthernetManager 对象，并异步初始化以太网连接。
    m_usb_tether_listener = std::make_unique<USBTetherListener>();
  }
  // Ethernet - optional, only on ground
  if (m_profile.is_ground()) {
    m_ethernet_manager = std::make_unique<EthernetManager>();
    m_ethernet_manager->async_initialize(
        m_nw_settings.get_settings().ethernet_operating_mode);
    // m_nw_settings.get_settings().ethernet_operating_mode
  }
  // Wi-Fi hotspot functionality if possible.
  // 12. 初始化 Wi-Fi 热点功能
  if (m_opt_hotspot_card.has_value()) {
    if (WiFiClient::create_if_enabled()) {
      // Wifi client active
    } else {
      // 如果 Wi-Fi 客户端未启用，创建一个 WifiHotspot 对象，并根据 Wi-Fi 广播链路的频率空间设置热点频率。
      const openhd::WifiSpace wb_frequency_space =
          (m_wb_link != nullptr)
              ? m_wb_link->get_current_frequency_channel_space()
              : openhd::WifiSpace::G5_8;
      // OHD hotspot needs to know the wifibroadcast frequency - it is always on
      // the opposite spectrum
      m_wifi_hotspot = std::make_unique<WifiHotspot>(
          m_profile, m_opt_hotspot_card.value(), wb_frequency_space);
      update_wifi_hotspot_enable();
    }
  }
  // automatically disable Wi-Fi hotspot if FC is armed
  // 自动禁用 Wi-Fi 热点（当飞控解锁时）
  if (m_wifi_hotspot) {
    auto cb = [this](bool armed) { update_wifi_hotspot_enable(); };
    openhd::ArmingStateHelper::instance().register_listener("ohd_interface_wfi",cb);
  }
  m_console->debug("OHDInterface::created");
}

OHDInterface::~OHDInterface() {
  // Terminate the link first
  m_wb_link = nullptr;
  // Then give the card(s) back to the system (no monitor mode)
  // give the monitor mode cards back to network manager
  openhd::wb::giveback_cards_monitor_mode(m_monitor_mode_cards, m_console);
  if (m_ethernet_manager) {
    m_ethernet_manager->stop();
    m_ethernet_manager = nullptr;
  }
}

std::vector<openhd::Setting> OHDInterface::get_all_settings() {
  std::vector<openhd::Setting> ret;
  m_console->warn("get all settings");
  if (m_wb_link) {
    auto settings = m_wb_link->get_all_settings();
    OHDUtil::vec_append(ret, settings);
  }
  if (m_microhard_link) {
    auto settings = m_microhard_link->get_all_settings();
    OHDUtil::vec_append(ret, settings);
  }
  if (m_wifi_hotspot != nullptr) {
    auto cb_wifi_hotspot_mode = [this](std::string, int value) {
      if (!is_valid_wifi_hotspot_mode(value)) return false;
      m_nw_settings.unsafe_get_settings().wifi_hotspot_mode = value;
      m_nw_settings.persist();
      update_wifi_hotspot_enable();
      return true;
    };
    ret.push_back(openhd::Setting{
        "WIFI_HOTSPOT_E",
        openhd::IntSetting{m_nw_settings.get_settings().wifi_hotspot_mode,
                           cb_wifi_hotspot_mode}});
  }
  if (m_profile.is_ground()) {
    const auto settings = m_nw_settings.get_settings();
    auto cb_ethernet = [this](std::string, int value) {
      m_nw_settings.unsafe_get_settings().ethernet_operating_mode = value;
      m_nw_settings.persist();
      // Change requires reboot
      return true;
    };
    ret.push_back(openhd::Setting{
        "ETHERNET",
        openhd::IntSetting{settings.ethernet_operating_mode, cb_ethernet}});
  }
  /* if(m_opt_hotspot_card){
     auto setting=openhd::create_read_only_string(fmt::format("HOTSPOT_CARD"),
   wifi_card_type_to_string(m_opt_hotspot_card.value().type));
     ret.emplace_back(setting);
   }*/
  openhd::validate_provided_ids(ret);
  return ret;
}

void OHDInterface::print_internal_fec_optimization_method() {
  fec_stream_print_fec_optimization_method();
}

std::shared_ptr<OHDLink> OHDInterface::get_link_handle() {
  if (m_ethernet_link) {
    m_console->warn("Using Link: Ethernet");
    return m_ethernet_link;
  }
  if (m_wb_link) {
    m_console->warn("Using Link: OpenHD-WifiBroadCast");
    return m_wb_link;
  }
  if (m_microhard_link) {
    m_console->warn("Using Link: Microhard");
    return m_microhard_link;
  }
  return nullptr;
}

void OHDInterface::generate_keys_from_pw_if_exists_and_delete() {
  // Make sure this stupid sodium init has been called
  // 确保已经调用了那个烦人的sodium初始化函数。
  if (sodium_init() == -1) {
    std::cerr << "Cannot init libsodium" << std::endl;
    exit(EXIT_FAILURE);
  }
  auto console = openhd::log::get_default();

  if (OHDFilesystemUtil::exists(std::string(getConfigBasePath()) +
                                "password.txt")) {
    auto pw = OHDFilesystemUtil::read_file(std::string(getConfigBasePath()) +
                                           "password.txt");
    OHDUtil::trim(pw);
    console->info("Generating key(s) from pw [{}]",
                  OHDUtil::password_as_hidden_str(pw));  // don't show the pw
    //生成秘钥对
    auto keys = wb::generate_keypair_from_bind_phrase(pw);
    //写入秘钥
    if (wb::write_keypair_to_file(keys, openhd::SECURITY_KEYPAIR_FILENAME)) {
      console->debug("Keypair file successfully written");
      // delete the file
      OHDFilesystemUtil::remove_if_existing(std::string(getConfigBasePath()) +
                                            "password.txt");
      OHDFilesystemUtil::make_file_read_write_everyone(
          openhd::SECURITY_KEYPAIR_FILENAME);
    } else {
      console->error("Cannot write keypair file !");
      OHDFilesystemUtil::remove_if_existing(openhd::SECURITY_KEYPAIR_FILENAME);
    }
  }

  // If no keypair file exists (It was not created from the password.txt file)
  // we create the txrx.key once (from the default password) such that the boot
  // up time is sped up on successive boot(s)
  // 如果密钥对文件不存在（即没有从 password.txt 文件创建该文件），
  // 我们会（根据默认密码）一次性创建 txrx.key
  // 文件，以便在后续启动时加快启动速度。
  auto val = wb::read_keypair_from_file(openhd::SECURITY_KEYPAIR_FILENAME);
  if ((!OHDFilesystemUtil::exists(openhd::SECURITY_KEYPAIR_FILENAME)) ||
      (!val)) {
    console->debug("Creating txrx.key from default pw (once)");
    auto keys = wb::generate_keypair_from_bind_phrase(wb::DEFAULT_BIND_PHRASE);
    wb::write_keypair_to_file(keys, openhd::SECURITY_KEYPAIR_FILENAME);
  }
}

// 用于更新 WiFi 热点的启用状态。
void OHDInterface::update_wifi_hotspot_enable() {
  assert(m_wifi_hotspot);
  // 获取设置并初始化变量
  const auto& settings = m_nw_settings.get_settings();
  bool enable_wifi_hotspot = false;

  // 根据不同的热点模式设置启用状态
  if (settings.wifi_hotspot_mode == WIFI_HOTSPOT_AUTO) {
    bool is_armed = openhd::ArmingStateHelper::instance().is_currently_armed();
    enable_wifi_hotspot = !is_armed;
  } else if (settings.wifi_hotspot_mode == WIFI_HOTSPOT_ALWAYS_OFF) {
    enable_wifi_hotspot = false;
  } else if (settings.wifi_hotspot_mode == WIFI_HOTSPOT_ALWAYS_ON) {
    enable_wifi_hotspot = true;
  } else {
    m_console->warn("Invalid wifi hotspot mode");
    enable_wifi_hotspot = false;
  }

  // 更新 WiFi 热点状态并记录信息
  m_wifi_hotspot->set_enabled_async(
      enable_wifi_hotspot);  // 以异步方式设置 WiFi 热点的启用状态
  openhd::LinkActionHandler::instance().m_wifi_hotspot_state =
      enable_wifi_hotspot ? 2 : 1;  // 更新 LinkActionHandler 实例中的 WiFi
                                    // 热点状态，启用时为 2，禁用时为 1
  openhd::LinkActionHandler::instance().m_wifi_hotspot_frequency =
      m_wifi_hotspot
          ->get_frequency();  // 更新 LinkActionHandler 实例中的 WiFi 热点频率
}
