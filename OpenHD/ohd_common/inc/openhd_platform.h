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

#ifndef OPENHD_PLATFORM_H
#define OPENHD_PLATFORM_H

#include <string>

// Some things conditionally depend on the platform we are running on
// 有些事情依赖于我们所运行的平台。

// When this one shows up a bit more work has to be done to run openhd on the
// platform (probably) ;)
// 当这个出现时，可能需要做更多的工作才能在平台上运行 openhd ;)
static constexpr int X_PLATFORM_TYPE_UNKNOWN = 0;

// Generic X86
static constexpr int X_PLATFORM_TYPE_X86 = 1;


// Numbers 10..20 are reserved for rpi
// Right now we are only interested if it is an RPI of the
// generation RPI 4 / RPI CM4 or the generation before -
// NOTE: RPI 5 is currently not supported due to the complete lack of suitable
// HW acceleration
// 数字 10..20 保留给 RPI
// 目前我们只关心是否是 RPI 4 / RPI CM4 或者之前的版本 –
// 注意：由于完全缺乏合适的硬件加速，RPI 5 目前不被支持
static constexpr int X_PLATFORM_TYPE_RPI_OLD = 10;
static constexpr int X_PLATFORM_TYPE_RPI_4 = 11;
static constexpr int X_PLATFORM_TYPE_RPI_CM4 = 12;
static constexpr int X_PLATFORM_TYPE_RPI_5 = 12;

// Numbers 20..30 are reserved for rockchip
// 数字 20..30 保留给 Rockchip
static constexpr int X_PLATFORM_TYPE_ROCKCHIP_RK3566_RADXA_ZERO3W = 20;  // Zero 3 W
static constexpr int X_PLATFORM_TYPE_ROCKCHIP_RK3588_RADXA_ROCK5_A = 21;
static constexpr int X_PLATFORM_TYPE_ROCKCHIP_RK3588_RADXA_ROCK5_B = 22;
static constexpr int X_PLATFORM_TYPE_ROCKCHIP_RK3566_RADXA_CM3 = 24;
static constexpr int X_PLATFORM_TYPE_ROCKCHIP_RV1126_UNDEFINED = 23;  // FUTURE

// Numbers 30..35 are reserved for allwinner
// 数字 30..35 保留给 Allwinner
static constexpr int X_PLATFORM_TYPE_ALWINNER_X20 = 30;

// @Buldo is working on openipc / sigmastar, 36..39
static constexpr int X_PLATFORM_TYPE_OPENIPC_SIGMASTAR_UNDEFINED = 36;

// Numbers 40..45 are reserved for NVIDIA
// 数字 40..45 保留给 NVIDIA
static constexpr int X_PLATFORM_TYPE_NVIDIA_XAVIER = 40;

// Numbers 46..50 are reserved for QUALCOMM
// 数字 46..50 保留给 QUALCOMM
static constexpr int X_PLATFORM_TYPE_QUALCOMM_QRB5165 = 46;
static constexpr int X_PLATFORM_TYPE_QUALCOMM_QCS405 = 47;
static constexpr int X_PLATFORM_TYPE_QUALCOMM_UNKNOWN = 48;

std::string x_platform_type_to_string(int platform_type);

// Depends on single threaded CPU performance & weather NEON is available
// Rough estimate
// 取决于单线程 CPU 性能以及是否支持 NEON
// 粗略估计
int get_fec_max_block_size_for_platform();

// All these members must not change during run time once they have been
// discovered !
// 一旦这些成员被发现，它们在运行时不得更改！
struct OHDPlatform {
 public:
  explicit OHDPlatform(const int platform_type)
      : platform_type(platform_type) {}
  const int platform_type;
  [[nodiscard]] std::string to_string() const;
  static const OHDPlatform& instance();
  bool is_rpi() const;
  bool is_rock() const;
  bool is_zero3w() const;
  bool is_radxa_cm3() const;
  bool is_rock5_a() const;
  bool is_rock5_b() const;
  bool is_rock5_a_b() const;
  bool is_rpi_or_x86() const;
  // alwinner
  bool is_x20() const;
  // qualcomm
  bool is_qrb5165() const;
  bool is_qcs405() const;
};

// We need to differentiate between rpi 4 and other pi's to use the right fec
// params.
// 我们需要区分 RPI 4 和其他 Pi，以使用正确的 FEC 参数。
bool platform_rpi_is_high_performance(const OHDPlatform& platform);

#endif
