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

#ifndef OPENHD_TELEMETRY_AIRTELEMETRY_H
#define OPENHD_TELEMETRY_AIRTELEMETRY_H

#include <string>

#include "endpoints/SerialEndpoint.h"
#include "internal/OHDMainComponent.h"
#include "openhd_link_statistics.hpp"
#include "openhd_platform.h"
#include "openhd_settings_imp.h"
#include "routing/MavlinkSystem.hpp"
//
#include "AirTelemetrySettings.h"
#include "endpoints/TCPEndpoint.h"
#include "endpoints/WBEndpoint.h"
#include "gpio_control/RaspberryPiGPIOControl.h"
#include "mavsdk_temporary/XMavlinkParamProvider.h"
#include "openhd_action_handler.h"
#include "openhd_link.hpp"
#include "openhd_spdlog.h"

/**
 * OpenHD Air telemetry. Assumes a Ground instance running on the ground pi.
 */
class AirTelemetry : public MavlinkSystem {
 public:
  explicit AirTelemetry();
  AirTelemetry(const AirTelemetry&) = delete;
  AirTelemetry(const AirTelemetry&&) = delete;
  ~AirTelemetry();
  /**
   * Telemetry will run infinite in its own threads until terminate is set to
   * true
   * @param enableExtendedLogging be really verbose on logging.
   */
  /**
   * 遥测将在其自己的线程中无限运行，直到 `terminate` 被设置为 true。
   * @param enableExtendedLogging
   * 是否启用扩展日志记录，以提供非常详细的日志信息。
   */
  void loop_infinite(bool& terminate, bool enableExtendedLogging = false);

  /**
   * @return verbose string about the current state, for debugging
   * 关于当前状态的详细字符串，用于调试。
   */
  [[nodiscard]] std::string create_debug();

  /**
   * add settings to the generic mavlink parameter server
   * changes are propagated back through the settings instances
   * @param settings the settings to add
   */
  /**
   * 将设置添加到通用的 MAVLink 参数服务器。
   * 设置的更改会通过设置实例传播回去。
   * @param settings 要添加的设置。
   */
  void add_settings_generic(const std::vector<openhd::Setting>& settings);

  /**
   * must be called once all settings have been added, this is needed to avoid
   * an invariant parameter set
   * 必须在所有设置添加完成后调用一次，这是为了避免参数集的不变性。
   */
  void settings_generic_ready();

  /**
   * On the air unit we use mavlink to change camera settings. We have exactly
   * one mavlink param server per camera
   * @param camera_index 0 for primary camera, 1 for secondary camera, ...
   * @param settings the settings for this camera
   */
  /**
   * 在空中单元中，我们使用 mavlink 来更改摄像头设置。每个摄像头都有一个唯一的
   * mavlink 参数服务器。
   * @param camera_index 0 表示主摄像头，1 表示次摄像头，...
   * @param settings 该摄像头的设置
   */

  void add_settings_camera_component(
      int camera_index, const std::vector<openhd::Setting>& settings);
  /**
   * The link handle can be set later after instantiation - until it is set,
   * messages from/to the ground unit are just discarded.
   */
  /**
   * 链路句柄可以在实例化后设置——在设置之前，与地面单元的消息将被直接丢弃。
   */
  void set_link_handle(std::shared_ptr<OHDLink> link);

 private:
  // send a mavlink message to the flight controller connected to the air unit
  // via UART, if connected.
  // 通过UART将mavlink消息发送到连接到空中单元的飞行控制器（如果已连接）。
  void send_messages_fc(std::vector<MavlinkMessage>& messages);

  // send mavlink messages to the ground unit, lossy
  // 发送mavlink消息到地面单元，可能会丢失
  void send_messages_ground_unit(std::vector<MavlinkMessage>& messages);

  // called every time one or more messages from the flight controller are
  // received
  // 每次接收到来自飞控的一个或多个消息时调用
  void on_messages_fc(std::vector<MavlinkMessage>& messages);

  // called every time one or more messages from the ground unit are received
  // 每次接收到来自地面单元的一个或多个消息时调用
  void on_messages_ground_unit(std::vector<MavlinkMessage>& messages);

  // R.N only on air, and only FC uart settings
  // 目前仅在空中单元（air unit）中有效，并且仅适用于飞控（FC）UART设置
  std::vector<openhd::Setting> get_all_settings();
  void setup_uart();

 private:
  std::unique_ptr<openhd::telemetry::air::SettingsHolder> m_air_settings;
  std::unique_ptr<SerialEndpointManager> m_fc_serial;
  // send/receive data via wb
  std::unique_ptr<WBEndpoint> m_wb_endpoint;
  // shared because we also push it onto our components list
  std::shared_ptr<OHDMainComponent> m_ohd_main_component;
  std::mutex m_components_lock;
  std::vector<std::shared_ptr<MavlinkComponent>> m_components;
  std::shared_ptr<XMavlinkParamProvider> m_generic_mavlink_param_provider;
  // rpi only, allow changing gpios via settings
  std::unique_ptr<openhd::telemetry::rpi::GPIOControl> m_opt_gpio_control =
      nullptr;
  std::shared_ptr<spdlog::logger> m_console;
  // EXP - always on TCP mavlink server
  std::unique_ptr<TCPEndpoint> m_tcp_server = nullptr;
};

#endif  // OPENHD_TELEMETRY_AIRTELEMETRY_H
