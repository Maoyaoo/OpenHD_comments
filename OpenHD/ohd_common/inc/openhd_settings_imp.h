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

#ifndef OPENHD_OPENHD_OHD_COMMON_MAVLINK_SETTINGS_ISETTINGSCOMPONENT_H_
#define OPENHD_OPENHD_OHD_COMMON_MAVLINK_SETTINGS_ISETTINGSCOMPONENT_H_

#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace openhd {

// Util - we have a default impl. that 'does nothing' but prints a message
std::function<bool(std::string id, int requested_value)>
create_log_only_cb_int();
std::function<bool(std::string id, std::string requested_value)>
create_log_only_cb_string();

// int / string setting general layout:
// value:
// The value which the ground station (e.g. the user) can modify via mavlink
// after passing the implemented sanity checks (e.g. the value that is changed
// by the mavlink parameter provider when OpenHD returned true in the
// change_callback). change_callback: This callback is called every time the
// user wants to change the parameter (T value) from value x to value y (via
// mavlink) return true to accept the value, otherwise return false. We have a
// default implementation that just prints the change request and always returns
// true, mostly for debugging / testing. But in general, all OpenHD modules that
// are configurable overwrite this callback with their own proper
// implementation. get_callback: Quite dirty - all the params in openhd are
// changed by the user via mavlink only - Except channel frequency and channel
// width during the channel scan feature. Workaround for this rare case - don't
// ask ;)

// int / string 设置的一般布局：
// value：
// 该值是地面站（例如用户）可以通过 mavlink 修改的值，
// 在通过已实现的合理性检查后（例如，当 OpenHD 返回 true 时，
// mavlink 参数提供者会更改该值）。
// change_callback：每次用户希望通过 mavlink 将参数（T 值）从值 x
// 更改为值 y 时，都会调用此回调函数。返回 true 以接受该值，
// 否则返回 false。我们有一个默认实现，它仅打印更改请求并始终返回 true，
// 主要用于调试/测试。但通常，所有可以配置的 OpenHD 模块都会
// 用自己的实现覆盖此回调函数。
// get_callback：相当脏 —— 所有的 OpenHD 参数都是通过 mavlink 由用户修改的，
// 除了在通道扫描功能期间的频道频率和频道宽度。对于这种罕见情况的解决方法 ——
// 不要问 ;)

struct IntSetting {
  int value;
  // change_callback 是一个回调函数，接受一个 ID 和一个整数值，返回一个布尔值。
  // 默认值是由 `create_log_only_cb_int()` 创建的回调函数。
  std::function<bool(std::string id, int requested_value)> change_callback =
      create_log_only_cb_int();
  std::function<int()> get_callback = nullptr;
};

struct StringSetting {
  std::string value;
  // change_callback 是一个回调函数，接受一个 ID
  // 和一个字符串值，返回一个布尔值。
  // 默认值是由 `create_log_only_cb_string()` 创建的回调函数。
  std::function<bool(std::string id, std::string requested_value)>
      change_callback = create_log_only_cb_string();
  std::function<std::string()> get_callback = nullptr;
};

struct Setting {
  // Do not mutate me
  std::string id;  // 每个设置的唯一标识符

  // setting 是一个 std::variant 类型，可以存储 `IntSetting` 或 `StringSetting`
  // 类型的值。 这意味着每个 `Setting` 可以包含两种类型中的任意一种。
  std::variant<IntSetting, StringSetting> setting;
};

// we need to have unique setting string ids. Creating duplicates by accident is
// not uncommon when adding new settings, and when this function is used
// properly we can catch those mistakes at run time.
// 我们需要具有唯一的设置字符串 ID。在添加新设置时，意外创建重复项是很常见的，
// 当正确使用此函数时，我们可以在运行时捕捉到这些错误。
void validate_provided_ids(const std::vector<Setting>& settings);

static bool validate_yes_or_no(int value) { return value == 0 || value == 1; }

// Helper for creating read-only params- they can be usefully for debugging
// 用于创建只读参数的辅助工具——它们对于调试非常有用
Setting create_read_only_int(const std::string& id, int value);

// Creates a read - only parameter - we repurpose the mavlink param set for
// reliably showing more info to the user / developer. Can be quite nice for
// debugging. Since the n of characters are limited, this might cut away parts
// of value
// 创建只读参数——我们重新利用 mavlink 参数设置来
// 稳定地向用户/开发者显示更多信息。对于调试非常有用。
// 由于字符数有限，这可能会截断值的一部分。
Setting create_read_only_string(const std::string& id, std::string value);

// Helper function - adds a new int param that has an ID, an initial value,
// and a cb that is called when the value shall be changed by mavlink
// 辅助函数 - 添加一个新的 int 参数，包含 ID、初始值，
// 以及当 mavlink 需要更改该值时调用的回调函数（cb）
void append_int_param(std::vector<Setting>& ret, const std::string& ID,
                      int value,
                      const std::function<bool(int requested_value)>& cb);

namespace testing {
std::vector<Setting> create_dummy_camera_settings();
std::vector<Setting> create_dummy_ground_settings();
// A size of 0 creates issues with the param server, but it is possible we don't
// have any params if none were addable during run time due
// 大小为 0 会导致参数服务器出现问题，但如果在运行时没有添加任何参数，
// 可能就没有任何参数可用。
void append_dummy_if_empty(std::vector<Setting>& ret);
}  // namespace testing

}  // namespace openhd
#endif  // OPENHD_OPENHD_OHD_COMMON_MAVLINK_SETTINGS_ISETTINGSCOMPONENT_H_
