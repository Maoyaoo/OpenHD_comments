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

#include <OHDTelemetry.h>
#include <getopt.h>
#include <ohd_interface.h>
#ifdef ENABLE_AIR
#include <ohd_video_air.h>
#endif  // ENABLE_AIR
#include <ohd_video_ground.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "config_paths.h"
#include "openhd_buttons.h"
#include "openhd_config.h"
#include "openhd_global_constants.hpp"
#include "openhd_platform.h"
#include "openhd_profile.h"
#include "openhd_spdlog.h"
#include "openhd_temporary_air_or_ground.h"

// |-------------------------------------------------------------------------------|
// |                         OpenHD core executable | | Weather you run as air
// (creates openhd air unit) or run as ground             | | (creates openhd
// ground unit) needs to be specified by either using the command| | line param
// (development) or using a text file (openhd images)                 | | Read
// the code documentation in this project for more info.                    |
// |-------------------------------------------------------------------------------|
// |-------------------------------------------------------------------------------|
// |                         OpenHD 核心可执行文件 | | 无论是作为空中单元运行
// （创建 OpenHD 空中单元）还是作为地面单元运行 | | （创建 OpenHD 地面单元），
// 都需要通过命令行参数（开发时）或文本文件（OpenHD 镜像）来指定 | |
// 请阅读本项目中的代码文档以获取更多信息。                    |
// |-------------------------------------------------------------------------------|

// A few run time options, only for development. Way more configuration (during
// development) can be done by using the hardware.config file
// 一些运行时选项，仅用于开发。通过使用 hardware.config
// 文件，可以进行更多的配置（在开发过程中） 定义了命令行参数的解析方式
static const char optstr[] = "?:agcwr:h:";
static const struct option long_options[] = {
    {"air", no_argument, nullptr, 'a'},
    {"ground", no_argument, nullptr, 'g'},
    {"clean-start", no_argument, nullptr, 'c'},
    {"no-qt-autostart", no_argument, nullptr, 'w'},
    {"run-time-seconds", required_argument, nullptr, 'r'},
    {"hardware-config-file", required_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0},
};
const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string blue = "\033[94m";
const std::string reset = "\033[0m";

struct OHDRunOptions {
    bool run_as_air = false;
    bool reset_all_settings = false;
    bool no_qopenhd_autostart = false;
    int run_time_seconds = -1;  //-1= infinite, only usefully for debugging // -1 = 无限，仅用于调试
    // Specify the hardware.config file, otherwise,
    // the default location (and default values if no file exists at the default
    // location) is used
    // 指定 hardware.config 文件，否则，
    // 将使用默认位置（如果默认位置不存在文件，则使用默认值）
    std::optional<std::string> hardware_config_file;
};

static OHDRunOptions parse_run_parameters(int argc, char* argv[]) {
    OHDRunOptions ret{};
    int c;
    // If this value gets set, we assume a developer is working on OpenHD and skip
    // the discovery via file(s).
    // 如果设置了此值，我们假设开发者正在开发 OpenHD，并跳过通过文件进行的发现过程。
    std::optional<bool> commandline_air = std::nullopt;
    while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
        const char* tmp_optarg = optarg;  // c 是解析后的选项字符，optarg 是当前选项的参数值。
        switch (c) {
            case 'a':
                // 如果 commandline_air 已经被设置过（即不为 std::nullopt），
                // 程序会输出错误信息并退出,避免因为不能同时设置 --air 和 --ground。
                if (commandline_air != std::nullopt) {
                    //  Already set, e.g. --ground is already used
                    std::cerr << "Please use either air or ground as param\n";
                    exit(1);
                }
                commandline_air = true;
                break;
            case 'g':
                // Already set, e.g. --air is already used
                if (commandline_air != std::nullopt) {
                    std::cerr << "Please use either air or ground as param\n";
                    exit(1);
                }
                commandline_air = false;
                break;
            case 'c':  // clean-start
                ret.reset_all_settings = true;
                break;
            case 'w':  // no-qt-autostart
                ret.no_qopenhd_autostart = true;
                break;
            case 'r':                                     // run-time-seconds
                ret.run_time_seconds = atoi(tmp_optarg);  // 转为整数
                break;
            case 'h':
                ret.hardware_config_file = tmp_optarg;
                break;
            case '?':
            default: {
                std::stringstream ss;
                ss << "Usage: \n";
                ss << "--air -a          [Run as air, creates dummy camera if no "
                      "camera is found] \n";
                ss << "--ground -g       [Run as ground, no camera detection] \n";
                ss << "--clean-start -c  [Wipe all persistent settings OpenHD has "
                      "written, can fix any boot issues when switching hw around] \n";
                ss << "--no-qt-autostart [disable auto start of QOpenHD on ground] \n";
                ss << "--run-time-seconds -r [Manually specify run time (default "
                      "infinite),for debugging] \n";
                ss << "--hardware-config-file -h [specify path to hardware.config "
                      "file]\n";
                ss << "Use hardware.conf for more configuration\n";
                std::cout << ss.str() << std::flush;
            }
                exit(1);
        }
    }
    // 如果没使用命令行启动则监测文件进行启动
    if (commandline_air == std::nullopt) {
        // command line parameters not used, use the file(s) for detection (default
        // for normal OpenHD images) The logs/checks here are just to help
        // developer(s) avoid common misconfigurations
        // 未使用命令行参数，使用文件进行检测（这是正常 OpenHD 镜像的默认方式）
        // 这里的日志/检查只是为了帮助开发者避免常见的配置错误
        const bool file_run_as_ground_exists = openhd::tmp::file_ground_exists();
        const bool file_run_as_air_exists = openhd::tmp::file_air_exists();
        bool error = false;
        // 天空端和地面端文件都存在，则作为地面端启动
        if (file_run_as_air_exists && file_run_as_ground_exists) {  // both files exist
            // Just run as ground
            ret.run_as_air = false;
            error = true;
        }
        // 天空端和地面端文件都不存在，则作为地面端启动
        if (!file_run_as_air_exists && !file_run_as_ground_exists) {  // no file exists
            // Just run as ground
            ret.run_as_air = false;
            error = true;
        }
        // 存在哪个文件就以哪个端启动
        if (!error) {
            if (!file_run_as_air_exists) {
                ret.run_as_air = false;
            } else {
                ret.run_as_air = true;
            }
        }
    } else {
        // command line parameters used, just validate they are not mis-configured
        // 使用命令行参数，只需验证它们没有配置错误
        assert(commandline_air.has_value());
        ret.run_as_air = commandline_air.value();
    }
    // If this file exists, delete all openhd settings resulting in default
    // value(s)
    // 如果该文件存在，删除所有 OpenHD 设置，恢复为默认值
    const auto filePathReset = std::string(getConfigBasePath()) + "reset.txt";
    if (OHDUtil::file_exists_and_delete(filePathReset.c_str())) {
        ret.reset_all_settings = true;
    }
// 在编译时，开发者可通过定义 ENABLE_AIR 宏来决定是否启用 “空中模式” 支持。若未启用，而程序运行时又配置为 “空中模式”，就会自动调整为 “地面模式”
#ifndef ENABLE_AIR
    if (ret.run_as_air) {
        std::cerr << "NOTE: COMPILED WITH GROUND ONLY SUPPORT,RUNNING AS GND" << std::endl;
        ret.run_as_air = false;
    }
#endif
    return ret;
}

int main(int argc, char* argv[]) {
    // OpenHD needs to be run as root!
    OHDUtil::terminate_if_not_root();

    // 这段代码的作用是检查 /run/openhd/hold.pid 文件是否存在。如果该文件存在，程序就会立即退出。
    // 这种做法可能用于防止某些重复运行的进程。例如，程序可能在启动时会先检查这个 .pid 文件，若该文件存在则表示程序或服务已经在运行，因此程序选择退出以避免重复启动。
    if (OHDFilesystemUtil::exists("/run/openhd/hold.pid")) {
        std::exit(0);
    }

    // 解析运行参数，包括：天空端/地面端、是否重置所有设置、硬件配置文件路径
    const OHDRunOptions options = parse_run_parameters(argc, argv);

    // 设置配置文件路径
    if (options.hardware_config_file.has_value()) {
        openhd::set_config_file(options.hardware_config_file.value());
    }
    // 打印openhd信息
    {                                     // Print all the arguments the OHD main executable is started with
        std::cout << "\033[2J\033[1;1H";  // clear terminal
        std::stringstream ss;
        ss << openhd::get_ohd_version_as_string() << "\n";
        ss << blue;
        ss << "  #######  ########  ######## ##    ## ##     ## ######## \n";
        ss << " ##     ## ##     ## ##       ###   ## ##     ## ##     ##\n";
        ss << " ##     ## ##     ## ##       ####  ## ##     ## ##     ##\n";
        ss << " ##     ## ########  ######   ## ## ## ######### ##     ##\n";
        ss << " ##     ## ##        ##       ##  #### ##     ## ##     ##\n";
        ss << " ##     ## ##        ##       ##   ### ##     ## ##     ##\n";
        ss << "  #######  ##        ######## ##    ## ##     ## ######## \n";
        ss << reset;
        ss << "----------------------- OpenSource -----------------------\n";
        ss << "\n";

        if (options.run_as_air) {
            ss << "----------------------- " << green << "Air Unit" << reset << " -----------------------\n";
        } else {
            ss << "----------------------- " << red << "Ground Unit" << reset << " ----------------------\n";
        }

        if (options.reset_all_settings) {
            ss << red << "Reset Settings" << reset << "\n";
        }

        ss << "\n";
        ss << "\n";
        ss << "\n";

        // ss << "Git info:Branch:" << git_Branch() << " SHA:" << git_CommitSHA1()
        // << " Dirty:" << OHDUtil::yes_or_no(git_AnyUncommittedChanges()) << "\n";

        std::cout << ss.str() << std::flush;
        // openhd::debug_config();
        // OHDInterface::print_internal_fec_optimization_method();
    }
    // Create the folder structure
    // 创建文件目录结构
    openhd::generateSettingsDirectoryIfNonExists();

    // 获取平台类型
    const auto platform = OHDPlatform::instance();

    // 设置LED灯状态加载
    openhd::LEDManager::instance().set_status_loading();

    // Generate the keys and delete pw if needed
    // 生成密钥并在需要时删除密码
    OHDInterface::generate_keys_from_pw_if_exists_and_delete();

    // Parse the program arguments
    // This is the console we use inside main, in general different openhd
    // modules/classes have their own loggers with different tags
    // 解析程序参数
    // 这是我们在主函数中使用的控制台，通常不同的 OpenHD 模块/类有各自的日志记录器和不同的标签
    std::shared_ptr<spdlog::logger> m_console = openhd::log::create_or_get("main");
    assert(m_console);

    // not guaranteed, but better than nothing, check if openhd is already running
    // (kinda) and print warning if yes.
    // 不保证准确，但总比没有检查好，检查 OpenHD 是否已经在运行
    // （某种程度上），如果是的话打印警告。
    openhd::check_currently_running_file_and_write();

    // Create and link all the OpenHD modules.
    // 创建并链接所有 OpenHD 模块。
    try {
        // This results in fresh default values for all modules (e.g. interface,
        // telemetry, video)
        // 这将为所有模块（例如接口、遥测、视频）生成新的默认值
        if (options.reset_all_settings) {
            openhd::clean_all_settings();
        }
        // 用户硬件使能清楚配置
        if (openhd::ButtonManager::instance().user_wants_reset_openhd_core()) {
            openhd::clean_all_settings();
        }

        // Profile no longer depends on n discovered cameras,
        // But if we are air, we have at least one camera, sw if no camera was found
        // 配置文件不再依赖于发现的摄像头数量，
        // 但如果我们是空中单元，至少有一个摄像头；如果没有找到摄像头，则使用软件模式
        const auto profile = DProfile::discover(options.run_as_air);
        write_profile_manifest(profile);  // 写入配置文件清单

        // we need to start QOpenHD when we are running as ground, or stop / disable
        // it when we are running as air. can be disabled for development purposes.
        // On x20, we do not have qopenhd installed (we run as air only) so we can
        // skip this step
        // 当我们作为地面单元运行时，需要启动 QOpenHD；
        // 当我们作为空中单元运行时，需要停止或禁用它。
        // 这可以为开发目的禁用。
        // 在 x20 上，我们没有安装 qopenhd（只作为空中单元运行），因此可以跳过这一步。
        if (!options.no_qopenhd_autostart) {
            if (!openhd::load_config().GEN_NO_QOPENHD_AUTOSTART && !OHDPlatform::instance().is_x20()) {
                if (!profile.is_air) {
                    OHDUtil::run_command("systemctl", {"start", "qopenhd"});  // 执行一个系统命令
                } else {
                    OHDUtil::run_command("systemctl", {"stop", "qopenhd"});
                }
            }
        }

        // create the global action handler that allows openhd modules to
        // communicate with each other e.g. when the rf link in ohd_interface needs
        // to talk to the camera streams to reduce the bitrate
        // 创建全局操作处理程序，允许 OpenHD 模块之间进行通信，例如，当 ohd_interface 中的 RF 链接需要
        // 与摄像头流进行通信以降低比特率时。
        openhd::LinkActionHandler::instance();

        // We start ohd_telemetry as early as possible, since even without a link
        // (transmission) it still picks up local log message(s) and forwards them
        // to any ground station clients (e.g. QOpenHD)
        // 我们尽早启动 ohd_telemetry，因为即使没有链接（传输），
        // 它仍然会接收本地日志消息并将它们转发给任何地面站客户端（例如 QOpenHD）。
        auto ohdTelemetry = std::make_shared<OHDTelemetry>(profile);

        // Then start ohdInterface, which discovers detected wifi cards and more.
        // 然后启动 ohdInterface，它会发现检测到的 WiFi 卡和更多内容。
        auto ohdInterface = std::make_shared<OHDInterface>(profile);

        // Telemetry allows changing all settings (even from other modules)
        // 遥测允许更改所有设置（即使是来自其他模块的设置）
        ohdTelemetry->add_settings_generic(ohdInterface->get_all_settings());

        // either one is active, depending on air or ground
        // 取决于是空中还是地面，其中一个是活动的
        std::unique_ptr<OHDVideoGround> ohd_video_ground = nullptr;
        if (profile.is_ground()) {
            ohd_video_ground = std::make_unique<OHDVideoGround>(ohdInterface->get_link_handle());
        }
#ifdef ENABLE_AIR
        std::unique_ptr<OHDVideoAir> ohd_video_air = nullptr;
        if (profile.is_air) {
            auto cameras = OHDVideoAir::discover_cameras();
            ohd_video_air = std::make_unique<OHDVideoAir>(cameras, ohdInterface->get_link_handle());
            // First add camera specific settings (primary & secondary camera)
            auto settings_components = ohd_video_air->get_all_camera_settings();
            ohdTelemetry->add_settings_camera_component(0, settings_components[0]);
            ohdTelemetry->add_settings_camera_component(1, settings_components[1]);
            // Then the rest
            ohdTelemetry->add_settings_generic(ohd_video_air->get_generic_settings());
        }
#endif  // ENABLE_AIR
        // We do not add any more settings to ohd telemetry - the param set(s) are
        // complete
        // 我们不再向 OHD 遥测添加更多设置 - 相关参数集已完备 。
        ohdTelemetry->settings_generic_ready();

        // now telemetry can send / receive data via wifibroadcast
        ohdTelemetry->set_link_handle(ohdInterface->get_link_handle());
        std::cout << green << "OpenHD was successfully started." << reset << std::endl;
        openhd::LEDManager::instance().set_status_okay();

        // run forever, everything has its own threads. Note that the only way to
        // break out basically is when one of the modules encounters an exception.
        // 程序将永久运行，每个组件都有各自独立的线程。需要注意的是，基本上只有当某个模块遇到异常时，程序才会终止运行。
        static bool quit = false;
        // https://unix.stackexchange.com/questions/362559/list-of-terminal-generated-signals-eg-ctrl-c-sigint
        signal(SIGTERM, [](int sig) {
            std::cerr << "Got SIGTERM, exiting\n";
            quit = true;
        });
        signal(SIGQUIT, [](int sig) {
            std::cerr << "Got SIGQUIT, exiting\n";
            quit = true;
        });
        const auto run_time_begin = std::chrono::steady_clock::now();
        while (!quit) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (options.run_time_seconds >= 1) {
                if (std::chrono::steady_clock::now() - run_time_begin >= std::chrono::seconds(options.run_time_seconds)) {
                    m_console->warn("Terminating, exceeded run time {}", options.run_time_seconds);
                    // we can just break out any time, usefully for checking memory leaks
                    // and more.
                    // 我们可以随时终止程序运行，这对于检查内存泄漏等问题很有用。
                    break;
                }
            }
            if (openhd::TerminateHelper::instance().should_terminate()) {
                m_console->debug("Terminating,reason:{}", openhd::TerminateHelper::instance().terminate_reason());
                break;
            }
        }
        // --- terminate openhd, most likely requested by a developer with sigterm
        // 这里 “OpenHD” 是一个特定的项目或软件名称，一般不用翻译直接保留；“SIGTERM” 是 Unix/Linux 系统中用于请求进程正常终止的信号。
        m_console->debug("Terminating openhd");
        openhd::LEDManager::instance().set_status_stopped();
        // Stop any communication between modules, to eliminate any issues created
        // by threads during cleanup
        // 停止模块之间的所有通信，以消除清理过程中线程引发的任何问题。
        openhd::LinkActionHandler::instance().disable_all_callables();
        openhd::ExternalDeviceManager::instance().remove_all();
        // dirty, wait a bit to make sure none of those action(s) are called anymore
        // 这方法不太优雅，但还是等待一小段时间，以确保那些操作不再被调用。这里 “dirty” 通常在编程语境里形容一种不太规范、不太优雅或者临时的解决方案。
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // unique ptr would clean up for us, but this way we are a bit more verbose
        // since some of those modules talk to each other, this is a bit prone to
        // failures.
        // 智能指针（unique_ptr）本可以为我们完成清理工作，但采用这种方式会让代码更详细易懂。
        // 由于其中一些模块之间会相互通信，所以这种做法有点容易出错。
#ifdef ENABLE_AIR
        if (ohd_video_air) {
            m_console->debug("Terminating ohd_video_air - begin");
            ohd_video_air.reset();
            m_console->debug("Terminating ohd_video_air - end");
        }
#endif
        if (ohd_video_ground) {
            m_console->debug("Terminating ohd_video_ground- begin");
            ohd_video_ground.reset();
            m_console->debug("Terminating ohd_video_ground - end");
        }
        if (ohdTelemetry) {
            m_console->debug("Terminating ohd_telemetry - begin");
            ohdTelemetry.reset();
            m_console->debug("Terminating ohd_telemetry - end");
        }
        if (ohdInterface) {
            m_console->debug("Terminating ohd_interface - begin");
            ohdInterface.reset();
            m_console->debug("Terminating ohd_interface - end");
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(1);
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        exit(1);
    }
    openhd::remove_currently_running_file();
    return 0;
}
