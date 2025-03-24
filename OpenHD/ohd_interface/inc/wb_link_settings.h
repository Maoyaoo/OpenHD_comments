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

#ifndef OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_SETTINGS_HPP_
#define OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_SETTINGS_HPP_

#include <utility>

#include "openhd_platform.h"
#include "openhd_profile.h"
#include "openhd_settings_directories.h"
#include "openhd_settings_persistent.h"
#include "wifi_card.h"

// 该代码的主要作用是管理和配置无线链路（Wifi Broadband，WB）的设置，用于优化无线视频传输系统（如OpenHD）。具体功能包括：

// 定义默认设置：提供了无线链路的各种默认参数，如频率、MCS索引、频道宽度、发射功率、FEC设置等。
// 持久化管理：通过 WBLinkSettingsHolder 类实现了设置的持久化存储和加载，确保配置在系统重启后仍然有效。
// 动态调整：支持通过RC频道动态调整MCS索引和带宽，以及在不同状态（如武装状态）下调整发射功率。
// 开发者选项：提供了一些高级配置选项（如高重传计数、仅监听模式），供开发者调试和优化系统。
// 验证逻辑：对部分设置（如RTL8812AU发射功率索引）进行了有效性验证，确保参数在合理范围内。
// 该代码是OpenHD系统中无线链路配置的核心模块，确保无线链路在不同硬件和环境下都能稳定高效地工作。

namespace openhd {

static constexpr auto DEFAULT_5GHZ_FREQUENCY =
    5745;  // Channel 149 / OpenHD race band 2// 默认的5GHz频率：5745
           // MHz（频道149，OpenHD竞赛频段2）
static constexpr auto DEFAULT_2GHZ_FREQUENCY =
    2452;  // Channel 9 / is a 20Mhz channel / No openhd band in 2.4G//
           // 默认的2.4GHz频率：2452 MHz（频道9，20MHz带宽，不属于OpenHD频段）
// highest MCS where modulation is still QPSK
static constexpr auto DEFAULT_MCS_INDEX =
    2;  // 默认的MCS索引：2（QPSK调制下的最高MCS）
// We always use a MCS index of X for the uplink, since (compared to the video
// link) it requires a negligible amount of bandwidth and for those using RC
// over OpenHD, we have the benefit that the range of RC is "more" than the
// range for video
static constexpr auto WB_GND_UPLINK_MCS_INDEX =
    0;  // 上行链路（从地面站到空中单元）的MCS索引始终为0，因为它需要的带宽很少
static constexpr auto DEFAULT_CHANNEL_WIDTH = 20;  // 默认的WiFi频道宽度：20MHz
// Consti10: Stephen used a default tx power of 3100 somewhere (not sure if that
// ever made it trough though) This value seems a bit high to me, so I am going
// with a default of "1800" (which should be 18.0 dBm ) Used to be in dBm, but
// mW really is more verbose to the user - we convert from mW to dBm when using
// the iw dev set command
static constexpr auto DEFAULT_WIFI_TX_POWER_MILLI_WATT =
    25;  // 默认的WiFi发射功率：25毫瓦（约18.0 dBm）
// by default, we do not differentiate (to not confuse the user)
static constexpr auto WIFI_TX_POWER_MILLI_WATT_ARMED_DISABLED =
    0;  // 武装状态下禁用发射功率调整（默认值为0）
// tx power index 22 is about 25mW on asus, but on some card(s) that can be too
// much already (especially on custom HW). therefore, this default value is
// written at run time (see below)
static constexpr auto DEFAULT_RTL8812AU_TX_POWER_INDEX =
    0;  // 默认的RTL8812AU发射功率索引：0（在运行时动态设置）
// by default, we do not differentiate (to not confuse users)
static constexpr auto RTL8812AU_TX_POWER_INDEX_ARMED_DISABLED =
    0;  // 武装状态下禁用RTL8812AU发射功率索引调整（默认值为0）
// LDPC is enabled by default - drivers that don't support ldpc during rx do not
// exist anymore, and if the tx driver doesn't support it, it is just omitted.
static constexpr bool DEFAULT_ENABLE_LDPC =
    false;  // 默认启用LDPC（低密度奇偶校验编码）：false
// SHORT GUARD - doesn't really have that much of an benefit regarding bitrate,
// so we set it off by default (use long guard)
static constexpr bool DEFAULT_ENABLE_SHORT_GUARD =
    false;  // 默认禁用短保护间隔（使用长保护间隔）：false

// Set to 0 for fec auto block length
// Set to 1 or greater for fixed k fec
// Default to auto since 2.2.5-evo
// 自动FEC（前向纠错）块长度：0（默认值）
static constexpr auto WB_VIDEO_FEC_BLOCK_LENGTH_AUTO = 0;
static constexpr auto DEFAULT_WB_VIDEO_FEC_BLOCK_LENGTH =
    WB_VIDEO_FEC_BLOCK_LENGTH_AUTO;
// FEC can fixup packet loss, as long as is statistically well distributed (no
// big gaps) if there are many big gaps, increasing the FEC percentage often
// doesn't help, it is better to reduce the key frame interval of your camera in
// this case
// 默认的FEC百分比：20%
static constexpr auto DEFAULT_WB_VIDEO_FEC_PERCENTAGE = 20;
// -1 = use openhd recommended for this platform
// 默认的最大FEC块大小：-1（使用OpenHD推荐值）
static constexpr uint32_t DEFAULT_MAX_FEC_BLK_SIZE = -1;
// 0 means disabled (default), the rc channel used for setting the mcs index
// otherwise
// 禁用通过RC频道调整MCS索引：0
static constexpr auto WB_MCS_INDEX_VIA_RC_CHANNEL_OFF = 0;
// 禁用通过RC频道调整带宽：0
static constexpr auto WB_BW_VIA_RC_CHANNEL_OFF = 0;

// 定义无线链路的设置结构体
struct WBLinkSettings {
  // 无线频率（2.4GHz或5GHz）
  uint32_t wb_frequency;  // writen once 2.4 or 5 is known
  // NOTE: Only stored on air, gnd automatically applies 40Mhz bwidth when air
  // reports (management frame(s))
  // 空中单元的频道宽度（默认20MHz或40MHz）
  uint32_t wb_air_tx_channel_width =
      DEFAULT_CHANNEL_WIDTH;  // 20 or 40 mhz bandwidth
  // MCS index used during injection - only used by air unit, since ground
  // always sends with MCS0
  // 空中单元的MCS索引（默认2）
  uint32_t wb_air_mcs_index = DEFAULT_MCS_INDEX;
  int wb_enable_stbc = 0;  // 0==disabled// 禁用STBC（空时块编码）
  bool wb_enable_ldpc = DEFAULT_ENABLE_LDPC;  // 默认禁用LDPC
  bool wb_enable_short_guard =
      DEFAULT_ENABLE_SHORT_GUARD;  // 默认禁用短保护间隔
  uint32_t wb_tx_power_milli_watt =
      DEFAULT_WIFI_TX_POWER_MILLI_WATT;  // 发射功率
  uint32_t wb_tx_power_milli_watt_armed =
      WIFI_TX_POWER_MILLI_WATT_ARMED_DISABLED;  // 武装状态下的发射功率
  // rtl8812au driver does not support setting tx power by iw dev, but rather
  // only by setting a tx power index override param. With the most recent
  // openhd rtl8812au driver, we can even change this parameter dynamically. See
  // https://github.com/OpenHD/rtl8812au/blob/v5.2.20/os_dep/linux/ioctl_cfg80211.c#L3667
  // These values are the values that are passed to
  // NL80211_ATTR_WIPHY_TX_POWER_LEVEL this param is normally in mBm, but has
  // been reworked to accept those rtl8812au specific tx power index override
  // values (under this name they were known already in previous openhd
  // releases, but we now support changing them dynamcially at run time)
  uint32_t wb_rtl8812au_tx_pwr_idx_override =
      DEFAULT_RTL8812AU_TX_POWER_INDEX;  // RTL8812AU发射功率索引
  // applied when armed
  uint32_t wb_rtl8812au_tx_pwr_idx_override_armed =
      RTL8812AU_TX_POWER_INDEX_ARMED_DISABLED;  // 武装状态下的RTL8812AU发射功率索引
  uint32_t wb_video_fec_percentage =
      DEFAULT_WB_VIDEO_FEC_PERCENTAGE;  // FEC百分比
  // decrease this value when there is a lot of pollution on your channel, and
  // you consistently get tx errors even though variable bitrate is working
  // fine. If you set this value to 80% (for example), it reduces the bitrate(s)
  // recommended to the encoder by 80% for each mcs index
  int wb_video_rate_for_mcs_adjustment_percent =
      100;  // MCS索引调整的比特率百分比
  // NOTE: -1 means use whatever is the openhd recommendation for this platform
  int wb_max_fec_block_size = DEFAULT_MAX_FEC_BLK_SIZE;  // 最大FEC块大小
  // change mcs index via RC channel
  uint32_t wb_mcs_index_via_rc_channel =
      WB_MCS_INDEX_VIA_RC_CHANNEL_OFF;  // 通过RC频道调整MCS索引
  // change bw via RC channel
  int wb_bw_via_rc_channel = WB_BW_VIA_RC_CHANNEL_OFF;  // 通过RC频道调整带宽
  // wb link recommends bitrate(s) to the encoder.
  bool enable_wb_video_variable_bitrate = true;  // 启用可变比特率
  int wb_qp_max = 17;                            // 最大量化参数
  int wb_qp_min = 42;                            // 最小量化参数
  // !!!!
  // This allows the ground station to become completely passive (aka tune in on
  // someone elses feed) but obviosuly you cannot reach your air unit anymore
  // when this mode is enabled (disable it to re-gain control)
  bool wb_enable_listen_only_mode = false;  // 启用仅监听模式
  // NOTE: Really complicated, for developers only
  bool wb_dev_air_set_high_retransmit_count =
      false;  // 开发者选项：设置高重传计数
};

// 创建默认的无线链路设置
WBLinkSettings create_default_wb_stream_settings(
    const std::vector<WiFiCard>& wifibroadcast_cards);

// 验证RTL8812AU发射功率索引是否有效
static bool validate_wb_rtl8812au_tx_pwr_idx_override(int value) {
  if (value >= 0 && value <= 63) return true;  // 有效范围为0到63
  openhd::log::get_default()->warn(
      "Invalid wb_rtl8812au_tx_pwr_idx_override {}", value);
  return false;
}

// 无线链路设置的持久化管理类
class WBLinkSettingsHolder : public openhd::PersistentSettings<WBLinkSettings> {
 public:
  /**
   * @param platform needed to figure out the proper default params
   * @param wifibroadcast_cards1 needed to figure out the proper default params
   */
  // 构造函数，初始化默认设置
  explicit WBLinkSettingsHolder(OHDProfile profile,
                                std::vector<WiFiCard> wifibroadcast_cards1)
      : openhd::PersistentSettings<WBLinkSettings>(
            get_interface_settings_directory()),
        m_cards(std::move(wifibroadcast_cards1)),
        m_profile(std::move(profile)) {
    init();
  }

 public:
  const OHDProfile m_profile;           // OpenHD平台配置
  const std::vector<WiFiCard> m_cards;  // WiFi卡列表

 private:
  // 获取唯一的文件名
  [[nodiscard]] std::string get_unique_filename() const override {
    std::stringstream ss;
    ss << "wifibroadcast_settings.json";
    return ss.str();
  }
  // 创建默认设置
  [[nodiscard]] WBLinkSettings create_default() const override {
    return create_default_wb_stream_settings(m_cards);
  }

 private:
  // 反序列化设置
  std::optional<WBLinkSettings> impl_deserialize(
      const std::string& file_as_string) const override;
  // 序列化设置
  std::string imp_serialize(const WBLinkSettings& data) const override;
};

// 定义设置项的键值（最大16字符限制）
static constexpr auto WB_FREQUENCY = "WB_FREQUENCY";      // 无线频率
static constexpr auto WB_CHANNEL_WIDTH = "WB_CHANNEL_W";  // 频道宽度
static constexpr auto WB_MCS_INDEX = "WB_MCS_INDEX";      // MCS索引
static constexpr auto WB_VIDEO_FEC_BLOCK_LENGTH =
    "WB_V_FEC_BLK_L";                                             // FEC块长度
static constexpr auto WB_VIDEO_FEC_PERCENTAGE = "WB_V_FEC_PERC";  // FEC百分比
static constexpr auto WB_VIDEO_RATE_FOR_MCS_ADJUSTMENT_PERC =
    "WB_V_RATE_PERC";  // MCS调整的比特率百分比
static constexpr auto WB_MAX_FEC_BLOCK_SIZE_FOR_PLATFORM =
    "WB_MAX_D_BZ";                                             // 最大FEC块大小
static constexpr auto WB_TX_POWER_MILLI_WATT = "TX_POWER_MW";  // 发射功率
static constexpr auto WB_TX_POWER_MILLI_WATT_ARMED =
    "TX_POWER_MW_ARM";  // 武装状态下的发射功率
static constexpr auto WB_RTL8812AU_TX_PWR_IDX_OVERRIDE =
    "TX_POWER_I";  // RTL8812AU发射功率索引
static constexpr auto WB_RTL8812AU_TX_PWR_IDX_ARMED =
    "TX_POWER_I_ARMED";  // 武装状态下的RTL8812AU发射功率索引
static constexpr auto WB_VIDEO_VARIABLE_BITRATE =
    "VARIABLE_BITRATE";                              // 可变比特率
static constexpr auto WB_QP_MAX = "QP_MAX";          // 最大量化参数
static constexpr auto WB_QP_MIN = "QP_MIN";          // 最小量化参数
static constexpr auto WB_ENABLE_STBC = "WB_E_STBC";  // 启用STBC
static constexpr auto WB_ENABLE_LDPC = "WB_E_LDPC";  // 启用LDPC
static constexpr auto WB_ENABLE_SHORT_GUARD =
    "WB_E_SHORT_GUARD";  // 启用短保护间隔
static constexpr auto WB_MCS_INDEX_VIA_RC_CHANNEL =
    "MCS_VIA_RC";  // 通过RC频道调整MCS索引
static constexpr auto WB_BW_VIA_RC_CHANNEL = "BW_VIA_RC";  // 通过RC频道调整带宽
static constexpr auto WB_PASSIVE_MODE = "WB_PASSIVE_MODE";  // 启用仅监听模式
static constexpr auto WB_DEV_AIR_SET_HIGH_RETRANSMIT_COUNT =
    "DEV_HIGH_RETR";  // 开发者选项：设置高重传计数

}  // namespace openhd

#endif  // OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_SETTINGS_HPP_
