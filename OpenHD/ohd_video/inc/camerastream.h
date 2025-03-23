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

#ifndef CAMERASTREAM_H
#define CAMERASTREAM_H

#include <string>
#include <vector>

#include "camera_holder.h"
#include "openhd_action_handler.h"
#include "openhd_platform.h"
#include "openhd_video_frame.h"

/**
 * Every camera stream should inherit from this class.
 * This hides away the underlying implementation (for example gstreamer,...) for
 * different platform(s). The paradigms developers should aim for with each
 * camera stream are:
 * 1) Once an instance is created, it will start generating video data, already
 * encoded and packetized with respect to the link MTU. RTP MUST be used for
 * packetization (at least for now) 2) If the camera disconnects or the
 * underlying process crashes (for whatever reason) the underlying
 * implementation should re-start the camera and encoding process
 * 3) If the user changes camera parameters, it should store these changes
 * locally (such that they are also set after the next re-start) and apply the
 * changes. It is no problem to just restart the underlying camera/encoding
 * process with the new parameters. 4) The implementation(s) should handle the
 * differences between camera(s) in regards to supported and not supported
 * parameters
 *
 * Video streaming in OpenHD is always unidirectional and lossy (FEC). However,
 * this is done by the link implementation - here we only generate encoded data
 * and packetize it into rtp fragments, then forward it.
 */

/**
 * 每个摄像头流都应继承自此类。
 * 该类隐藏了底层实现（例如 GStreamer 等）对不同平台的支持。
 * 开发者在使用每个摄像头流时应遵循的基本准则是：
 * 1) 一旦实例化，它将开始生成视频数据，数据已经编码并根据链接的 MTU进行分包。RTP 必须用于分包（至少现在是如此）。
 * 2) 如果摄像头断开连接或底层进程崩溃（无论是什么原因），底层实现应重新启动摄像头和编码过程。
 * 3) 如果用户更改摄像头参数，应该将这些更改本地保存（确保下次重启后也能恢复这些更改），并应用这些更改。重新启动底层摄像头/编码过程并应用新参数是没有问题的。
 * 4) 实现应处理摄像头之间在支持与不支持参数上的差异。
 *
 * 在 OpenHD中，视频流始终是单向的并且是有损的（FEC）。然而，这部分由链接实现处理——在这里，我们只生成已编码的数据并将其打包成RTP 数据包，然后转发。
 */

class CameraStream {
   public:
    /**
     * After a camera stream is constructed, it won't start streaming until
     * setup() and start() are called
     * @param platform the platform we are running on
     * @param camera_holder the camera to create the stream with, camera_holder
     * provides access to the camera (capabilities) and settings.
     * @param i_transmit abstract interface where encoded video data is forwarded
     * to (was UDP port previously)
     */
    /**
     * 在构造摄像头流后，直到调用 setup() 和 start()方法之前，摄像头流不会开始传输。
     * @param platform 当前运行的设备平台
     * @param camera_holder 用于创建流的摄像头，camera_holder提供对摄像头（功能）和设置的访问。
     * @param i_transmit 编码后的视频数据转发的抽象接口（以前是 UDP 端口）
     */
    CameraStream(std::shared_ptr<CameraHolder> camera_holder, openhd::ON_ENCODE_FRAME_CB out_cb);
    CameraStream(const CameraStream&) = delete;
    CameraStream(const CameraStream&&) = delete;

    // after start_looping is called the camera should start streaming (generating
    // video data) as soon as possible terminate_loping() is called when openhd
    // terminates (only for development) The camera is responsible to implement
    // its loop thread such that it can react to setting changes
    // 在调用 start_looping() 后，摄像头应尽快开始流式传输（生成视频数据）
    // terminate_loping() 在 OpenHD 终止时调用（仅用于开发）
    // 摄像头负责实现其循环线程，以便能够响应设置的更改
    virtual void start_looping() = 0;
    virtual void terminate_looping() = 0;

    /**
     * Handle a change in the bitrate, most likely requested by the RF link.
     * This is the only value an implementation should support changing without a
     * complete restart of the pipeline / stream. It is okay to not implement this
     * interface method properly, e.g leave it empty.
     */
    /**
     * 处理比特率的变化，这通常是由 RF 链路请求的。
     * 这是实现应该支持的唯一无需完全重启管道/流的变化值。
     * 可以不正确实现此接口方法，例如可以留空。
     */
    virtual void handle_change_bitrate_request(openhd::LinkActionHandler::LinkBitrateInformation lb) = 0;

    /**
     * Handle a change in the arming state
     * We have air video recording depending on the arming state, but the setting
     * and implementation is camera specific. It is okay to not implement this
     * interface method properly, e.g leave it empty.
     */
    /**
     * 处理武装状态的变化
     * 我们根据武装状态进行空中视频录制，但该设置和实现是特定于摄像头的。
     * 可以不正确实现此接口方法，例如可以留空。
     */
    virtual void handle_update_arming_state(bool armed) = 0;

   public:
    std::shared_ptr<CameraHolder> m_camera_holder;
    static constexpr auto CAM_STATUS_STREAMING = 1;
    static constexpr auto CAM_STATUS_RESTARTING = 2;

   protected:
    openhd::ON_ENCODE_FRAME_CB m_output_cb;
};

#endif
