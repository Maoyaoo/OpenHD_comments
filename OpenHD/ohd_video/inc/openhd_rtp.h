//
// Created by consti10 on 17.04.24.
//

#ifndef OPENHD_OPENHD_RTP_H
#define OPENHD_OPENHD_RTP_H

#include "openhd_link.hpp"
#include "openhd_spdlog.h"
#include "rtp-payload-internal.h"

namespace openhd {

/**
 * Due to legacy reasons, we have 2 cases:
 * 1) We use gstreamer for rtp encoding - in this case, we get rtp fragments out
 * via appsink and only need to listen for the FU-E (End bit) fragment(s) to
 * properly forward a fragmented frame 2) We use gstreamer or something else for
 * h264/h265 encoding, but not rtp encoding - in this case, we get
 * TODO: Be specific on the format
 * NALUs and packetize them using lib rtp. The first
 * approach is much more reliable, but the second approach has its own
 * advantages, too.
 */
class RTPHelper {
 public:
  explicit RTPHelper();

  typedef std::function<void(
      std::vector<std::shared_ptr<std::vector<uint8_t>>> frame_fragments)>
      OUT_CB;
  void set_out_cb(RTPHelper::OUT_CB cb);

  void feed_nalu(const uint8_t* data, int data_len);

 public:
  // public due to c/c++ mix (callbacks)
  void on_new_rtp_fragment(const uint8_t* nalu, int bytes, uint32_t timestamp,
                           int last);

 private:
  OUT_CB m_out_cb = nullptr;
  rtp_payload_t m_handler{};
  void* encoder;
  std::shared_ptr<spdlog::logger> m_console;
  bool m_last_fu_s_idr = false;

 private:
  std::vector<std::shared_ptr<std::vector<uint8_t>>> m_frame_fragments;
};

class RTPFragmentBuffer {
 public:
  explicit RTPFragmentBuffer();
  void buffer_and_forward(std::shared_ptr<std::vector<uint8_t>> fragment,
                          uint64_t dts);

 public:
  bool m_enable_ultra_secure_encryption;
  bool m_is_h265 = false;
  bool m_uses_intra_refresh = false;
  int m_stream_index = 0;

 private:
  void on_new_rtp_fragmented_frame();

 private:
  std::shared_ptr<spdlog::logger> m_console;
  bool m_last_fu_s_idr = false;
  std::vector<std::shared_ptr<std::vector<uint8_t>>> m_frame_fragments;
};

}  // namespace openhd

#endif  // OPENHD_OPENHD_RTP_H
