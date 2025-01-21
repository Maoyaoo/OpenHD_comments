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

#include "openhd_bitrate.h"

// 构造函数接收一个标签 tag 和一个布尔值 debug_pps，并初始化成员变量。m_console 是用于日志记录的对象，m_debug_pps 用于控制是否调试数据包每秒（pps）。
openhd::BitrateDebugger::BitrateDebugger(std::string tag, bool debug_pps)
    : m_debug_pps(debug_pps) {
  m_console = openhd::log::create_or_get(tag);
}


// 这个方法每当收到数据包时被调用。它会累计数据包大小并计算比特率（以字节/秒为单位）以及数据包每秒（pps）。如果一秒钟过去了，它会打印出这些信息，并重置计数器以便下一次计算。
void openhd::BitrateDebugger::on_packet(int64_t n_bytes) {
  assert(n_bytes > 0);
  m_bytes += n_bytes;
  m_n_packets++;
  const auto elapsed = std::chrono::steady_clock::now() - m_last_log;
  if (elapsed > std::chrono::seconds(1)) {
    const double elapsed_us =
        (double)std::chrono::duration_cast<std::chrono::microseconds>(elapsed)
            .count();
    const double elapsed_s = elapsed_us / 1000.0 / 1000.0;
    const double bytes_per_s = static_cast<double>(m_bytes) / elapsed_s;
    const double pps = static_cast<double>(m_n_packets) / elapsed_s;
    if (m_debug_pps) {
      m_console->debug("{} {}", bytes_per_second_to_string(bytes_per_s),
                       openhd::pps_to_string(pps));
    } else {
      m_console->debug("{}", bytes_per_second_to_string(bytes_per_s));
    }
    m_bytes = 0;
    m_n_packets = 0;
    m_last_log = std::chrono::steady_clock::now();
  }
}

// 此方法将比特率（以比特/秒为单位）转换为字符串形式，单位为 mBit/s 或 kBit/s，取决于值的大小。
std::string openhd::bits_per_second_to_string(uint64_t bits_per_second) {
  const auto mBits_per_second =
      static_cast<double>(bits_per_second) / (1000.0 * 1000.0);
  std::stringstream ss;
  ss.precision(2);
  if (mBits_per_second > 1) {
    ss << mBits_per_second << "mBit/s";
    return ss.str();
  }
  const auto kBits_per_second = static_cast<float>(bits_per_second) / 1000;
  ss << kBits_per_second << "kBit/s";
  return ss.str();
}

// 这个方法将字节数（以字节/秒为单位）转换为比特率字符串（单位为 mBit/s 或 kBit/s）。
std::string openhd::bytes_per_second_to_string(double bytes_per_second) {
  const auto bits_per_second = bytes_per_second * 8;
  std::stringstream ss;
  ss.precision(2);
  if (bits_per_second > 1000 * 1000) {
    const auto mBits_per_second =
        static_cast<double>(bits_per_second) / (1000.0 * 1000.0);
    ss << mBits_per_second << "mBit/s";
    return ss.str();
  }
  const auto kBits_per_second = static_cast<double>(bits_per_second) / 1000;
  ss << kBits_per_second << "kBit/s";
  return ss.str();
}

// 该方法将数据包每秒（pps）转换为字符串形式，单位为 pps
std::string openhd::pps_to_string(double pps) {
  std::stringstream ss;
  ss.precision(2);
  ss << pps << "pps";
  return ss.str();
}
