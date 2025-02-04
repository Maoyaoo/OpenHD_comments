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

#include "WBEndpoint.h"

#include <utility>

#include "openhd_spdlog_include.h"

WBEndpoint::WBEndpoint(std::shared_ptr<OHDLink> link, std::string TAG) : MEndpoint(std::move(TAG)), m_link_handle(std::move(link)) {
    // assert(m_tx_rx_handle);
    // 如果为空，调用 openhd::log::get_default()->warn() 记录一个警告日志，表示空中与地面之间缺少遥测连接。
    if (!m_link_handle) {
        openhd::log::get_default()->warn(
            "WBEndpoint-tx rx handle is missing (no telemetry connection between "
            "air and ground)");
    } else {
        // 注册一个回调函数，当通过 OHDLink 接收到数据时，回调函数会被调用，并且数据会被传递到 MEndpoint::parseNewData 方法进行进一步处理
        auto cb = [this](std::shared_ptr<std::vector<uint8_t>> data) { MEndpoint::parseNewData(data->data(), data->size()); };
        m_link_handle->register_on_receive_telemetry_data_cb(cb);
    }
}

WBEndpoint::~WBEndpoint() {
    if (m_link_handle) {
        m_link_handle->register_on_receive_telemetry_data_cb(nullptr);
    }
}

bool WBEndpoint::sendMessagesImpl(const std::vector<MavlinkMessage>& messages) {
    // 1、消息打包：
    auto message_buffers = aggregate_pack_messages(messages);
    // 2、遍历打包后的消息：
    for (const auto& message_buffer : message_buffers) {
        // 3、检查通信链路有效性
        if (m_link_handle) {
            std::lock_guard<std::mutex> guard(m_send_messages_mutex);  // 线程安全的操作：
            // 实际发送数据：
            m_link_handle->transmit_telemetry_data({message_buffer.aggregated_data, message_buffer.recommended_n_retransmissions});
        }
    }
    return true;
}
