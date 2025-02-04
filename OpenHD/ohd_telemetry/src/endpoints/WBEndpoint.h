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

#ifndef OPENHD_OPENHD_OHD_TELEMETRY_SRC_ENDPOINTS_WBENDPOINT_H_
#define OPENHD_OPENHD_OHD_TELEMETRY_SRC_ENDPOINTS_WBENDPOINT_H_

#include "MEndpoint.h"
#include "openhd_link.hpp"

// Abstraction for sending / receiving data on/from the link between air and
// ground unit
class WBEndpoint : public MEndpoint {
   public:
    //  std::shared_ptr<OHDLink> link：这是一个智能指针（std::shared_ptr），它指向一个 OHDLink 对象。OHDLink 可能代表空中与地面之间的通信链路
    explicit WBEndpoint(std::shared_ptr<OHDLink> link, std::string TAG);
    ~WBEndpoint();

   private:
    std::shared_ptr<OHDLink> m_link_handle;  // 指向一个 OHDLink 对象。这个指针用来存储和管理空中与地面之间通信链路的实例
    bool sendMessagesImpl(const std::vector<MavlinkMessage>& messages) override;  // 实现消息的发送功能
    std::mutex m_send_messages_mutex;  // 这是一个互斥锁，用于在多线程环境下同步对发送消息操作的访问。保证在多线程环境中不会发生竞争条件。
};

#endif  // OPENHD_OPENHD_OHD_TELEMETRY_SRC_ENDPOINTS_WBENDPOINT_H_
