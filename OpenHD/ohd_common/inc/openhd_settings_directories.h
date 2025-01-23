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

#ifndef OPENHD_SETTINGS_H
#define OPENHD_SETTINGS_H

#include <string>

namespace openhd {

// from
// https://superuser.com/questions/631859/preferred-place-to-store-configuration-files-that-change-often
// All persistent settings are written into this directory.
// Jan 28 / v2.3.1 : A lot of (rpi) users complained that they cannot change
// settings manually anymore. Even though this is not recommended, we want to
// support that - and since on rpi image only /boot shows up under windows in
// the file reader, we had to change the path in this regard. Shouldn't create
// any issues on linux, since we are root, we can just cretae the directory at
// run time
// !!!! Had to be reverted - writing to /boot on rpi is too prone to file system
// corruption !!!
// static constexpr auto SETTINGS_BASE_PATH ="/boot/openhd/settings/";
// 来源：
// https://superuser.com/questions/631859/preferred-place-to-store-configuration-files-that-change-often
// 所有持久化设置都会写入这个目录。
// 1月28日 / v2.3.1版本：很多（树莓派）用户反馈他们无法再手动更改设置。
// 尽管不建议这样做，但我们还是想支持这一需求。
// 由于在树莓派镜像中，只有 /boot 目录能在Windows的文件阅读器中显示，所以我们不得不更改此处的路径。
// 在Linux系统上应该不会产生任何问题，因为我们以root权限运行，可以在运行时直接创建该目录。
// !!!! 此更改必须撤销 —— 在树莓派上向 /boot 目录写入数据极易导致文件系统损坏！！！
// static constexpr auto SETTINGS_BASE_PATH ="/boot/openhd/settings/";
static constexpr auto SETTINGS_BASE_PATH = "/usr/local/share/openhd/";
// for example, the unique id
static std::string get_unit_id_file_path() {
    return std::string(SETTINGS_BASE_PATH) + "unit.id";
}

// Interface, telemetry and video each have their own directory for settings
// to separate them logically like also done in code
// 接口、遥测和视频各自拥有用于存放设置的独立目录，以便像代码中那样在逻辑上对它们进行分离。
static std::string get_interface_settings_directory() {
    return std::string(SETTINGS_BASE_PATH) + "interface/";
}
static std::string get_telemetry_settings_directory() {
    return std::string(SETTINGS_BASE_PATH) + "telemetry/";
}
static std::string get_video_settings_directory() {
    return std::string(SETTINGS_BASE_PATH) + "video/";
}

/**
 * If the directory does not exist yet,
 * generate the directory where all persistent settings of OpenHD are stored.
 */
/**
 * 如果该目录尚不存在，
 * 则创建用于存储OpenHD所有持久化设置的目录。
 */
void generateSettingsDirectoryIfNonExists();

// fucking boost, random bugged on allwinner. This is a temporary solution
// 该死的Boost库，在全志（Allwinner）芯片上随机出问题。这是个临时解决方案。
static std::string create_unit_it_temporary() {
    return "01234566789";
}

/**
 * If no unit id file exists, this is the first boot of this OpenHD image on the
 * platform. In this case, generate a new random unit id, and store it
 * persistently. Then return the unit id. If a unit id file already exists, read
 * and return the unit id.
 * @return the unit id, it doesn't change during reboots of the same system.
 */
/**
 * 如果设备ID文件不存在，说明这是该OpenHD镜像在平台上的首次启动。
 * 这种情况下，生成一个新的随机设备ID，并将其持久化存储。然后返回该设备ID。
 * 如果设备ID文件已经存在，则读取并返回其中存储的设备ID。
 * @return 设备ID，在同一系统重启期间该ID不会改变。
 */
std::string getOrCreateUnitId();

// Clean up the directory where OpenHD persistent settings are stored
// Which in turn means that all modules that follow the "create default settings
// when no settings are found by (HW)-id" will create full new default settings.
// 清理存储OpenHD持久化设置的目录
// 这意味着所有遵循“当通过（硬件）ID未找到设置时创建默认设置”规则的模块，都将创建全新的默认设置。
void clean_all_settings();

// Helper for development - we catch 2 things with the following pattern:
// 1) When openhd is started - check if the file exists, in which case either a
// develoer started openhd twice (which most likely was a mistake) or the
// previous openhd execution did not terminate properly (which is only a soft
// error, since properly terminating is a nice to have but not necessarily
// required) 2) When openhd is stopped (SIGTERM) - remove the file
// 开发辅助功能：通过以下模式我们能捕捉到两种情况：
// 1) 当OpenHD启动时 - 检查文件是否存在。若文件存在，可能是开发者误操作启动了两次OpenHD，
//    也可能是上一次OpenHD执行未正常终止（不过这只是个小问题，因为正常终止虽然理想但并非必需）。
// 2) 当OpenHD停止（接收到SIGTERM信号）时 - 删除该文件。
static std::string get_openhd_is_running_filename() {
    return "/tmp/openhd_is_running.txt";
}

void check_currently_running_file_and_write();

void remove_currently_running_file();

}  // namespace openhd

#endif
