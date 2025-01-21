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

#ifndef OPENHD_OPENHD_OHD_COMMON_OPENHD_SPDLOG_HPP_
#define OPENHD_OPENHD_OHD_COMMON_OPENHD_SPDLOG_HPP_

// The goal is to eventually use spdlog throughout all openhd modules, but have
// specific logger(s) on a module basis such that we can enable / disable
// logging for a specific module (e.g. ohd_video: set log level to debug / info)
// when debugging ohd_video.
// 目标是最终在所有 OpenHD 模块中使用 spdlog，但要在模块级别上使用
// 特定的日志记录器，以便在调试特定模块时可以启用/禁用
// 日志记录（例如，ohd_video：设置日志级别为调试/信息）
// 当调试 ohd_video 时。

#include <spdlog/fwd.h>
// #include <spdlog/fmt/fmt.h>
// #include <spdlog/common.h>
// #include <spdlog/spdlog.h>
// # define FMT_STRING(s) s

#include <memory>
#include <mutex>
#include <vector>

namespace openhd::log {

// Note: the _mt loggers have threadsafety by design already, but we need to
// make sure to crete the instance only once For some reason there is no helper
// for that in speeddlog / i haven't found it yet
// 注意：_mt 日志记录器已经通过设计具备线程安全性，但我们需要确保
// 只创建一次实例。由于某些原因，在 spdlog 中没有找到相关的帮助函数
// （或者我还没有找到）。

// Thread-safe but recommended to store result in an intermediate variable
// 线程安全，但建议将结果存储在中间变量中。
std::shared_ptr<spdlog::logger> create_or_get(const std::string& logger_name);

// Uses the thread-safe create_or_get -> slower than using the intermediate
// variable approach, but sometimes you just don't care about that.
// 使用线程安全的 create_or_get -> 比使用中间变量方法慢，但有时你并不在乎这个。
std::shared_ptr<spdlog::logger> get_default();

// By default, only messages of level warn or higher are forwarded via mavlink
// (and then shown in QOpenHD). Use this if you want to show a non-warning
// message in QOpenHD.
// 默认情况下，只有警告级别或更高的消息才会通过 mavlink 转发
// （然后在 QOpenHD 中显示）。如果你想在 QOpenHD 中显示非警告
// 消息，请使用此设置。
void log_via_mavlink(int level, std::string message);

struct MavlinkLogMessage {
  uint8_t level;
  // MUST be null-terminated
  uint8_t message[50];
};

class MavlinkLogMessageBuffer {
 public:
  // Thread-safe
  // Enqueues a log message for the telemetry thread to fetch
  std::vector<MavlinkLogMessage> dequeue_log_messages();
  // Thread-safe
  // Dequeues buffered telemetry log messages,
  // called in regular intervals by the telemetry thread
  void enqueue_log_message(MavlinkLogMessage message);
  // We only have one instance of this class inside openhd
  static MavlinkLogMessageBuffer& instance();

 private:
  std::mutex m_mutex;
  std::vector<MavlinkLogMessage> m_buffer;
};

// these match the mavlink SEVERITY_LEVEL enum, but this code should not depend
// on the mavlink headers See
// https://mavlink.io/en/messages/common.html#MAV_SEVERITY
enum class STATUS_LEVEL {
  EMERGENCY = 0,
  ALERT,
  CRITICAL,
  ERROR,
  WARNING,
  INFO,
  NOTICE,
  DEBUG
};

STATUS_LEVEL level_spdlog_to_mavlink(const spdlog::level::level_enum& level);

// Please use sparingly.
void log_to_kernel(const std::string& message);

// Extra logging method to log without pulling in spdlog / fmt
void debug_log(const std::string& message);
void info_log(const std::string& message);
void warning_log(const std::string& message);

}  // namespace openhd::log

#endif  // OPENHD_OPENHD_OHD_COMMON_OPENHD_SPDLOG_HPP_
