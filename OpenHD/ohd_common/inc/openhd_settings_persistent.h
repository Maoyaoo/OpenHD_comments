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

#ifndef OPENHD_OPENHD_OHD_COMMON_OPENHD_SETTINGS_PERSISTENT_HPP_
#define OPENHD_OPENHD_OHD_COMMON_OPENHD_SETTINGS_PERSISTENT_HPP_

#include <cassert>
#include <fstream>
#include <functional>
#include <utility>

#include "openhd_spdlog.h"
#include "openhd_util_filesystem.h"

/**
 * In general, all OpenHD modules (e.g. video, telemetry, interface) handle
 * their settings completely independently by writing and reading json files.
 */
// 一般来说，所有 OpenHD 模块（例如视频、遥测、接口）通过读写 JSON
// 文件，完全独立地处理它们的设置。
namespace openhd {

/**
 * Helper class to persist settings during reboots (impl is using most likely
 * json in OpenHD). Properly handles the typical edge cases, e.g. a) No settings
 * have been stored for the given unique filename (e.g. for camera of type X) =>
 * create default settings. b) The user/developer manually wrote values of the
 * wrong type into the json file => delete invalid settings, create default.
 * This class is a bit hard to understand, I'd recommend just looking up one of
 * the implementations to understand it.
 * @tparam T the settings struct to persist
 */
/* 辅助类，用于在重启期间持久化设置（实现可能在 OpenHD 中使用 JSON）
正确处理典型的边界情况，例如：
a) 对于给定的唯一文件名（例如类型为 X 的摄像头），没有存储任何设置 =>
创建默认设置。 b) 用户/开发者手动将错误类型的值写入 JSON 文件 =>
删除无效设置，创建默认设置。 这个类有点难理解，建议查看其中一个实现来理解它。
@tparam T 要持久化的设置结构体 */

template <class T>
class PersistentSettings {
 public:
  /**
   * @param base_path the directory into which the settings file is then
   * written. (filename: base_path+unique filename).
   */
  /**
   * @param base_path 设置文件将被写入的目录。（文件名：base_path+唯一文件名）。
   */
  explicit PersistentSettings(std::string base_path)
      : _base_path(std::move(base_path)) {
    assert(_base_path.back() == '/');
  }
  // delete copy and move constructor
  PersistentSettings(const PersistentSettings&) = delete;
  PersistentSettings(const PersistentSettings&&) = delete;

  /**
   * read only, to express the need for calling persist otherwise
   */
  [[nodiscard]] const T& get_settings() const {
    assert(_settings);
    return *_settings;
  }
  /**
   * Don't forget to call persist once done modifying
   * 修改完成后别忘了调用 persist。
   * @return
   */
  [[nodiscard]] T& unsafe_get_settings() const {
    assert(_settings);
    return *_settings;
  }
  // save changes by writing them out to the file, and notifying the listener cb
  // if there is any
  // 通过将更改写入文件并在有监听器回调时通知其来保存更改
  void persist(bool trigger_restart = true) const {
    PersistentSettings::persist_settings();
    if (_settings_changed_callback && trigger_restart) {
      _settings_changed_callback();
    }
  }
  // Persist then new settings, then call the callback to propagate the change
  // 持久化新的设置，然后调用回调以传播更改。
  void update_settings(const T& new_settings) {
    openhd::log::debug_log("Got new settings in [" + get_unique_filename() +
                           "]");
    _settings = std::make_unique<T>(new_settings);
    PersistentSettings::persist_settings();
    if (_settings_changed_callback) {
      _settings_changed_callback();
    }
  }

  typedef std::function<void()> SETTINGS_CHANGED_CALLBACK;
  void register_listener(SETTINGS_CHANGED_CALLBACK callback) {
    assert(!_settings_changed_callback);
    _settings_changed_callback = std::move(callback);
  }
  /*
   * looks for a previously written file (base_path+unique filename).
   * If this file exists, create settings from it - otherwise, create default
   * and persist.
   */
  // 查找以前写入的文件（base_path+唯一文件名）。
  // 如果该文件存在，则从中创建设置 - 否则，创建默认设置并持久化。
  void init() {
    if (!OHDFilesystemUtil::exists(_base_path)) {
      OHDFilesystemUtil::create_directory(_base_path);
    }
    const auto last_settings_opt = read_last_settings();
    if (last_settings_opt.has_value()) {
      _settings = std::make_unique<T>(last_settings_opt.value());
      openhd::log::info_log("Using settings in [" + get_file_path() + "]");
    } else {
      openhd::log::info_log("Creating default settings in [" + get_file_path() +
                            "]");
      // create default settings and persist them for the next reboot
      // 创建默认设置并持久化，以便下次重启时使用。
      _settings = std::make_unique<T>(create_default());
      persist_settings();  // 将设置或配置数据保存到持久存储
    }
  }

 protected:
  // NEEDS TO BE OVERRIDDEN
  [[nodiscard]] virtual std::string get_unique_filename() const = 0;
  virtual T create_default() const = 0;
  virtual std::optional<T> impl_deserialize(
      const std::string& file_as_string) const = 0;            // 反序列化
  virtual std::string imp_serialize(const T& data) const = 0;  // 序列化

 private:
  const std::string _base_path;
  std::unique_ptr<T> _settings;
  SETTINGS_CHANGED_CALLBACK _settings_changed_callback = nullptr;
  // 获取文件路径
  [[nodiscard]] std::string get_file_path() const {
    return _base_path + get_unique_filename();
  }
  /**
   * serialize settings to json and write to file for persistence
   * 将设置序列化为 JSON 并写入文件以实现持久化。
   */
  void persist_settings() const {
    assert(_settings);
    const auto file_path = get_file_path();
    // Serialize, then write to file
    const auto content = imp_serialize(*_settings);
    OHDFilesystemUtil::write_file(file_path, content);
  }
  /**
   * Try and deserialize the last stored settings (json)
   * Return std::nullopt if
   * 1) The file does not exist
   *  2) The json parse encountered an error
   *  3) The json conversion encountered an error
   *  In case of 1 this is most likely new hw, and default settings will be
   * created. In case of 2,3 it was most likely a user that modified the json
   * incorrectly Also, default settings will be created in this case.
   */
  /**
   * 尝试反序列化最后存储的设置（JSON）
   * 如果满足以下条件之一，则返回 std::nullopt：
   * 1、文件不存在
   * 2、JSON 解析遇到错误
   * 3、JSON 转换遇到错误
   * 对于第 1 种情况，这很可能是新的硬件，将创建默认设置
   * 对于第 2 和 3 种情况，很可能是用户错误地修改了 JSON 文件。
   * 在这种情况下，也会创建默认设置。
   */
  [[nodiscard]] std::optional<T> read_last_settings() const {
    const auto file_path = get_file_path();
    const auto opt_content = OHDFilesystemUtil::opt_read_file(file_path);
    if (!opt_content.has_value()) {
      return std::nullopt;
    }
    const std::string content = opt_content.value();
    const auto parsed_opt = impl_deserialize(content);
    return parsed_opt;
  }
};

}  // namespace openhd

#endif  // OPENHD_OPENHD_OHD_COMMON_OPENHD_SETTINGS_PERSISTENT_HPP_
