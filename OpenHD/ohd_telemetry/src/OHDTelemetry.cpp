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

#include "OHDTelemetry.h"

#include "AirTelemetry.h"
#include "GroundTelemetry.h"

OHDTelemetry::OHDTelemetry(OHDProfile profile1, bool enableExtendedLogging) : m_profile(std::move(profile1)), m_enableExtendedLogging(enableExtendedLogging) {
    if (this->m_profile.is_air) {
        m_air_telemetry = std::make_unique<AirTelemetry>();
        assert(m_air_telemetry);

        // 创建一个新线程，并将其指针存储在 m_loop_thread 中。
        m_loop_thread = std::make_unique<std::thread>([this] {
            assert(m_air_telemetry);
            m_air_telemetry->loop_infinite(m_loop_thread_terminate, this->m_enableExtendedLogging);
        });
    } else {
        m_ground_telemetry = std::make_unique<GroundTelemetry>();
        assert(m_ground_telemetry);
        m_loop_thread = std::make_unique<std::thread>([this] {
            assert(m_ground_telemetry);
            m_ground_telemetry->loop_infinite(m_loop_thread_terminate, this->m_enableExtendedLogging);
        });
    }
}

OHDTelemetry::~OHDTelemetry() {
    m_loop_thread_terminate = true;
    m_loop_thread->join();
}

std::string OHDTelemetry::createDebug() const {
    if (m_profile.is_air) {
        return m_air_telemetry->create_debug();
    } else {
        return m_ground_telemetry->create_debug();
    }
}

void OHDTelemetry::add_settings_generic(const std::vector<openhd::Setting>& settings) const {
    if (m_profile.is_air) {
        m_air_telemetry->add_settings_generic(settings);
    } else {
        m_ground_telemetry->add_settings_generic(settings);
    }
}
void OHDTelemetry::settings_generic_ready() const {
    if (m_profile.is_air) {
        m_air_telemetry->settings_generic_ready();
    } else {
        m_ground_telemetry->settings_generic_ready();
    }
}
void OHDTelemetry::add_settings_camera_component(int camera_index, const std::vector<openhd::Setting>& settings) const {
    // we only have cameras on the air telemetry unit
    assert(m_profile.is_air);
    // only 2 cameras suported for now.
    m_air_telemetry->add_settings_camera_component(camera_index, settings);
}

void OHDTelemetry::set_link_handle(std::shared_ptr<OHDLink> link) {
    if (link == nullptr) {
        openhd::log::get_default()->warn("set_link_handle - no link available");
        return;
    }
    if (m_profile.is_air) {
        m_air_telemetry->set_link_handle(link);
    } else {
        m_ground_telemetry->set_link_handle(link);
    }
}