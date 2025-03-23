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

#ifndef OPENHD_WBLINKMANAGER_H
#define OPENHD_WBLINKMANAGER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../lib/wifibroadcast/wifibroadcast/src/WBTxRx.h"  // WB 收发器相关

/**
 * Quite a lot of complicated code to implement 40Mhz without sync of air and
 * ground worth it, though ;) We have public std::atomic members, since the data
 * only needs to be accessed/written atomically from the wb_link worker thread.
 */
/**
 * 这部分代码实现了 40MHz 带宽的管理，虽然复杂，但值得。
 * 我们使用公共的 std::atomic 成员，因为这些数据只需要在 wb_link 工作线程中原子地访问/写入。
 */
// 定义了 ManagementAir 和 ManagementGround 两个类，分别用于管理空中端和地面端的 40MHz 带宽配置。通过发送和接收管理帧，确保空中端和地面端的频率和频道宽度同步。
/**
 * 管理空中端的类。
 */
class ManagementAir {
   public:
    explicit ManagementAir(std::shared_ptr<WBTxRx> wb_tx_rx, int initial_freq_mhz, int inital_channel_width_mhz);
    // 禁用拷贝构造函数和移动构造函数
    ManagementAir(const ManagementAir&) = delete;
    ManagementAir(const ManagementAir&&) = delete;
    ~ManagementAir();
    // 启动管理线程
    void start();
    // TODO dirty
    std::shared_ptr<RadiotapHeaderTxHolder> m_tx_header;
    // Changing the frequency or channel width temporarily increases the interval
    // at which the management frames are sent
    // 设置频率，临时增加管理帧的发送间隔
    void set_frequency(int frequency);
    // 设置频道宽度
    void set_channel_width(uint8_t bw);

   public:
    std::atomic<uint32_t> m_curr_frequency_mhz;     // 当前频率（MHz）
    std::atomic<uint8_t> m_curr_channel_width_mhz;  // 当前频道宽度（MHz）
    // 获取最后接收到的管理帧的时间戳（毫秒）
    int get_last_received_packet_ts_ms();

   private:
    // 管理线程的主循环
    void loop();
    // 处理接收到的管理帧
    void on_new_management_packet(const uint8_t* data, int data_len);
    std::shared_ptr<WBTxRx> m_wb_txrx;                                                                     // WB 收发器实例
    std::shared_ptr<spdlog::logger> m_console;                                                             // 日志记录器
    std::atomic<bool> m_tx_thread_run = true;                                                              // 线程运行标志
    std::unique_ptr<std::thread> m_tx_thread;                                                              // 管理线程
    std::chrono::steady_clock::time_point m_air_last_management_frame = std::chrono::steady_clock::now();  // 最后发送管理帧的时间
    std::atomic<int> m_last_received_packet_timestamp_ms = 0;                                              // 最后接收管理帧的时间戳
    std::chrono::steady_clock::time_point m_increase_interval_tp;                                          // 增加发送间隔的时间点
    std::atomic<int> m_last_change_timestamp_ms;                                                           // 最后更改频率或频道宽度的时间戳
};

/**
 * 管理地面端的类。
 */
class ManagementGround {
   public:
    explicit ManagementGround(std::shared_ptr<WBTxRx> wb_tx_rx);
    // 启动管理线程
    void start();
    // 禁用拷贝构造函数和移动构造函数
    ManagementGround(const ManagementGround&) = delete;
    ManagementGround(const ManagementGround&&) = delete;
    ~ManagementGround();
    // TODO dirty
    std::shared_ptr<RadiotapHeaderTxHolder> m_tx_header;

   public:
    std::atomic<int> m_air_reported_curr_frequency = -1;      // 空中端报告的当前频率
    std::atomic<int> m_air_reported_curr_channel_width = -1;  // 空中端报告的当前频道宽度
    int get_last_received_packet_ts_ms();                     // 获取最后接收到的管理帧的时间戳（毫秒）

   private:
    void loop();
    std::shared_ptr<WBTxRx> m_wb_txrx;                         // WB 收发器实例
    std::shared_ptr<spdlog::logger> m_console;                 // 日志记录器
    std::atomic<bool> m_tx_thread_run = true;                  // 线程运行标志
    std::unique_ptr<std::thread> m_tx_thread;                  // 管理线程
    std::atomic<int> m_last_received_packet_timestamp_ms = 0;  // 最后接收管理帧的时间戳
    // 40Mhz / 20Mhz link management
    // 处理接收到的管理帧
    void on_new_management_packet(const uint8_t* data, int data_len);
};

#endif  // OPENHD_WBLINKMANAGER_H
