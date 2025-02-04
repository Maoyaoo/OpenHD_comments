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

#ifndef OPENHD_OHDTELEMETRY_H
#define OPENHD_OHDTELEMETRY_H

#include <memory>
#include <thread>
#include <utility>

#include "openhd_action_handler.h"
#include "openhd_external_device.h"
#include "openhd_link.hpp"
#include "openhd_link_statistics.hpp"
#include "openhd_platform.h"
#include "openhd_profile.h"
#include "openhd_settings_imp.h"

// Forward declare them to speed up compilation time.
// 提前声明它们以加快编译时间。
class AirTelemetry;
class GroundTelemetry;

/**
 * This class holds either a Air telemetry or Ground Telemetry instance.
 */
/**
 * 此类包含一个空中遥测实例或地面遥测实例。
 */
class OHDTelemetry {
   public:
    OHDTelemetry(OHDProfile profile1, bool enableExtendedLogging = false);
    OHDTelemetry(const OHDTelemetry&) = delete;
    OHDTelemetry(const OHDTelemetry&&) = delete;
    ~OHDTelemetry();
    [[nodiscard]] std::string createDebug() const;

    // Settings and statistics. Other modules (e.g. video, interface) use the
    // mavlink settings provided by OHD Telemetry. However, we do not have code
    // dependencies directly between these modules, to allow independent testing
    // without telemetry and to keep the functionalities seperated. All modules
    // other than camera share the same settings component for now. Note that the
    // settings are still experiencing changes / are not finalized, e.g. we might
    // introduce different settings components for different OHD modules if
    // viable.
    // 设置和统计信息。其他模块（例如视频、接口）使用由 OHD 遥测提供的 MAVLink 设置。
    // 然而，这些模块之间没有直接的代码依赖关系，以便在没有遥测的情况下进行独立测试，
    // 并保持功能的分离。目前，除摄像头外的所有模块共享相同的设置组件。
    // 请注意，设置仍在经历变化 / 尚未最终确定，例如如果可行，
    // 我们可能会为不同的 OHD 模块引入不同的设置组件。
    void add_settings_generic(const std::vector<openhd::Setting>& settings) const;

    // This is confusing, but there is no way around (keyword: invariant
    // settings), since we add the settings one at a time as we create the other
    // modules (e.g. interface, video) sequentially one at a time in the OHD
    // main.cpp file. Note that without calling this function, no ground station
    // will see any settings, even though they are already added.
    // 这有些令人困惑，但别无他法（关键词：不变设置），
    // 因为我们在 OHD 的 main.cpp 文件中逐个创建其他模块（例如接口、视频）时，
    // 会逐个添加设置。请注意，如果不调用此函数，地面站将看不到任何设置，
    // 即使它们已经被添加。
    void settings_generic_ready() const;

    // Cameras get their own component ID, other than the "rest" which shares the
    // same component id for simplicity. Note, at some point it might make sense
    // to also use its own component id for OHD interface
    // 摄像头拥有自己的组件 ID，而其他模块为了简化共享相同的组件 ID。
    // 注意，在某些情况下，为 OHD 接口使用自己的组件 ID 可能也是有意义的。
    void add_settings_camera_component(int camera_index, const std::vector<openhd::Setting>& settings) const;

    // OHDTelemetry is agnostic of the type of transmission between air and ground
    // and also agnostic weather this link exists or not (since it is already
    // using a lossy link).
    // OHDTelemetry 对空中和地面之间的传输类型是无感知的，
    // 并且对是否存在这种链接也是无感知的（因为它已经是一种有损的链接）。
    void set_link_handle(std::shared_ptr<OHDLink> link);

   private:
    // only either one of them both is active at a time.
    // active when air
    std::unique_ptr<AirTelemetry> m_air_telemetry;
    // active when ground
    std::unique_ptr<GroundTelemetry> m_ground_telemetry;
    // Main telemetry thread. Note that the endpoints also might have their own
    // Receive threads
    // 主遥测线程。请注意，端点也可能有自己的接收线程
    std::unique_ptr<std::thread> m_loop_thread;
    bool m_loop_thread_terminate = false;
    const OHDProfile m_profile;
    const bool m_enableExtendedLogging;
};

#endif  // OPENHD_OHDTELEMETRY_H
