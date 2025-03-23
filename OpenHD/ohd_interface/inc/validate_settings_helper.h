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

#ifndef OPENHD_OPENHD_OHD_INTERFACE_INC_VALIDATE_SETTINGS_HELPER_H_
#define OPENHD_OPENHD_OHD_INTERFACE_INC_VALIDATE_SETTINGS_HELPER_H_

#include <vector>

#include "openhd_spdlog_include.h"  // OpenHD 日志库
#include "wifi_channel.h"           // WiFi 频道相关功能

// 该代码定义了一系列工具函数，用于验证和转换用户可设置的参数，包括：

// WiFi 频率（2.4GHz 和 5GHz）
// 频道宽度
// MCS（调制与编码方案）索引
// 发射功率（毫瓦和毫分贝毫瓦）
// FEC（前向纠错）块长度和百分比
// 这些函数主要用于确保用户输入的参数在有效范围内，避免配置错误导致的系统问题。

// Helper for validating user-selectable settings
// 用户可设置选项的验证辅助工具
namespace openhd {

/**
 * 验证是否为有效的 2.4GHz 频率。
 * @param frequency 频率值
 * @return 如果是有效的 2.4GHz 频率，返回 true；否则返回 false。
 */
static bool is_valid_frequency_2G(uint32_t frequency) {
    const auto supported = openhd::get_channels_2G();
    for (const auto& value : supported) {
        if (value.frequency == frequency)
            return true;
    }
    return false;
}

/**
 * 验证是否为有效的 5GHz 频率。
 * @param frequency 频率值
 * @return 如果是有效的 5GHz 频率，返回 true；否则返回 false。
 */
static bool is_valid_frequency_5G(uint32_t frequency) {
    const auto supported = openhd::get_channels_5G();
    for (const auto& value : supported) {
        if (value.frequency == frequency)
            return true;
    }
    return false;
}

/**
 * 验证是否为有效的频道宽度。
 * @param channel_width 频道宽度值
 * @return 如果是 20 或 40，返回 true；否则返回 false。
 */
static bool is_valid_channel_width(uint32_t channel_width) {
    return channel_width == 20 || channel_width == 40;
}

/**
 * 验证是否为有效的 MCS（Modulation and Coding Scheme）索引。
 * @param mcs_index MCS 索引值
 * @return 如果索引在 0 到 31 之间，返回 true；否则返回 false。
 */
static bool is_valid_mcs_index(uint32_t mcs_index) {
    return mcs_index >= 0 && mcs_index <= 31;
}

/**
 * 验证是否为有效的发射功率（毫瓦）。
 * @param tx_power_mw 发射功率值（毫瓦）
 * @return 如果功率在 10 到 30000 毫瓦之间，返回 true；否则返回 false。
 */
// Internally, OpenHD uses milli watt (mW)
// No wifi card will ever do 30W, but some cards increase their tx power a bit
// more when you set a higher value (I think)
static bool is_valid_tx_power_milli_watt(int tx_power_mw) {
    return tx_power_mw >= 10 && tx_power_mw <= 30 * 1000;
}

/**
 * 验证是否为有效的 FEC（前向纠错）块长度。
 * @param block_length FEC 块长度值
 * @return 如果块长度在 0 到 99 之间，返回 true；否则返回 false。
 */
// NOTE: 0 means variable fec, video codec has to be set in this case
static bool is_valid_fec_block_length(int block_length) {
    return block_length >= 0 && block_length < 100;
}

/**
 * 验证是否为有效的 FEC 百分比。
 * @param fec_perc FEC 百分比值
 * @return 如果百分比在 1 到 400 之间，返回 true；否则返回 false。
 */
// max 100% fec (2x the amount of data), this is already too much
// 21.10: Using more than 2x for FEC can be usefully for testing
static bool is_valid_fec_percentage(int fec_perc) {
    bool valid = fec_perc > 0 && fec_perc <= 400;
    if (!valid) {
        openhd::log::warning_log("Invalid fec percentage");
    }
    return valid;
}

/**
 * 将毫分贝毫瓦（milli-dBm）转换为毫瓦（milli-watt）。
 * @param milli_dbm 毫分贝毫瓦值
 * @return 转换后的毫瓦值。
 */
// https://www.rapidtables.com/convert/power/dBm_to_mW.html
// P(mW) = 1mW ⋅ 10(P(dBm)/ 10)
static float milli_dbm_to_milli_watt(float milli_dbm) {
    double exponent = milli_dbm / 1000.0 / 10.0;
    auto ret = std::pow(10.0, exponent);
    return static_cast<float>(ret);
}

/**
 * 将毫瓦（milli-watt）转换为毫分贝毫瓦（milli-dBm）。
 * @param milli_watt 毫瓦值
 * @return 转换后的毫分贝毫瓦值。
 */
// P(dBm) = 10 ⋅ log10( P(mW) / 1mW)
static uint32_t milli_watt_to_milli_dbm(uint32_t milli_watt) {
    const double tmp = std::log10(static_cast<double>(milli_watt) / 1.0);
    const double milli_dbm = tmp * 10 * 100;
    // return static_cast<uint32_t>(milli_dbm);
    return std::lround(milli_dbm);
}
// However, this is weird:
// https://linux.die.net/man/8/iwconfig
// the power in dBm is P = 30 + 10.log(W)
// log10(x/1)==log(x) / log(10) = ~2.3

/**
 * 缩放调整后的毫瓦（milli-watt）转换为毫分贝毫瓦（milli-dBm）。
 * @param milli_watt 毫瓦值
 * @param scaler 缩放因子
 * @param remover 偏移量
 * @return 转换后的毫分贝毫瓦值。
 */
static uint32_t milli_watt_to_mBm(uint32_t milli_watt, double scaler, double remover) {
    const double tmp = std::log10(static_cast<double>(milli_watt) / 1.0);
    const double milli_dbm = (tmp * 10 * 100 * scaler) - remover;
    // return static_cast<uint32_t>(milli_dbm);
    return std::lround(milli_dbm);
}

}  // namespace openhd

#endif  // OPENHD_OPENHD_OHD_INTERFACE_INC_VALIDATE_SETTINGS_HELPER_H_
