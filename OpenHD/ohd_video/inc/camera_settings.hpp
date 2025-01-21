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

#ifndef OPENHD_CAMERA_H
#define OPENHD_CAMERA_H

#include "camera_enums.hpp"
#include "validate_settings.h"

// Mutable data for a discovered camera
// See camera_holder for how the settings are created the first time a camera is
// detected and changed via mavlink / openhd mavlink.

// For the default value, we assume a fec overhead of 20% - 8MBit/s before FEC
// fits well into MCS index 3, even on highly polluted channels (we account for
// the worst here)
static constexpr int DEFAULT_BITRATE_KBITS = 8000;
// The ideal value is not definitive, and depends on the rf environment, the FEC
// percentage, and the camera fps Higher values result in less key frames, and
// better image quality at the same bitrate, but increases the risk for
// "stuttering" in case frames are lost.
static constexpr int DEFAULT_KEYFRAME_INTERVAL = 5;

// Minimum amount of free space required to enable air video recording.
// Also, If the free space becomes less than that, air recording (if running)
// should be stopped. This feature is r.n already implemented for all cameras
// (in gstreamerstream)
static constexpr auto MINIMUM_AMOUNT_FREE_SPACE_FOR_AIR_RECORDING_MB = 300;
static constexpr int RPI_LIBCAMERA_DEFAULT_EV = 0;

static constexpr int OPENHD_BRIGHTNESS_DEFAULT = 100;
static constexpr int OPENHD_SATURATION_DEFAULT = 100;
static constexpr int OPENHD_CONTRAST_DEFAULT = 100;
static constexpr int OPENHD_SHARPNESS_DEFAULT = 100;

static constexpr int OPENHD_FLIP_NONE = 0;
static constexpr int OPENHD_FLIP_HORIZONTAL = 1;
static constexpr int OPENHD_FLIP_VERTICAL = 2;
static constexpr int OPENHD_FLIP_VERTICAL_AND_HORIZONTAL = 3;

// User-selectable camera options
// These values are settings that can change dynamically at run time
// (non-deterministic)
// 用户可选择的摄像头选项
// 这些值是可以在运行时动态更改的设置（非确定性）
struct CameraSettings {
  // Enable / Disable streaming for this camera
  // This can be usefully for debugging, but also when the there is suddenly a
  // really high interference, and the user wants to fly home without video,
  // using only telemetry / HUD. Default to true, otherwise we'd have conflicts
  // with the "always a picture without changing any settings" paradigm.
  // 启用/禁用此摄像头的流式传输
  // 这对于调试非常有用，也可以在突然出现严重干扰时使用，
  // 用户希望在没有视频的情况下飞回家，仅使用遥测数据/HUD。
  // 默认为 true，否则会与“始终有画面且不更改任何设置”的原则产生冲突。
  bool enable_streaming = true;
  int qp_max = 51;
  int qp_min = 5;

  // The video format selected by the user. If the user sets a video format that
  // isn't supported (for example, he might select h264|1920x1080@120 but the
  // camera can only do 60fps) the camera might stop streaming, and the user has
  // to set a different resolution manually (In general, we cannot really check
  // if a camera supports a given resolution / framerate properly yet) Note that
  // this default value is overridden in case we know more about the camera(s).
  // 用户选择的视频格式。如果用户设置了一个不受支持的视频格式
  // （例如，他可能选择 h264|1920x1080@120，但摄像头只能支持 60fps），
  // 摄像头可能会停止流式传输，用户需要手动设置不同的分辨率。
  // （通常，我们还不能真正检查摄像头是否支持给定的分辨率/帧率）
  // 注意，如果我们了解更多关于摄像头的信息，默认值将被覆盖。
  VideoFormat streamed_video_format{VideoCodec::H264, 640, 480, 30};

  // The settings below can only be implemented on a "best effort" manner -
  // changing them does not necessarily mean the camera supports changing them.
  // Unsupported settings have to be ignored during pipeline construction In
  // general, we only try to expose these values as mavlink parameters if the
  // camera supports them, to not confuse the user.
  //
  // ----------------------------------------------------------------------------------------------------------
  //
  // The bitrate the generated stream should have. Note that not all cameras /
  // encoders support a constant bitrate, and not all encoders support all
  // bitrates, especially really low ones. How an encoder handles a specific
  // constant bitrate is vendor specific. Note that we always use a constant
  // bitrate in OpenHD, since it is the only way to properly adjust the bitrate
  // depending on the link quality (once we have that wired up).
  // 以下设置只能以“尽力而为”的方式实现——更改它们并不一定意味着摄像头支持更改这些设置。
  // 在管道构建过程中，必须忽略不受支持的设置。
  // 通常，我们只有在摄像头支持这些设置时，才会将它们暴露为 mavlink
  // 参数，以避免混淆用户。
  // ----------------------------------------------------------------------------------------------------------
  // 生成的流应具有的比特率。请注意，并非所有摄像头/编码器都支持恒定比特率，
  // 并且并非所有编码器都支持所有比特率，尤其是非常低的比特率。
  // 编码器如何处理特定的恒定比特率是厂商特定的。
  // 请注意，在 OpenHD
  // 中，我们始终使用恒定比特率，因为这是根据链路质量正确调整比特率的唯一方法（
  // 一旦我们将其连接起来）。
  int h26x_bitrate_kbits = DEFAULT_BITRATE_KBITS;

  // Interval (in frames) between I frames. -1 = automatic, 0 = single-keyframe
  // , else positive values up to 2147483647 note that with 0 and/or sometimes
  // -1, you can create issues like no decoded image at all, since wifibroadcast
  // relies on keyframes in regular intervals. Also, some camera(s) might use a
  // different mapping in regard to the keyframe interval than what's defined
  // here, supporting them needs different setting validation methods. only
  // valid for h264 / h265, mjpeg has no keyframe interval
  // I 帧之间的间隔（以帧为单位）。-1 = 自动，0 = 单帧关键帧，
  // 否则为正值，最大值为 2147483647。请注意，使用 0 和/或有时使用 -1，
  // 可能会导致没有解码图像的问题，因为 wifibroadcast 依赖于定期的关键帧。
  // 此外，某些摄像头可能使用与此处定义的关键帧间隔不同的映射，
  // 支持它们需要不同的设置验证方法。
  // 仅对 h264 / h265 有效，mjpeg 没有关键帧间隔。
  int h26x_keyframe_interval = DEFAULT_KEYFRAME_INTERVAL;

  // Type of Intra Refresh to use, -1 to disable intra refresh. R.n only
  // supported on gst-rpicamsrc and sw encoder See gst-rpicamsrc for more info
  // on mmal (there we have different intra options) sw encoder only has off
  // (-1) and on (anything not -1)
  // 要使用的 Intra 刷新类型，-1 表示禁用 Intra 刷新。仅在 R.n 上支持
  // 仅在 gst-rpicamsrc 和软件编码器上支持。有关 mmal 的更多信息，请参阅
  // gst-rpicamsrc （在那里我们有不同的 intra 选项），软件编码器仅有关闭（-1）
  // 和开启（任何不是 -1 的值）。
  int h26x_intra_refresh_type = -1;

  // N of slices. Not supported on all hardware (none to be exact unless the
  // cisco sw encoder) as of now 0 == frame slicing off
  // 切片数。并非所有硬件都支持（目前没有，除非是 cisco 软件编码器）
  // 0 == 关闭帧切片。
  int h26x_num_slices = 0;

  // enable/disable recording to file
  // 启用/禁用录制到文件
  int air_recording = AIR_RECORDING_OFF;

  //
  // Below are params that most often only affect the ISP, not the encoder
  //
  // camera rotation, only supported on rpicamsrc at the moment
  // 0 nothing, 90° to the right, 180° to the right, 270° to the right
  //
  // 以下是通常只影响 ISP（图像信号处理器），而不是编码器的参数
  //
  // 摄像头旋转，目前仅在 rpicamsrc 上支持
  // 0 不旋转，90° 向右旋转，180° 向右旋转，270° 向右旋转
  int camera_rotation_degree = 0;
  
  // horizontal / vertical flip, r.n only supported on rpicamsrc, libcamera,
  // (x20 ?)
  int openhd_flip = OPENHD_FLIP_NONE;

  // Depending on the cam type, openhd uses hw-accelerated encoding whenever
  // possible. However, in some cases (e.g. when using a USB camera that outputs
  // raw and h264, but the hw encoder of the cam is bad) or for experimenting
  // (e.g. when using libcamera / rpicamsrc and RPI4) one might prefer to use SW
  // encode. Enabling this is no guarantee a sw encoded pipeline exists for this
  // camera.
  bool force_sw_encode = false;

  // OpenHD WB supports changing encryption on the fly per camera stream
  bool enable_ultra_secure_encryption = false;

  // -----------------------------------------------------------------------------------------------------------------------
  // IQ (Image quality) settings begin. Values prefixed with openhd_ are values
  // where openhd defines the range, and each camera that implements the given
  // functionality needs to use this range (re-mapping is possible, for example
  // openhd_brightness is re-mapped for libcamera, which takes a float Values
  // prefixed with a vendor-specific string (for example lc_ ) are values that
  // cannot be generified and therefore need to be different for each camera.
  // default 100, range [0,200]
  int openhd_brightness = OPENHD_BRIGHTNESS_DEFAULT;
  int openhd_saturation = OPENHD_SATURATION_DEFAULT;
  int openhd_contrast = OPENHD_CONTRAST_DEFAULT;
  int openhd_sharpness = OPENHD_SHARPNESS_DEFAULT;
  // libcamera params
  int rpi_libcamera_ev_value = RPI_LIBCAMERA_DEFAULT_EV;
  int rpi_libcamera_denoise_index = 0;
  int rpi_libcamera_awb_index = 0;             // 0=Auto
  int rpi_libcamera_metering_index = 0;        // 0=centre
  int rpi_libcamera_exposure_index = 0;        // 0=normal
  int rpi_libcamera_shutter_microseconds = 0;  // 0= auto

  // these are customizable settings
  // 34817 == black hot
  // actually not zoom
  int infiray_custom_control_zoom_absolute_colorpalete = 34817;
};

static bool requires_hflip(const CameraSettings& settings) {
  if (settings.openhd_flip == OPENHD_FLIP_HORIZONTAL ||
      settings.openhd_flip == OPENHD_FLIP_VERTICAL_AND_HORIZONTAL)
    return true;
  return false;
}

static bool requires_vflip(const CameraSettings& settings) {
  if (settings.openhd_flip == OPENHD_FLIP_VERTICAL ||
      settings.openhd_flip == OPENHD_FLIP_VERTICAL_AND_HORIZONTAL)
    return true;
  return false;
}
// TODO - some platforms (only) flip, some platforms rotate the full range
static int get_rotation_degree_0_90_180_270(const CameraSettings& settings) {
  if (settings.openhd_flip == OPENHD_FLIP_NONE) return 0;
  if (settings.openhd_flip == OPENHD_FLIP_HORIZONTAL) return 180;
  if (settings.openhd_flip == OPENHD_FLIP_VERTICAL_AND_HORIZONTAL) return 180;
  return 0;
}
static int get_rotation_degree_qcom(const CameraSettings& settings) {
  if (settings.openhd_flip == OPENHD_FLIP_NONE) return 0;
  if (settings.openhd_flip == OPENHD_FLIP_HORIZONTAL) return 1;
  if (settings.openhd_flip == OPENHD_FLIP_VERTICAL_AND_HORIZONTAL) return 2;
  return 0;
}

#endif
