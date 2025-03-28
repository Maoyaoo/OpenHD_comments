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

#ifndef OPENHD_OPENHD_OHD_VIDEO_INC_GST_BITRATE_CONTROLL_WRAPPER_H_
#define OPENHD_OPENHD_OHD_VIDEO_INC_GST_BITRATE_CONTROLL_WRAPPER_H_

#include <gst/gst.h>

#include <optional>

#include "openhd_spdlog.h"
#include "openhd_spdlog_include.h"

// #define EXPERIMENTAL_USE_OPENH264_ENCODER

// Bitrate is one of the few params we want to support changing dynamically at
// run time without the need for a pipeline restart. This just wraps the
// differences for those pipelines nicely
// 比特率是我们希望在运行时动态更改的少数几个参数之一，而无需重启管道。这个功能只是为了方便地封装这些管道的差异。
struct GstBitrateControlElement {
    // Some elements take kbit/s, some take bit/s
    bool takes_kbit = false;
    // the encoder (or similar) element, must not be null
    GstElement* encoder;
    // Not all encoders / elements call the bitrate property "bitrate"
    std::string property_name = "bitrate";
};

// 从给定的 GStreamer 管道中获取一个动态比特率控制元素
static std::optional<GstBitrateControlElement> get_dynamic_bitrate_control_element_in_pipeline(GstElement* gst_pipeline, const CameraHolder& camera_holder) {
    auto camera = camera_holder.get_camera();
    auto settings = camera_holder.get_settings();
    GstBitrateControlElement ret{};
    ret.encoder = nullptr;
    if (camera.requires_rpi_mmal_pipeline()) {
        ret.encoder = gst_bin_get_by_name(GST_BIN(gst_pipeline), "rpicamsrc");
        ret.property_name = "bitrate";
        ret.takes_kbit = false;
    } else if (camera.camera_type == X_CAM_TYPE_DUMMY_SW || is_usb_camera(camera.camera_type) || settings.force_sw_encode) {
        ret.encoder = gst_bin_get_by_name(GST_BIN(gst_pipeline), "swencoder");
        ret.property_name = "bitrate";
#ifdef EXPERIMENTAL_USE_OPENH264_ENCODER
        ret.takes_kbit = false;
#else
        ret.takes_kbit = true;
#endif
    } else if (camera.requires_x20_cedar_pipeline()) {
        // We can change bitrate dynamically
        ret.encoder = gst_bin_get_by_name(GST_BIN(gst_pipeline), "sunxisrc");
        ret.property_name = "bitrate";
        ret.takes_kbit = true;
    }
    if (ret.encoder == nullptr) {
        openhd::log::get_default()->debug("Cannot find dynamic bitrate control element for camera {}", camera.cam_type_as_verbose_string());
        return std::nullopt;
    }
    // try fetching the value for testing if it actually works
    gint actual_bits_per_second = -1;
    g_object_get(ret.encoder, ret.property_name.c_str(), &actual_bits_per_second, NULL);
    if (actual_bits_per_second == -1) {
        openhd::log::get_default()->warn("dynamic bitrate control element doesn't work");
        return std::nullopt;
    }
    openhd::log::get_default()->info("Got bitrate control for camera {}, current:{}", camera.cam_type_as_verbose_string(), actual_bits_per_second);
    return ret;
}

static bool change_bitrate(const GstBitrateControlElement& ctrl_el, int bitrate_kbits) {
    const auto bitrate = ctrl_el.takes_kbit ? bitrate_kbits : openhd::kbits_to_bits_per_second(bitrate_kbits);
    g_object_set(ctrl_el.encoder, ctrl_el.property_name.c_str(), bitrate, NULL);
    gint actual_bits_per_second = -1;
    g_object_get(ctrl_el.encoder, ctrl_el.property_name.c_str(), &actual_bits_per_second, NULL);
    if (actual_bits_per_second != bitrate) {
        openhd::log::get_default()->warn("Cannot change bitrate to {}kbit/s, got {}kBit/s", bitrate_kbits, actual_bits_per_second);
        return false;
    }
    openhd::log::get_default()->debug("Changed bitrate to {} kbit/s", bitrate_kbits);
    return true;
}

static void unref_bitrate_element(GstBitrateControlElement& element) {
    if (element.encoder) {
        openhd::log::get_default()->debug("Unref bitrate control element begin");
        gst_object_unref(element.encoder);
        element.encoder = nullptr;
        openhd::log::get_default()->debug("Unref bitrate control element end");
    }
}

static std::chrono::nanoseconds convert_ts(uint64_t dts) {
    return std::chrono::nanoseconds(dts);
}
static std::chrono::nanoseconds calculate_delta(uint64_t dts) {
    return convert_ts(std::chrono::steady_clock::now().time_since_epoch().count() - dts);
}

#endif  // OPENHD_OPENHD_OHD_VIDEO_INC_GST_BITRATE_CONTROLL_WRAPPER_H_
