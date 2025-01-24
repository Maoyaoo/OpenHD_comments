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

#ifndef OPENHD_OPENHD_OHD_COMMON_OPENHD_ACTION_HANDLER_HPP_
#define OPENHD_OPENHD_OHD_COMMON_OPENHD_ACTION_HANDLER_HPP_

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <utility>

#include "openhd_link_statistics.hpp"
#include "openhd_spdlog.h"
#include "openhd_util.h"

// This class exists to handle the rare case(s) when one openhd module needs to
// talk to another. For example, the wb link (ohd_interface) might request a
// lower encoder bitrate (ohd_video) Since we do not have any code dependencies
// between the subbmodules directly (other than that they all depend on
// ohd_common) we solve this issue by exposing action handlers as singletons
// here.
// 此类的存在是为了解决少数情况下一个 OpenHD 模块需要与另一个模块通信的情况。
// 例如，wb 链路（ohd_interface）可能会请求降低编码器比特率（ohd_video）。
// 由于子模块之间没有直接的代码依赖（除了它们都依赖于 ohd_common），
// 我们通过在此处将操作处理程序公开为单例来解决此问题。
namespace openhd {

// In a few places inside openhd we need to react to changes on the FC arming
// state. Here one can register / unregister a cb that is called whenever the
// arming state changes The default arming state is disarmed
// 在 OpenHD 的某些地方，我们需要对飞控（FC）解锁状态的变化作出反应。
// 在这里，可以注册/注销一个回调函数，该回调函数会在解锁状态发生变化时被调用。
// 默认的解锁状态是未解锁（disarmed）。
class ArmingStateHelper {
 public:
  ArmingStateHelper() = default;
  ArmingStateHelper(const ArmingStateHelper&) = delete;
  ArmingStateHelper(const ArmingStateHelper&&) = delete;
  static ArmingStateHelper& instance();
  typedef std::function<void(const bool armed)> STATE_CHANGED_CB;
  /**
   * Register a listener that is called reliably whenever the arming state
   * changes (disarm, arm, disarm,... for example)
   * @param tag needs to be a unique tag (per all submodules)
   */
  void register_listener(const std::string& tag, STATE_CHANGED_CB cb);
  /**
   * Unregister a previously registered cb.
   */
  void unregister_listener(const std::string& tag);
  // For fetching the arming state in a manner where a deterministic arm /
  // disarm pattern is not needed
  bool is_currently_armed() { return m_is_armed; }
  void update_arming_state_if_changed(bool armed);

 private:
  std::atomic_bool m_is_armed = false;
  std::map<std::string, STATE_CHANGED_CB> m_cbs;
  std::shared_ptr<spdlog::logger> m_console =
      openhd::log::create_or_get("ArmingStateHelper");
};

// In (only one) place right now we need to react to changes on the RC channels
// the FC reports
// 目前（只有一个地方）我们需要对飞控（FC）报告的遥控器（RC）通道的变化作出反应。
class FCRcChannelsHelper {
 public:
  FCRcChannelsHelper() = default;
  FCRcChannelsHelper(const FCRcChannelsHelper&) = delete;
  FCRcChannelsHelper(const FCRcChannelsHelper&&) = delete;
  static FCRcChannelsHelper& instance();
  typedef std::function<void(const std::array<int, 18>& rc_channels)>
      ACTION_ON_ANY_RC_CHANNEL_CB;
  // called every time a rc channel value(s) mavlink packet is received from the
  // FC (regardless if there was an actual change on any of the channels or not)
  // Works well on Ardupilot, which broadcasts the proper telem message by
  // default
  // 每次从飞控（FC）接收到包含遥控器（RC）通道值的 MAVLink 数据包时调用
  // （无论任意通道是否发生了实际变化）。
  // 该功能在 Ardupilot 上运行良好，因为它默认会广播正确的遥测消息。
  void update_rc_channels(const std::array<int, 18>& rc_channels);
  void action_on_any_rc_channel_register(ACTION_ON_ANY_RC_CHANNEL_CB cb);

 private:
  std::shared_ptr<ACTION_ON_ANY_RC_CHANNEL_CB> m_action_rc_channel = nullptr;
};

class LinkActionHandler {
 public:
  LinkActionHandler() = default;
  LinkActionHandler(const LinkActionHandler&) = delete;
  LinkActionHandler(const LinkActionHandler&&) = delete;
  static LinkActionHandler& instance();

 public:
  // Link bitrate change request
  struct LinkBitrateInformation {
    int recommended_encoder_bitrate_kbits;
  };
  typedef std::function<void(LinkBitrateInformation link_bitrate_info)>
      ACTION_REQUEST_BITRATE_CHANGE;
  static std::string link_bitrate_info_to_string(
      const LinkBitrateInformation& lb) {
    std::stringstream ss;
    ss << "[recommended_encoder_bitrate:";
    ss << lb.recommended_encoder_bitrate_kbits;
    ss << "kBit/s]";
    return ss.str();
  }
  // used by ohd_video
  void action_request_bitrate_change_register(
      const ACTION_REQUEST_BITRATE_CHANGE& cb) {
    if (cb == nullptr) {
      m_action_request_bitrate_change = nullptr;
      return;
    }
    m_action_request_bitrate_change =
        std::make_shared<ACTION_REQUEST_BITRATE_CHANGE>(cb);
  }
  // called by ohd_interface / wb
  void action_request_bitrate_change_handle(
      LinkBitrateInformation link_bitrate_info) {
    // openhd::log::get_default()->debug("action_request_bitrate_change_handle
    // {}", link_bitrate_info_to_string(link_bitrate_info));
    auto tmp = m_action_request_bitrate_change;
    if (tmp) {
      auto& cb = *tmp;
      // The cb will update setting the global atomic value for cam1 / cam2
      // accordingly
      cb(link_bitrate_info);
    }
  }

 public:
  // checking both 2G and 5G channels takes really long, but in rare cases might
  // be wanted by the user checking both 20Mhz and 40Mhz (instead of only either
  // of them both) also duplicates the scan time
  // 检查 2G 和 5G 频段的通道会耗费很长时间，但在少数情况下可能是用户所需要的。
  // 同时检查 20MHz 和 40MHz（而不是仅检查其中之一）也会使扫描时间加倍。
  struct ScanChannelsParam {
    uint32_t channels_to_scan = 0;
  };
  std::function<bool(ScanChannelsParam)> wb_cmd_scan_channels = nullptr;

 public:
  // Cleanup, set all lambdas that handle things to nullptr
  void disable_all_callables() {
    action_request_bitrate_change_register(nullptr);
    wb_cmd_scan_channels = nullptr;
    wb_cmd_analyze_channels = nullptr;
    wb_get_supported_channels = nullptr;
  }

 private:
  // By using shared_ptr to wrap the stored the cb we are semi thread-safe
  std::shared_ptr<ACTION_REQUEST_BITRATE_CHANGE>
      m_action_request_bitrate_change = nullptr;
  std::shared_ptr<openhd::link_statistics::STATS_CALLBACK>
      m_link_statistics_callback = nullptr;

 public:
  // Camera stats / info that is broadcast in regular intervals
  // Set by the camera streaming implementation - read by OHDMainComponent
  // (mavlink broadcast) Simple read - write pattern (mutex is a bit overkill,
  // but we don't have atomic struct)
  // 定期广播的摄像头统计信息/状态。
  // 由摄像头流媒体实现设置 - 由 OHDMainComponent 读取（通过 MAVLink 广播）。
  // 简单的读写模式（使用互斥锁稍显过度，但我们没有原子结构）。
  struct CamInfo {
    bool active = false;  // Do not send stats for a non-active camera
    uint8_t cam_index = 0;
    uint8_t cam_type = 0;
    uint8_t cam_status = 0;
    uint8_t air_recording_active = 0;
    uint8_t encoding_format = 0;
    uint16_t encoding_bitrate_kbits = 0;
    uint8_t encoding_keyframe_interval = 0;
    uint16_t stream_w = 0;
    uint16_t stream_h = 0;
    uint16_t stream_fps = 0;
    uint8_t supports_variable_bitrate = 0;
    uint8_t qp_max = 0;
    uint8_t qp_min = 0;
  };
  void set_cam_info(uint8_t cam_index, CamInfo camInfo) {
    if (cam_index == 0) {
      std::lock_guard<std::mutex> lock(m_cam_info_cam1_mutex);
      m_cam_info_cam1 = camInfo;
    } else {
      std::lock_guard<std::mutex> lock(m_cam_info_cam2_mutex);
      m_cam_info_cam2 = camInfo;
    }
  }
  void set_cam_info_bitrate(uint8_t cam_index, uint16_t bitrate_kbits) {
    if (cam_index == 0) {
      std::lock_guard<std::mutex> lock(m_cam_info_cam1_mutex);
      m_cam_info_cam1.encoding_bitrate_kbits = bitrate_kbits;
    } else {
      std::lock_guard<std::mutex> lock(m_cam_info_cam2_mutex);
      m_cam_info_cam2.encoding_bitrate_kbits = bitrate_kbits;
    }
  }
  void set_cam_info_status(uint8_t cam_index, uint8_t status) {
    if (cam_index == 0) {
      std::lock_guard<std::mutex> lock(m_cam_info_cam1_mutex);
      m_cam_info_cam1.cam_status = status;
    } else {
      std::lock_guard<std::mutex> lock(m_cam_info_cam2_mutex);
      m_cam_info_cam2.cam_status = status;
    }
  }
  void set_cam_info_type(uint8_t cam_index, uint8_t type) {
    if (cam_index == 0) {
      std::lock_guard<std::mutex> lock(m_cam_info_cam1_mutex);
      m_cam_info_cam1.cam_type = type;
    } else {
      std::lock_guard<std::mutex> lock(m_cam_info_cam2_mutex);
      m_cam_info_cam2.cam_type = type;
    }
  }
  CamInfo get_cam_info(int cam_index) {
    if (cam_index == 0) {
      std::lock_guard<std::mutex> lock(m_cam_info_cam1_mutex);
      return m_cam_info_cam1;
    }
    std::lock_guard<std::mutex> lock(m_cam_info_cam2_mutex);
    return m_cam_info_cam2;
  }

 private:
  CamInfo m_cam_info_cam1{};
  CamInfo m_cam_info_cam2{};
  std::mutex m_cam_info_cam1_mutex;
  std::mutex m_cam_info_cam2_mutex;
  // LINK STATISTICS
  // Written by wb_link, published via mavlink by telemetry OHDMainComponent
 private:
  std::mutex m_last_link_stats_mutex;
  openhd::link_statistics::StatsAirGround m_last_link_stats{};

 public:
  void update_link_stats(openhd::link_statistics::StatsAirGround stats) {
    std::lock_guard<std::mutex> guard(m_last_link_stats_mutex);
    m_last_link_stats = std::move(stats);
  }
  openhd::link_statistics::StatsAirGround get_link_stats() {
    std::lock_guard<std::mutex> guard(m_last_link_stats_mutex);
    return m_last_link_stats;
  }

 public:
  std::function<std::vector<uint16_t>()> wb_get_supported_channels = nullptr;
  std::function<bool(int)> wb_cmd_analyze_channels = nullptr;

 public:
  std::atomic<int> scan_channels_air_unit_progress = -1;

 public:
  struct AnalyzeChannelsResult {
    std::array<uint16_t, 30> channels_mhz{0};
    std::array<uint16_t, 30> foreign_packets{0};
    int8_t progress;
  };
  void add_analyze_result(AnalyzeChannelsResult scan_result) {
    std::lock_guard<std::mutex> guard(m_scan_results_mutex);
    m_scan_results.push_back(scan_result);
  }
  std::vector<AnalyzeChannelsResult> get_analyze_results() {
    std::lock_guard<std::mutex> guard(m_scan_results_mutex);
    auto ret = m_scan_results;
    m_scan_results.clear();
    return ret;
  }

 private:
  std::mutex m_scan_results_mutex;
  std::vector<AnalyzeChannelsResult> m_scan_results;

 public:
  struct ScanChannelsProgress {
    uint16_t channel_mhz;
    uint8_t progress;
    uint8_t channel_width_mhz;
    bool success;
  };
  void add_scan_channels_progress(ScanChannelsProgress val) {
    std::lock_guard<std::mutex> guard(m_scan_channels_progress_mutex);
    m_scan_channels_progress.push_back(val);
  }
  std::vector<ScanChannelsProgress> get_scan_channels_progress() {
    std::lock_guard<std::mutex> guard(m_scan_channels_progress_mutex);
    auto ret = m_scan_channels_progress;
    m_scan_channels_progress.clear();
    return ret;
  }

 private:
  std::mutex m_scan_channels_progress_mutex;
  std::vector<ScanChannelsProgress> m_scan_channels_progress;

 public:
  // See mavlink for values
  std::atomic_uint8_t m_wifi_hotspot_state = 0;
  std::atomic_uint16_t m_wifi_hotspot_frequency = 0;
  std::atomic_uint8_t m_ethernet_hotspot_state = 0;
};


class TerminateHelper {
 public:
  static TerminateHelper& instance();
  void terminate_after(std::string tag, std::chrono::milliseconds delay);
  bool should_terminate();
  std::string terminate_reason();

 private:
  std::atomic_bool m_should_terminate = false;
  int64_t m_terminate_tp = 0;
  std::string m_terminate_reason;
};

}  // namespace openhd

#endif  // OPENHD_OPENHD_OHD_COMMON_OPENHD_ACTION_HANDLER_HPP_
