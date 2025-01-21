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

#include <camera_discovery.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "nalu/fragment_helper.h"
#include "ohd_video_air.h"
#include "openhd_bitrate.h"   //in ohd_common包
#include "openhd_link.hpp"    //in ohd_common包
#include "openhd_platform.h"  //in ohd_common包
#include "openhd_profile.h"   //in ohd_common包
#include "openhd_util.h"      //in ohd_common包

//
// Can be used to test / validate a camera implementation.
// R.n prints info about the received frame(s) to stdout.
// (See DummyDebugLink)
//
// 可用于测试/验证相机实现。
// R.n 将接收到的帧信息打印到标准输出。
// （参见 DummyDebugLink）

int main(int argc, char* argv[]) {
  // We need root to read / write camera settings.
  // 检查执行用户权限是否为root
  OHDUtil::terminate_if_not_root();

  // 获取当前运行平台信息
  const auto platform = OHDPlatform::instance();

  // 读取相机配置文件，并且返回相机列表（相机类型、ID）
  auto cameras = OHDVideoAir::discover_cameras();

  // 开启比特率debug
  openhd::BitrateDebugger bitrate_debugger{"Bitrate", true};

  auto forwarder = openhd::UDPForwarder("127.0.0.1", 5600);

  // 声明一个 Lambda 表达式，cb 作为回调函数被定义。
  // int stream_index 表示流的索引，
  // const openhd::FragmentedVideoFrame& fragmented_video_frame 是一个
  // FragmentedVideoFrame 类型的常量引用，表示分段的视频帧数据。
  auto cb = [&forwarder, &bitrate_debugger](
                int stream_index,
                const openhd::FragmentedVideoFrame& fragmented_video_frame) {
    // 定义了一个变量来累计视频数据的总大小。
    int total_size = 0;
    // 遍历 fragmented_video_frame 中的 RTP 数据包片段（rtp_fragments
    // 是一个包含所有片段的集合）。
    for (auto& fragemnt : fragmented_video_frame.rtp_fragments) {
      // 将片段的数据通过 UDP 转发。
      forwarder.forwardPacketViaUDP(fragemnt->data(), fragemnt->size());
      // 累加每个片段的大小到 total_size。
      total_size += fragemnt->size();
    }
    // 检查是否存在“脏帧”。
    if (fragmented_video_frame.dirty_frame) {
      // 通过 make_fragments 函数，将脏帧数据分割成多个片段。
      auto fragments =
          make_fragments(fragmented_video_frame.dirty_frame->data(),
                         fragmented_video_frame.dirty_frame->size());
      //  将脏帧的每个片段数据通过 UDP 转发，并累加其大小。
      for (auto& fragment : fragments) {
        forwarder.forwardPacketViaUDP(fragment->data(), fragment->size());
        total_size += fragment->size();
      }
    }
    // openhd::log::get_default()->debug("total size:{}", total_size);
    // 调用 bitrate_debugger 的 on_packet 方法，传递当前视频数据的总大小。
    // bitrate_debugger 用于监控和记录比特率，on_packet 方法通常会记录这次处理的总数据量，用于后续的带宽或性能分析。
    bitrate_debugger.on_packet(total_size);
  };

  auto debug_link = std::make_shared<DummyDebugLink>();
  debug_link->m_opt_frame_cb = cb;
  OHDVideoAir ohdVideo(cameras, debug_link);
  std::cout << "OHDVideo started\n";
  OHDUtil::keep_alive_until_sigterm();
  std::cerr << "OHDVideo stopped\n";
  return 0;
}
