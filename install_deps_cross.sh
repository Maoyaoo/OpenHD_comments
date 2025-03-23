#!/usr/bin/env bash
################################################################################
# OpenHD
#
# Licensed under the GNU General Public License (GPL) Version 3.
#
# This software is provided "as-is," without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose, and non-infringement. For details, see the
# full license in the LICENSE file provided with this source code.
#
# Non-Military Use Only:
# This software and its associated components are explicitly intended for
# civilian and non-military purposes. Use in any military or defense
# applications is strictly prohibited unless explicitly and individually
# licensed otherwise by the OpenHD Team.
#
# Contributors:
# A full list of contributors can be found at the OpenHD GitHub repository:
# https://github.com/OpenHD
#
# © OpenHD, All Rights Reserved.
################################################################################

set -e

PLATFORM="$1"
TOOLCHAIN="/opt/rv1106_toolchain"  # 交叉编译工具链路径
SYSROOT="${TOOLCHAIN}/sysroot"     # 工具链的 sysroot 路径

# 基础依赖包
BASE_PACKAGES="libpoco-dev clang-format libusb-1.0-0-dev libpcap-dev libsodium-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev libsdl2-dev"
VIDEO_PACKAGES="libgstreamer-plugins-base1.0-dev libv4l-dev"
BUILD_PACKAGES="git build-essential autotools-dev automake libtool python3-pip autoconf apt-transport-https ruby-dev cmake"

# 平台特定的包
function install_pi_packages {
    PLATFORM_PACKAGES="libcamera-openhd"
    PLATFORM_PACKAGES_REMOVE="python3-libcamera libcamera0"
}
function install_x86_packages {
    PLATFORM_PACKAGES="libunwind-dev gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly"
    PLATFORM_PACKAGES_REMOVE=""
}
function install_rock_packages {
    PLATFORM_PACKAGES="gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly"
    PLATFORM_PACKAGES_REMOVE=""
}

# 设置工具链环境变量
export PATH="${TOOLCHAIN}/bin:${PATH}"
export CC=arm-rockchip830-linux-uclibcgnueabihf-gcc
export CXX=arm-rockchip830-linux-uclibcgnueabihf-g++
export SYSROOT=${SYSROOT}

# 添加 OpenHD 仓库
apt update
apt install -y curl
curl -1sLf 'https://dl.cloudsmith.io/public/openhd/release/setup.deb.sh' | sudo -E bash
apt update
apt upgrade -y -o Dpkg::Options::="--force-overwrite" --no-install-recommends --allow-downgrades

# 选择平台特定的包
if [[ "${PLATFORM}" == "rpi" ]]; then
    install_pi_packages
elif [[ "${PLATFORM}" == "ubuntu-x86" ]]; then
    install_x86_packages
elif [[ "${PLATFORM}" == "rock5" ]]; then
    install_rock_packages
else
    echo "Platform not supported"
    exit 1
fi

# 移除平台特定的包
echo "Removing platform-specific packages..."
for package in ${PLATFORM_PACKAGES_REMOVE}; do
    echo "Removing ${package}..."
    apt purge -y ${package}
    if [ $? -ne 0 ]; then
        echo "Failed to remove ${package}!"
        exit 1
    fi
done

# 安装依赖包到 sysroot
echo "Installing dependencies to sysroot..."
for package in ${PLATFORM_PACKAGES} ${BASE_PACKAGES} ${VIDEO_PACKAGES} ${BUILD_PACKAGES}; do
    echo "Downloading ${package}..."
    apt download ${package}

    echo "Extracting ${package} to sysroot..."
    dpkg-deb -x ${package}*.deb ${SYSROOT}

    echo "Cleaning up ${package}..."
    rm -f ${package}*.deb
done

# 安装 Ruby 包
gem install dotenv -v 2.8.1
gem install fpm
