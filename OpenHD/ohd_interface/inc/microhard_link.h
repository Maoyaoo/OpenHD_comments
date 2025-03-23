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

#ifndef OPENHD_MICROHARD_LINK_H
#define OPENHD_MICROHARD_LINK_H

#include "openhd_link.hpp"
#include "openhd_settings_imp.h"
#include "openhd_udp.h"

/**
 * Link implementation for microhard modules
 */
class MicrohardLink : public OHDLink {
   public:
    explicit MicrohardLink(OHDProfile profile);

    // 实现 OHDLink 的虚函数
    void transmit_telemetry_data(TelemetryTxPacket packet) override;                                                  // 传输遥测数据
    void transmit_video_data(int stream_index, const openhd::FragmentedVideoFrame& fragmented_video_frame) override;  // 传输视频数据
    void transmit_audio_data(const openhd::AudioPacket& audio_packet) override;                                       // 传输音频数据

    /**
     * 获取所有的 Mavlink 设置
     * @return 返回所有设置的列表，值可能会根据空中/地面单元和使用的硬件而变化
     */
    std::vector<openhd::Setting> get_all_settings();

    /**
     * 监控网关信号强度
     * @param gateway_ip 网关的 IP 地址
     */
    static void monitor_gateway_signal_strength(const std::string& gateway_ip);

   private:
    const OHDProfile m_profile;                        // 当前设备的配置信息
    std::unique_ptr<openhd::UDPForwarder> m_video_tx;  // 视频数据发送器
    std::unique_ptr<openhd::UDPReceiver> m_video_rx;   // 视频数据接收器
    //
    std::unique_ptr<openhd::UDPReceiver> m_telemetry_tx_rx;  // 遥测数据收发器
};

#endif  // OPENHD_MICROHARD_LINK_H
