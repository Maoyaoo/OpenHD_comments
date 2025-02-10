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

#ifndef STREAMS_H
#define STREAMS_H

#include <array>
#include <chrono>
#include <optional>
#include <utility>
#include <vector>

#include "../lib/wifibroadcast/wifibroadcast/src/WBStreamRx.h"
#include "../lib/wifibroadcast/wifibroadcast/src/WBStreamTx.h"
#include "../lib/wifibroadcast/wifibroadcast/src/WBTxRx.h"
#include "../lib/wifibroadcast/wifibroadcast/src/encryption/EncryptionFsUtils.h"
#include "openhd_action_handler.h"
#include "openhd_link.hpp"
#include "openhd_link_statistics.hpp"
#include "openhd_platform.h"
#include "openhd_profile.h"
#include "openhd_settings_imp.h"
#include "openhd_spdlog.h"
#include "openhd_util_time.h"
#include "wb_link_helper.h"
#include "wb_link_manager.h"
#include "wb_link_settings.h"
#include "wb_link_work_item.hpp"
#include "wifi_card.h"

/**
 * This class takes a list of cards supporting monitor mode (only 1 card on air)
 * and is responsible for configuring the given cards and then setting up all
 * the Wifi-broadcast streams needed for OpenHD. In the end, we have a link that
 * has some broadcast characteristics for video (video is always broadcast from
 * air to ground) but also a bidirectional link (without re-transmission(s)) for
 * telemetry. This class assumes a corresponding instance on the air or ground
 * unit, respective.
 */
/**
 * 这个类接收一个支持监控模式的网卡列表（只有1张网卡在空中工作），
 * 负责配置给定的网卡，然后设置所有OpenHD所需的WiFi广播流。
 * 最终，我们将拥有一个链接，该链接具备一些视频广播特性（视频始终从空中广播到地面），
 * 但也具有一个双向链接（没有重传）用于遥测。
 * 这个类假设空中或地面单元有一个相应的实例。
 */
class WBLink : public OHDLink {
 public:
  /**
   * @param broadcast_cards list of discovered wifi card(s) that support monitor
   * mode & are injection capable. Needs to be at least one card, and only one
   * card on an air unit. The given cards need to support monitor mode and
   * either 2.4G or 5G wifi. In the case where there are multiple card(s), the
   * first given card is used for transmission & receive, the other card(s) are
   * not used for transmission, only for receiving.
   * @param opt_action_handler global openhd action handler, optional (can be
   * nullptr during testing of specific modules instead of testing a complete
   * running openhd instance)
   */
  /**
   * @param broadcast_cards 支持监控模式并具有注入能力的发现的WiFi网卡列表。
   * 需要至少一张网卡，并且空中单元上只能有一张网卡。所给的网卡需要支持监控模式，
   * 并且支持2.4G或5G WiFi。如果有多个网卡，第一张网卡用于发送和接收，其他网卡
   * 不用于发送，只用于接收。
   * @param opt_action_handler 全局的OpenHD动作处理器，
   * 可选的（在测试特定模块时可以为nullptr，
   * 而不是测试一个完整运行的OpenHD实例）
   */
  explicit WBLink(OHDProfile profile, std::vector<WiFiCard> broadcast_cards);
  WBLink(const WBLink&) = delete;
  WBLink(const WBLink&&) = delete;
  ~WBLink();
  /**
   * @return all mavlink settings, values might change depending on air/ground
   * and/or the used hardware
   * 所有的Mavlink设置，值可能会根据空中/地面和/或使用的硬件发生变化
   */
  std::vector<openhd::Setting> get_all_settings();
  /**
   * Used by wifi hotspot feature (opposite wifi space if possible)
   * @return the current wb channel space
   * 用于WiFi热点功能（如果可能，使用相对的WiFi频段）
   * @return 当前的WB频道频段
   */
  [[nodiscard]] openhd::WifiSpace get_current_frequency_channel_space() const;

 private:
  // NOTE:
  // For everything prefixed with 'request_', we validate the param (since it
  // comes from mavlink and might be unsafe to apply) And return false if it is
  // an invalid param (e.g. an unsupported frequency by the card). We then
  // return true if we can enqueue this change operation to be applied on the
  // worker thread (false otherwise). This way we have the nice feature that we
  // 1) reject settings while the worker thread is busy (e.g. during a channel
  // scan) or if a previous change (like tx power) is still being performed. In
  // this case, the user can just try again later (and should not be able to
  // change the frequency for example during a channel scan anyway). 2) can send
  // the mavlink ack immediately, instead of needing to wait for the action to
  // be performed (Changing the tx power for example can take some time, while
  // the OS is busy talking to the wifi driver). Only disadvantage: We need to
  // be able to reason about weather the given change will be successfully or
  // not beforehand.
  // 注意：
  // 对于所有以 'request_'
  // 为前缀的内容，我们会验证参数（因为它来自mavlink，可能不安全）
  // 如果参数无效（例如网卡不支持的频率），则返回false。然后如果我们可以将该更改操作
  // 排队并应用到工作线程中，则返回true（否则返回false）。这样，我们就能实现以下特性：
  // 1)
  // 在工作线程忙碌时（例如正在进行频道扫描时）或前一个更改（如发射功率）仍在执行时，
  // 我们会拒绝设置。此时，用户只需稍后再试（例如，用户在频道扫描过程中本来就不应该能
  // 改变频率）。
  // 2)
  // 可以立即发送mavlink确认，而不需要等待操作执行（例如，改变发射功率可能需要一些时间，
  // 而操作系统正在忙于与WiFi驱动通信）。唯一的缺点是：我们需要事先能够推测出给定的更改
  // 是否能够成功。
  bool request_set_frequency(int frequency);

  // Channel width / bandwidth is local to the air, and can be changed without
  // synchronization due to 20Mhz management packets
  // 信道宽度/带宽是空中单元本地的，由于 20MHz
  // 管理数据包的存在，可以在无需同步的情况下更改。
  bool request_set_air_tx_channel_width(int channel_width);

  // TX power can be set for both air / ground independently.
  // 发射功率可以独立设置空中单元和地面单元。
  bool request_set_tx_power_mw(int new_tx_power_mw, bool armed);
  bool request_set_tx_power_rtl8812au(int tx_power_index_override, bool armed);

  // MCS index can be changed on air (user can control the rate with it).
  // MCS 索引可以在空中单元上更改（用户可以通过它控制速率）。
  bool request_set_air_mcs_index(int mcs_index);

  // These do not "break" the bidirectional connectivity and therefore
  // can be changed easily on the fly
  // 这些不会“中断”双向连接，因此可以轻松地动态更改
  bool set_air_video_fec_percentage(int fec_percentage);
  bool set_air_enable_wb_video_variable_bitrate(int value);
  bool set_air_max_fec_block_size_for_platform(int value);
  bool set_air_wb_video_rate_for_mcs_adjustment_percent(int value);
  bool set_dev_air_set_high_retransmit_count(int value);
  // Initiate channel scan / channel analyze.
  // Those operations run asynchronous until completed, and during this time
  // all other "request_" setting changes are rejected (since the work thread
  // does the long-running async operation)
  // 启动信道扫描 / 信道分析。
  // 这些操作是异步执行的，直到完成。在此期间，所有其他的 "request_"
  // 设置更改会被拒绝（ 因为工作线程正在执行长时间运行的异步操作）
  bool request_start_scan_channels(
      openhd::LinkActionHandler::ScanChannelsParam scan_channels_params);
  bool request_start_analyze_channels(int channels_to_scan);

  // apply the frequency (wifi channel) and channel with for all wifibroadcast
  // cards r.n uses both iw and modifies the radiotap header
  // 应用频率（WiFi信道）和信道宽度到所有的wifibroadcast卡。
  // r.n同时使用iw并修改radiotap头
  bool apply_frequency_and_channel_width(int frequency, int channel_width_rx,
                                         int channel_width_tx);
  bool apply_frequency_and_channel_width_from_settings();

  // set the tx power of all wb cards. For rtl8812au, uses the tx power index
  // for other cards, uses the mW value
  // 设置所有wb卡的发射功率。对于rtl8812au，使用发射功率索引；
  // 对于其他卡，使用mW值。
  void apply_txpower();
  /**
   * Every time the arming state is updated, we just set a flag here such that
   * the main thread updates the tx power
   */
  /**
   * 每次更新解锁状态时，我们只需在此设置一个标志，以便主线程更新发射功率。
   */
  void update_arming_state(bool armed);

  // Recalculate stats, apply settings asynchronously and more
  // 重新计算统计信息、异步应用设置及其他操作
  // 定期更新统计信息，更新后的数据通过动作处理器传递给
  // ohd_telemetry 模块
  void loop_do_work();

  // update statistics, done in regular intervals, updated data is given to the
  // ohd_telemetry module via the action handler
  // 更新统计信息，按常规间隔进行，更新的数据通过动作处理器传递给ohd_telemetry模块
  void wt_update_statistics();

  // Do rate adjustments, does nothing if variable bitrate is disabled
  // 执行速率调整，如果禁用可变比特率，则不执行任何操作
  void wt_perform_rate_adjustment();
  void wt_gnd_perform_channel_management();

  // this is special, mcs index can not only be changed via mavlink param, but
  // also via RC channel (if enabled)
  // 这是特别的，MCS索引不仅可以通过mavlink参数更改，还可以通过RC通道更改（如果启用）
  void wt_perform_mcs_via_rc_channel_if_enabled();
  void wt_perform_bw_via_rc_channel_if_enabled();

  // Time out to go from wifibroadcast mode to wifi hotspot mode
  // 从wifibroadcast模式切换到wifi热点模式的超时时间
  void wt_perform_air_hotspot_after_timeout();
  // X20 only, thermal protection
  void wt_perform_update_thermal_protection();

  // Returns true if the work item queue is currently empty and the item has
  // been added false otherwise. In general, we only suport one item on the work
  // queue - otherwise we reject the param, since the user can just try again
  // later (and in case the work queue is currently busy with a frequency scan
  // for example, we do not support changing the frequency or similar.
  // 如果工作项队列当前为空且该项已添加，则返回true；否则返回false。
  // 通常情况下，我们只支持工作队列中有一个项——否则我们会拒绝该参数，因为用户可以稍后再试。
  // 例如，如果工作队列当前忙于频率扫描，我们不支持更改频率或类似操作。
  bool try_schedule_work_item(const std::shared_ptr<WorkItem>& work_item);

  // Called by telemetry on both air and ground (send to opposite, respective)
  // 由空中和地面上的遥测调用（发送到相应的对端）
  void transmit_telemetry_data(TelemetryTxPacket packet) override;

  // Called by the camera stream on the air unit only
  // transmit video data via wifibradcast
  // 仅由空中单元的摄像头流调用
  // 通过wifibroadcast传输视频数据
  void transmit_video_data(
      int stream_index,
      const openhd::FragmentedVideoFrame& fragmented_video_frame) override;
  void transmit_audio_data(const openhd::AudioPacket& audio_packet) override;
  // How often per second we broadcast the session key -
  // we send the session key ~2 times per second
  // 我们每秒广播会话密钥的频率 -
  // 我们大约每秒发送会话密钥两次
  static constexpr std::chrono::milliseconds SESSION_KEY_PACKETS_INTERVAL =
      std::chrono::milliseconds(500);
  // This is a long-running operation during which changing things like
  // frequency and more are disabled. Tries to find a running air unit and goes
  // to this frequency if found. continuously broadcasts progress via mavlink.
  // 这是一个长时间运行的操作，在此期间，频率等更改被禁用。
  // 尝试查找正在运行的空中单元，并在找到后切换到该频率。
  // 通过mavlink持续广播进度。
  void perform_channel_scan(
      const openhd::LinkActionHandler::ScanChannelsParam& scan_channels_params);
  // similar to channel scan, analyze channel(s) for interference
  // 类似于频道扫描，分析频道的干扰情况
  void perform_channel_analyze(int channels_to_scan);
  void reset_all_rx_stats();
  void recommend_bitrate_to_encoder(int recommended_video_bitrate_kbits);

  // set passive mode to disabled (do not drop packets) unless we are ground
  // and passive mode is enabled by the user
  // 设置被动模式为禁用（不丢弃数据包），除非我们在地面，并且被动模式已被用户启用
  void re_enable_injection_unless_user_passive_mode_enabled();
  int get_max_fec_block_size();

  // Called when the wifi card (really really likely) disconneccted
  // 当wifi卡（非常可能）断开连接时调用
  void on_wifi_card_fatal_error();

 private:
  const OHDProfile m_profile;
  const std::vector<WiFiCard> m_broadcast_cards;
  std::shared_ptr<spdlog::logger> m_console;
  std::unique_ptr<openhd::WBLinkSettingsHolder> m_settings;
  std::shared_ptr<RadiotapHeaderTxHolder> m_tx_header_1;
  // On air, we use different radiotap data header(s) for different streams
  // (20Mhz vs 40Mhz)
  // 在空中，我们为不同的流使用不同的radiotap数据头
  // （20MHz与40MHz）
  std::shared_ptr<RadiotapHeaderTxHolder> m_tx_header_2;
  std::shared_ptr<WBTxRx> m_wb_txrx;
  // For telemetry, bidirectional in opposite directions
  // 对于遥测，双向通信在相反方向上传输
  std::unique_ptr<WBStreamTx> m_wb_tele_tx;
  std::unique_ptr<WBStreamRx> m_wb_tele_rx;
  // For video, on air there are only tx instances, on ground there are only rx
  // instances.
  // 对于视频，在空中只有发送（tx）实例，在地面只有接收（rx）实例
  std::vector<std::unique_ptr<WBStreamTx>> m_wb_video_tx_list;
  std::vector<std::unique_ptr<WBStreamRx>> m_wb_video_rx_list;
  // For audio or custom data
  // 对于音频或自定义数据
  std::unique_ptr<WBStreamTx> m_wb_audio_tx;
  std::unique_ptr<WBStreamRx> m_wb_audio_rx;
  // We have one worker thread for asynchronously performing operation(s) like
  // changing the frequency but also recalculating statistics that are then
  // forwarded to openhd_telemetry for broadcast
  // 我们有一个工作线程，用于异步执行操作，例如
  // 更改频率，还包括重新计算统计数据，然后将这些数据
  // 转发到openhd_telemetry进行广播
  bool m_work_thread_run;
  std::unique_ptr<std::thread> m_work_thread;
  std::mutex m_work_item_queue_mutex;

  // NOTE: We only support one active work item at a time,
  // otherwise, we reject any changes requested by the user.
  // 注意：我们一次只支持一个活跃的工作项，
  // 否则，我们会拒绝用户请求的任何更改。
  std::queue<std::shared_ptr<WorkItem>> m_work_item_queue;
  static constexpr auto RECALCULATE_STATISTICS_INTERVAL =
      std::chrono::milliseconds(500);
  std::chrono::steady_clock::time_point m_last_stats_recalculation =
      std::chrono::steady_clock::now();
  std::atomic<int> m_max_total_rate_for_current_wifi_config_kbits = 0;
  std::atomic<int> m_max_video_rate_for_current_wifi_fec_config = 0;
  // Whenever the frequency has been changed, we reset tx errors and start new
  // 每当频率发生变化时，我们会重置发送错误并开始新的计数
  bool m_rate_adjustment_frequency_changed = false;
  // bitrate we recommend to the encoder / camera(s)
  // 我们推荐给编码器/摄像头的比特率
  int m_recommended_video_bitrate_kbits = 0;
  std::atomic<int> m_curr_n_rate_adjustments = 0;
  // Set to true when armed, disarmed by default
  // Used to differentiate between different tx power levels when armed /
  // disarmed
  // 在启用时设置为true，默认情况下为禁用
  // 用于区分启用/禁用时的不同发射功率级别
  bool m_is_armed = false;
  std::atomic_bool m_request_apply_tx_power = false;
  std::atomic_bool m_request_apply_air_mcs_index = false;
  std::atomic_bool m_request_apply_air_bw = false;
  std::chrono::steady_clock::time_point m_last_log_bind_phrase_mismatch =
      std::chrono::steady_clock::now();
  // We store tx power for easy access in stats
  // 我们将发射功率存储在统计数据中，便于访问
  std::atomic<int> m_curr_tx_power_idx = 0;
  std::atomic<int> m_curr_tx_power_mw = 0;
  std::atomic<int> m_last_received_packet_ts_ms =
      openhd::util::steady_clock_time_epoch_ms();
  std::chrono::steady_clock::time_point m_reset_frequency_time_point =
      std::chrono::steady_clock::now();
  // 40Mhz / 20Mhz link management
  // 40MHz / 20MHz 链路管理
  std::unique_ptr<ManagementAir> m_management_air = nullptr;
  std::unique_ptr<ManagementGround> m_management_gnd = nullptr;
  // We start on 40Mhz, and go down to 20Mhz if possible
  // 我们从40MHz开始，如果可能的话降到20MHz
  std::atomic<int> m_gnd_curr_rx_channel_width = 40;
  std::atomic<int> m_gnd_curr_rx_frequency = -1;
  // Allows temporarily closing the video input
  // 允许临时关闭视频输入
  std::atomic_bool m_air_close_video_in = false;
  const int m_recommended_max_fec_blk_size_for_this_platform;
  bool m_wifi_card_error_has_been_handled = false;
  // We have 3 thermal protection levels - as of now, only on X20
  // 我们有3个热保护级别 - 目前仅在X20上可用
  static constexpr uint8_t THERMAL_PROTECTION_NONE = 0;
  static constexpr uint8_t THERMAL_PROTECTION_RATE_REDUCED = 1;
  static constexpr uint8_t THERMAL_PROTECTION_VIDEO_DISABLED = 2;
  std::atomic_uint8_t m_thermal_protection_level = 0;
  std::chrono::steady_clock::time_point m_thermal_protection_enable_tp =
      std::chrono::steady_clock::now();

 private:
  openhd::wb::ForeignPacketsHelper m_foreign_p_helper;
  openhd::wb::RCChannelHelper m_rc_channel_helper;
  openhd::wb::FrameDropsHelper m_frame_drop_helper;
  std::atomic_int m_primary_total_dropped_frames = 0;
  std::atomic_int m_secondary_total_dropped_frames = 0;

 private:
  const bool DIRTY_forward_gapped_fragments = false;
  const bool DIRTY_add_aud_nal = false;
  const int DIRTY_emulate_drop_mode = 0;

 private:
  const std::chrono::steady_clock::time_point m_wb_link_start_ts =
      std::chrono::steady_clock::now();
  std::optional<std::chrono::steady_clock::time_point> m_hs_timeout =
      std::chrono::steady_clock::now();
};

#endif
