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

#ifndef OPENHD_ETHERNET_LINK_H
#define OPENHD_ETHERNET_LINK_H

#include <memory>
#include <thread>

#include "openhd_config.h"
#include "openhd_link.hpp"
#include "openhd_udp.h"
#include "openhd_util.h"

// 用于实现基于以太网的通信链接，主要用于 OpenHD 项目中。它支持传输以下类型的数据：

// 视频数据：通过 UDP 传输视频流。
// 遥测数据：通过 UDP 传输遥测信息。
// 音频数据：通过 UDP 传输音频数据。
class EthernetLink : public OHDLink {
   public:
    // 构造函数，支持从配置文件初始化
    EthernetLink(const openhd::Config& config, OHDProfile profile);
    EthernetLink(OHDProfile profile);
    ~EthernetLink();

    // OHDLink implementations
    void transmit_telemetry_data(TelemetryTxPacket packet) override;                                                  // 传输遥测数据
    void transmit_video_data(int stream_index, const openhd::FragmentedVideoFrame& fragmented_video_frame) override;  // 传输视频数据
    void transmit_audio_data(const openhd::AudioPacket& audio_packet) override;                                       // 传输音频数据

   private:
    OHDProfile m_profile;     // 当前设备的配置信息
    openhd::Config m_config;  // 配置文件
    // Configuration variables (defaults if not overridden)
    std::string GROUND_UNIT_IP = "192.168.2.1";  // 地面单元 IP
    std::string AIR_UNIT_IP = "192.168.2.18";    // 空中单元 IP
    int VIDEO_PORT = 5910;                       // 视频数据端口
    int TELEMETRY_PORT = 5920;                   // 遥测数据端口

    std::unique_ptr<openhd::UDPForwarder> m_video_tx;      // Video transmitter 视频数据发送器
    std::unique_ptr<openhd::UDPReceiver> m_video_rx;       // Video receiver 视频数据接收器
    std::unique_ptr<openhd::UDPForwarder> m_telemetry_tx;  // Telemetry transmitter 遥测数据发送器
    std::unique_ptr<openhd::UDPReceiver> m_telemetry_rx;   // Telemetry receiver 遥测数据接收器

    void initialize_air_unit();     // 初始化空中单元的函数
    void initialize_ground_unit();  // 初始化地面单元的函数

    void handle_video_data(int stream_index, const uint8_t* data, int data_len);  // 处理接收到的视频数据的函数
    void handle_telemetry_data(const uint8_t* data, int data_len);                // 处理接收到的遥测数据的函数
};

#endif  // OPENHD_ETHERNET_LINK_H
