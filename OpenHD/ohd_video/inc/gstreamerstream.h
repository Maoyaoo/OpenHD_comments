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

#ifndef GSTREAMERSTREAM_H
#define GSTREAMERSTREAM_H

#include <gst/gst.h>

#include <array>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "camera_settings.hpp"
#include "camerastream.h"
#include "gst_bitrate_controll_wrapper.hpp"
#include "openhd_platform.h"
#include "openhd_spdlog.h"
// #include "gst_recorder.h"
#include "nalu/CodecConfigFinder.hpp"
#include "openhd_rtp.h"

// Implementation of OHD CameraStream for pretty much everything, using
// gstreamer.
// NOTE: What we are doing here essentially is creating a big gstreamer pipeline
// string and then executing this pipeline. This makes development easy (since
// you can just test the pipeline(s) manually using gst-launch and add settings
// and more this way) but you are encouraged to use other approach(es) if they
// better fit your needs (see CameraStream.h)
// 使用 gstreamer 实现 OHD CameraStream，几乎适用于所有情况。
// 注意：我们在这里所做的基本上是创建一个大的 gstreamer 管道字符串，然后执行该管道。
// 这样做使得开发变得容易（因为你可以使用 gst-launch 手动测试管道，并通过这种方式添加设置等），
// 但如果其他方法更适合你的需求，建议使用其他方法（参见 CameraStream.h）。
class GStreamerStream : public CameraStream {
   public:
    GStreamerStream(std::shared_ptr<CameraHolder> camera_holder, openhd::ON_ENCODE_FRAME_CB out_cb);
    ~GStreamerStream();
    void start_looping() override;
    void terminate_looping() override;

   private:
    // Creates a valid gstreamer pipeline for the given camera,
    // including the source and encoder, not including appsink
    // 为给定的摄像头创建一个有效的 GStreamer 管道，
    // 包括源和编码器，但不包括 appsink。
    std::string create_source_encode_pipeline(const CameraHolder& cam_holder);
    void setup();
    // Set gst state to PLAYING
    void start();
    // Set gst state to PAUSED
    void stop();
    // Set gst state to GST_STATE_NULL and properly cleanup the pipeline.
    void cleanup_pipe();
    void handle_change_bitrate_request(openhd::LinkActionHandler::LinkBitrateInformation lb) override;
    // this is called when the FC reports itself as armed / disarmed
    // 当飞控报告其为已武装/未武装时调用此函数
    void handle_update_arming_state(bool armed) override;
    void loop_infinite();
    void stream_once();
    // To reduce the time on the param callback(s) - they need to return
    // immediately to not block the param server
    // 为了减少参数回调的时间——它们需要立即返回，以避免阻塞参数服务器
    void request_restart();

   private:
    // points to a running gst pipeline instance
    // 指向正在运行的 GStreamer 管道实例
    GstElement* m_gst_pipeline = nullptr;

    // pull samples (fragments) out of the gstreamer pipeline
    // 指向正在运行的 GStreamer 管道实例
    GstElement* m_app_sink_element = nullptr;

    // not supported by all camera(s).
    // for dynamically changing the bitrate
    // 并非所有摄像头都支持此功能。
    // 用于动态改变比特率
    std::optional<GstBitrateControlElement> m_bitrate_ctrl_element = std::nullopt;

    // If a pipeline is started with air recording enabled, the file name the
    // recording is written to is stored here otherwise, it is set to std::nullopt
    // 如果以启用空中录制的方式启动管道，录制文件的文件名将存储在此处；
    // 否则，设置为 std::nullopt。
    std::optional<std::string> m_opt_curr_recording_filename = std::nullopt;
    std::shared_ptr<spdlog::logger> m_console;

    // Set to true if armed, used for auto record on arm
    // 如果已武装则设置为 true，用于在武装时自动录制
    bool m_armed_enable_air_recording = false;
    std::atomic<int> m_curr_dynamic_bitrate_kbits = -1;

    // Not working yet, keep the old approach
    // 还未实现，保留旧的做法
    // std::unique_ptr<GstVideoRecorder> m_gst_video_recorder=nullptr;
    std::atomic_bool m_request_restart = false;
    std::atomic_bool m_keep_looping = false;
    std::unique_ptr<std::thread> m_loop_thread = nullptr;

   private:
    // The stuff here is to pull the data out of the gstreamer pipeline, such that
    // we can forward it to the WB link
    // 这里的内容是为了从 GStreamer 管道中提取数据，以便
    // 我们可以将其转发到 WB 链接
    void on_new_rtp_frame_fragment(std::shared_ptr<std::vector<uint8_t>> fragment, uint64_t dts);
    void on_new_rtp_fragmented_frame();
    std::vector<std::shared_ptr<std::vector<uint8_t>>> m_frame_fragments;

    void x_on_new_rtp_fragmented_frame(std::vector<std::shared_ptr<std::vector<uint8_t>>> frame_fragments);
    bool m_last_fu_s_idr = false;
    bool dirty_use_raw = false;
    std::chrono::steady_clock::time_point m_last_log_streaming_disabled = std::chrono::steady_clock::now();

   private:
    std::shared_ptr<openhd::RTPHelper> m_rtp_helper;
};

#endif
