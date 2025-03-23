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

#ifndef OPENHD_VIDEO_OHDVIDEO_H
#define OPENHD_VIDEO_OHDVIDEO_H

#include <string>

#include "camerastream.h"
#include "ohd_video_air_generic_settings.h"
#include "openhd_external_device.h"
#include "openhd_link.hpp"
#include "openhd_platform.h"
#include "openhd_spdlog.h"
#include "openhd_udp.h"

class GstAudioStream;
/**
 * Main entry point for OpenHD video streaming for discovered cameras on the air
 * unit. NOTE: Camera(s) and camera settings are local on the air unit, the
 * ground unit does not need to know anything about that - it just "stupidly"
 * forwards received video data. Therefore, we only create an instance of this
 * class on the air unit. See the Readme.md and camerastream.h for more
 * information.
 */
/**
 * OpenHD 视频流传输的主入口点，用于空中单元上发现的摄像头。注意：
 * 摄像头和摄像头设置仅在空中单元本地，地面单元不需要了解这些 -
 * 它只是“傻乎乎地”转发接收到的视频数据。
 * 因此，我们只在空中单元上创建此类的实例。有关更多信息，请参见 Readme.md 和
 * camerastream.h。
 */
class OHDVideoAir {
   public:
    /**
     * Creates a video stream for each of the discovered cameras given in @param
     * cameras. You have to provide at least one camera - if there is no camera
     * found, use a dummy camera.
     * @param opt_action_handler openhd global handler for communication between
     * different ohd modules.
     * @param link_handle handle for sending video data over the (currently only
     * wb) link between air and ground
     */
    /**
     * 为 @param cameras
     * 中提供的每个发现的摄像头创建一个视频流。您必须至少提供一个摄像头 -
     * 如果没有找到摄像头，请使用虚拟摄像头。
     * @param opt_action_handler openhd 全局处理器，用于不同 ohd 模块之间的通信。
     * @param link_handle 用于通过（目前仅有的）wb
     * 链接在空中单元和地面单元之间发送视频数据的句柄。
     */
    OHDVideoAir(std::vector<XCamera> cameras, std::shared_ptr<OHDLink> link_handle);
    ~OHDVideoAir();
    OHDVideoAir(const OHDVideoAir&) = delete;
    OHDVideoAir(const OHDVideoAir&&) = delete;
    static std::vector<XCamera> discover_cameras();
    /**
     * In ohd-telemetry, we create a mavlink settings component for each of the
     * camera(s),instead of using one generic settings component like for the rest
     * of the settings. Get all the settings for the discovered cameras. Settings
     * for Camera0 are the first element, settings for camera1 the second
     */
    /**
     * 在 ohd-telemetry 中，我们为每个摄像头创建一个 mavlink
     * 设置组件，而不是像其他设置那样使用一个通用的设置组件。
     * 获取所有已发现摄像头的设置。Camera0 的设置是第一个元素，camera1
     * 的设置是第二个元素。
     */
    std::array<std::vector<openhd::Setting>, 2> get_all_camera_settings();
    std::vector<openhd::Setting> get_generic_settings();
    // r.n limited to primary and secondary camera
    // r.n 限制为主摄像头和副摄像头
    static constexpr auto MAX_N_CAMERAS = 2;
    void update_arming_state(bool armed);

   private:
    // All the created camera streams
    // 所有创建的相机流
    std::vector<std::shared_ptr<CameraStream>> m_camera_streams;
    std::shared_ptr<GstAudioStream> m_audio_stream;
    std::shared_ptr<spdlog::logger> m_console;
    std::shared_ptr<OHDLink> m_link_handle;
    // r.n only for multi camera support
    // r.n 仅用于多摄像头支持
    std::unique_ptr<AirCameraGenericSettingsHolder> m_generic_settings;

   private:
    // Add a CameraStream for a discovered camera.
    // 为已发现的摄像头添加一个 CameraStream。
    void configure(const std::shared_ptr<CameraHolder>& camera);
    // propagate a bitrate change request to the CameraStream implementation(s)
    // 将比特率更改请求传播到 CameraStream 实现。
    void handle_change_bitrate_request(openhd::LinkActionHandler::LinkBitrateInformation lb);
    // Called every time an encoded frame was generated
    //   每次生成编码帧时调用。
    void on_video_data(int stream_index, const openhd::FragmentedVideoFrame& fragmented_video_frame);
    void on_audio_data(const openhd::AudioPacket& audioPacket);
    // NOTE: On air, by default, we do not forward video via UDP to save precious
    // cpu time - but we allow user(s) to connect to the air unit via mavlink TCP
    // directly, in which case we start forwarding of video data to the device.
    // 注意：在空中单元，默认情况下我们不通过 UDP 转发视频以节省宝贵的 CPU 时间 -
    // 但我们允许用户通过 mavlink TCP
    // 直接连接到空中单元，在这种情况下，我们开始将视频数据转发到设备。
    void start_stop_forwarding_external_device(openhd::ExternalDevice external_device, bool connected);
    std::unique_ptr<openhd::UDPMultiForwarder> m_primary_video_forwarder = nullptr;
    std::unique_ptr<openhd::UDPMultiForwarder> m_secondary_video_forwarder = nullptr;
    std::unique_ptr<openhd::UDPMultiForwarder> m_audio_forwarder = nullptr;
    // Optimization for 0 overhead on air when not enabled
    // 在未启用时，为了实现零开销的空中优化
    std::atomic_bool m_has_localhost_forwarding_enabled = false;
    bool x_set_camera_type(bool primary, int cam_type);
};

#endif  // OPENHD_VIDEO_OHDVIDEO_H
