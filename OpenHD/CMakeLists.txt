cmake_minimum_required(VERSION 3.16.3)
project(OpenHD)

# 设置交叉编译工具链
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 指定交叉编译工具链路径
set(TOOLCHAIN_DIR /opt/rv1106_toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin)
set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/arm-rockchip830-linux-uclibcgnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-rockchip830-linux-uclibcgnueabihf-g++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Allows to explicitly disable air support - removes a few dependencies
option(ENABLE_AIR "Enable air support" ON)
message("Air support enabled: ${ENABLE_AIR}")
if (ENABLE_AIR)
        add_definitions(-DENABLE_AIR)
endif ()

# Allows to explicitly disable USB camera support - removes a few dependencies
# (Libusb an libv4l2), only makes sense if air support is enabled
option(ENABLE_USB_CAMERAS "USB cameras support" ON)
message("USB cameras enabled: ${ENABLE_USB_CAMERAS}")
if (ENABLE_USB_CAMERAS)
        add_definitions(-DENABLE_USB_CAMERAS)
endif ()

# Add subdirectories for libraries
add_subdirectory(ohd_common EXCLUDE_FROM_ALL)
add_subdirectory(ohd_interface EXCLUDE_FROM_ALL)
add_subdirectory(ohd_telemetry EXCLUDE_FROM_ALL)
add_subdirectory(ohd_video EXCLUDE_FROM_ALL)

# Suppress specific warnings
add_compile_options(-Wno-address-of-packed-member -Wno-cast-align)

# Here we build the single executable that makes up OpenHD.
# It depends on all the libraries we have just added above
add_executable(openhd main.cpp
        ohd_common/inc/config_paths.h
        ohd_common/src/config_paths.cpp)

# Link the libraries
target_link_libraries(openhd PRIVATE OHDInterfaceLib OHDVideoLib OHDTelemetryLib OHDCommonLib)

# Check if libatomic is needed and available
include(CheckLibraryExists)
check_library_exists(atomic __atomic_fetch_add_1 "" HAVE_LIBATOMIC)

if (HAVE_LIBATOMIC)
    target_link_libraries(openhd PRIVATE atomic)
endif()

# 设置输出目录到 release 文件夹
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/release)

# 安装可执行文件到 release 文件夹
install(TARGETS openhd DESTINATION ${CMAKE_SOURCE_DIR}/release)