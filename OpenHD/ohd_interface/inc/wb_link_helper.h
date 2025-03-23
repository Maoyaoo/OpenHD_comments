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

#ifndef OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_HELPER_H_
#define OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_HELPER_H_

#include <mutex>     // 互斥锁
#include <optional>  // 可选值
#include <utility>   // 工具函数

#include "TimeHelper.hpp"      // 时间相关工具
#include "openhd_spdlog.h"     // OpenHD 日志库
#include "wb_link_settings.h"  // WB link 设置相关

// 该代码定义了一系列辅助工具类和方法，用于简化 wb_link 类的实现，包括：

// WiFi 卡片管理：验证卡片是否支持特定频率、频道宽度和功能，以及设置频率、频道宽度和发射功率。
// 数据包管理：统计和处理外来数据包。
// RC 通道管理：读取和解析遥控器通道数据。
// 帧丢失管理：检测帧丢失并根据需要调整比特率。
// 主要功能
// WiFi 卡片管理：

// 验证卡片是否支持特定频率、频道宽度和 MCS 索引。
// 为所有卡片设置频率、频道宽度和发射功率。
// 检查卡片类型（如是否为 RTL8812AU）。
// 数据包管理：

// 统计外来数据包数量并计算每秒的外来数据包数。
// RC 通道管理：

// 读取和解析遥控器通道数据。
// 根据通道数据获取 MCS 索引和带宽。
// 帧丢失管理：

// 检测帧丢失并决定是否需要降低比特率。
// 允许延迟错误检测以避免误报。

/**
 * The wb_link class is becoming a bit big and therefore hard to read.
 * Here we have some common helper methods used in wb_link
 */
/**
 * wb_link 类变得庞大且难以阅读，
 * 这里提供了一些在 wb_link 中使用的通用辅助方法。
 */
namespace openhd::wb {

/**
 * @return true if the "disable all frequency checks" file exists
 */
/**
 * @return 如果存在 "disable all frequency checks" 文件，返回 true。
 */
bool disable_all_frequency_checks();

/**
 * returns true if all the given cards supports the given frequency
 */
/**
 * 返回给定的所有卡是否支持指定的频率。
 */
bool all_cards_support_frequency(uint32_t frequency, const std::vector<WiFiCard>& m_broadcast_cards, const std::shared_ptr<spdlog::logger>& m_console);

/**
 * 返回给定的所有卡片是否支持指定的频率和频道宽度。
 */
bool all_cards_support_frequency_and_channel_width(uint32_t frequency,
                                                   uint32_t channel_width,
                                                   const std::vector<WiFiCard>& m_broadcast_cards,
                                                   const std::shared_ptr<spdlog::logger>& m_console);
/**
 * 验证频率更改是否有效。
 */
bool validate_frequency_change(int new_frequency, int current_channel_width, const std::vector<WiFiCard>& m_broadcast_cards, const std::shared_ptr<spdlog::logger>& m_console);
/**
 * 验证频道宽度更改是否有效。
 */
bool validate_air_channel_width_change(int new_channel_width, const WiFiCard& card, const std::shared_ptr<spdlog::logger>& m_console);
/**
 * 验证 MCS 索引更改是否有效。
 */
bool validate_air_mcs_index_change(int new_mcs_index, const WiFiCard& card, const std::shared_ptr<spdlog::logger>& m_console);
/**
 * 返回是否有任何卡片支持指定的频率。
 */
bool any_card_support_frequency(uint32_t frequency, const std::vector<WiFiCard>& m_broadcast_cards, const std::shared_ptr<spdlog::logger>& m_console);
/**
 * 为所有卡片设置频率和频道宽度。
 */
bool set_frequency_and_channel_width_for_all_cards(uint32_t frequency, uint32_t channel_width, const std::vector<WiFiCard>& m_broadcast_cards);

/**
 * 为所有卡片设置发射功率。
 */
void set_tx_power_for_all_cards(int tx_power_mw, int rtl8812au_tx_power_index_override, const std::vector<WiFiCard>& m_broadcast_cards);

/**
 * 获取所有卡片的设备名称。
 */
// WB takes a list of card device names
std::vector<std::string> get_card_names(const std::vector<WiFiCard>& cards);

/**
 * 返回是否有任何卡片是 rtl8812au 类型。
 */
// Returns true if any of the given cards is of type rtl8812au
bool has_any_rtl8812au(const std::vector<WiFiCard>& cards);

/**
 * 返回是否有任何卡片不是 rtl8812au 类型。
 */
// Returns true if any of the given cards is not of type rtl8812au
bool has_any_non_rtl8812au(const std::vector<WiFiCard>& cards);

/**
 * 返回是否有任何卡片支持 STBC、LDPC 和 SGI。
 */
bool any_card_supports_stbc_ldpc_sgi(const std::vector<WiFiCard>& cards);

/**
 * 获取扫描频道的频率列表。
 */
std::vector<WifiChannel> get_scan_channels_frequencies(const WiFiCard& card, int channels_to_scan);
/**
 * 获取分析频道的频率列表。
 */
std::vector<WifiChannel> get_analyze_channels_frequencies(const WiFiCard& card, int channels_to_scan);

/**
 * 移除网络管理器对给定卡片的控制，
 * 并确保没有干扰监控模式的 Linux 进程在运行，然后将其设置为监控模式。
 */
/*
 * Removes network manager from the given cards (if it is running)
 * and in general tries to make sure no linux stuff that would interfer with
 * monitor mode is running on the card(s), and then sets them into monitor mode.
 */
void takeover_cards_monitor_mode(const std::vector<WiFiCard>& cards, std::shared_ptr<spdlog::logger> console);

/**
 * 将卡片归还给网络管理器。
 */
/**
 * Gives the card(s) back to network manager;
 */
void giveback_cards_monitor_mode(const std::vector<WiFiCard>& cards, std::shared_ptr<spdlog::logger> console);

/**
 * 根据 WiFi 配置计算比特率（kbits）。
 */
int calculate_bitrate_for_wifi_config_kbits(const WiFiCard& card, int frequency_mhz, int channel_width_mhz, int mcs_index, int dev_adjustment_percent, bool debug_log);

/**
 * 用于管理外来数据包的辅助类。
 */
class ForeignPacketsHelper {
   public:
    // 更新数据包计数
    void update(uint64_t count_p_any, uint64_t count_p_valid) {
        const uint64_t n_foreign_packets = count_p_any > count_p_valid ? count_p_any - count_p_valid : 0;
        if (m_foreign_packets_last_time > n_foreign_packets) {
            m_foreign_packets_last_time = n_foreign_packets;
            return;
        }
        const int delta = static_cast<int>(n_foreign_packets - m_foreign_packets_last_time);
        m_foreign_packets_last_time = n_foreign_packets;
        update_n_foreign_packets(delta);
    }
    // 获取每秒的外来数据包数
    int get_foreign_packets_per_second() const { return m_pps_current; }
    // 更新外来数据包计数
    void update_n_foreign_packets(int n_foreign_packets) {
        assert(n_foreign_packets >= 0);
        // openhd::log::get_default()->debug("N foreign
        // packets:{}",n_foreign_packets);
        m_pps_foreign_packets_count += n_foreign_packets;
        const auto elapsed = std::chrono::steady_clock::now() - m_pps_last_recalculation;
        if (elapsed > std::chrono::seconds(1)) {
            m_pps_last_recalculation = std::chrono::steady_clock::now();
            if (m_pps_foreign_packets_count <= 0) {
                m_pps_current = 0;
                return;
            }
            const int elapsed_us = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
            m_pps_current = m_pps_foreign_packets_count * 1000 * 1000 / elapsed_us;
            m_pps_foreign_packets_count = 0;
        }
    }

   private:
    uint64_t m_foreign_packets_last_time = 0;
    int m_pps_foreign_packets_count = 0;
    std::chrono::steady_clock::time_point m_pps_last_recalculation = std::chrono::steady_clock::now();
    int m_pps_current = -1;
};

/**
 * This class basically only offers atomic read / write operations on the "RC
 * CHANNELS" as reported by the FC. This is needed for the "MCS VIA RC CHANNEL
 * CHANGE" feature.
 */
/**
 * 用于管理 RC 通道的辅助类。
 */
class RCChannelHelper {
   public:
    // Atomic / thread-safe setter / getter
    // 原子设置/获取 RC 通道
    void set_rc_channels(const std::array<int, 18>& rc_channels);
    std::optional<std::array<int, 18>> get_fc_reported_rc_channels();

    // 根据 RC 通道获取 MCS 索引
    /**
     * Get the mcs index mapping pwm channel (channel_index) to mcs indices.
     * If no rc data has been supplied by the FC yet and / or the channel index is
     * invalid or the pwm value is not valid, return std::nullopt.
     */
    std::optional<int> get_mcs_from_rc_channel(int channel_index, std::shared_ptr<spdlog::logger>& m_console);

    // 根据 RC 通道获取带宽
    // returns either a valid channel width (20 /40) or std::nullopt
    std::optional<uint8_t> get_bw_from_rc_channel(int channel_index);

   private:
    std::optional<std::array<int, 18>> m_rc_channels;
    std::mutex m_rc_channels_mutex;
};

/**
 * 用于管理帧丢失的辅助类。
 */
class FrameDropsHelper {
   public:
    // Thread-safe, aka can be called from the thread injecting frame(s) in
    // reference to the wb_link worker thread
    // 通知帧丢失
    void notify_dropped_frame(int n_dropped = 1) { m_frame_drop_counter += n_dropped; }
    // Thread-safe as long as it is called from the thread performing management
    // 检查是否需要降低比特率
    bool needs_bitrate_reduction() {
        if (m_opt_no_error_delay.has_value()) {
            if (std::chrono::steady_clock::now() >= m_opt_no_error_delay) {
                const auto elapsed = std::chrono::steady_clock::now() - m_last_check;
                m_last_check = std::chrono::steady_clock::now();
                const int dropped_since_last_check = m_frame_drop_counter.exchange(0);
                m_console->debug(
                    "Dropped {} frames in {} during adjust period (no bitrate "
                    "reduction)",
                    dropped_since_last_check, MyTimeHelper::R(elapsed));
                m_opt_no_error_delay = std::nullopt;
            }
            return false;
        }
        const auto elapsed = std::chrono::steady_clock::now() - m_last_check;
        if (elapsed >= std::chrono::seconds(3)) {
            m_last_check = std::chrono::steady_clock::now();
            const int dropped_since_last_check = m_frame_drop_counter.exchange(0);
            static constexpr int MAX_DROPPED_FRAMES_ALLOWED = 3;
            if (dropped_since_last_check > MAX_DROPPED_FRAMES_ALLOWED) {
                m_console->debug("Dropped {} frames during {} delta period", dropped_since_last_check, MyTimeHelper::R(elapsed));
                return true;
            }
        }
        return false;
    }
    // 设置日志记录器
    void set_console(std::shared_ptr<spdlog::logger> console) { m_console = std::move(console); }
    // Every time we change the bitrate, it might take some time until the camera
    // reacts (TODO: Define a minumum allowed variance for openhd supported
    // cameras)
    // - this results in dropped frame(s) during this period not being reported as
    // an error (Such that we don't do any rate reduction while the encoder is
    // still reacting to the newly set bitrate)
    // 延迟错误检测
    void delay_for(std::chrono::milliseconds delay) { m_opt_no_error_delay = std::chrono::steady_clock::now() + delay; }

   private:
    std::shared_ptr<spdlog::logger> m_console;
    std::chrono::steady_clock::time_point m_last_check = std::chrono::steady_clock::now();
    std::atomic_int m_frame_drop_counter = 0;
    std::optional<std::chrono::steady_clock::time_point> m_opt_no_error_delay = std::nullopt;
};

/**
 * 用于管理信道污染的辅助类（未实现）。
 */
class PollutionHelper {
   public:
   private:
};

}  // namespace openhd::wb

#endif  // OPENHD_OPENHD_OHD_INTERFACE_INC_WB_LINK_HELPER_H_
