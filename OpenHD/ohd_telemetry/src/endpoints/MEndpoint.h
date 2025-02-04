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

#ifndef XMAVLINKSERVICE_MENDPOINT_H
#define XMAVLINKSERVICE_MENDPOINT_H

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>

#include "../mav_helper.h"
#include "../mav_include.h"
#include "openhd_spdlog.h"

// Mavlink Endpoint
// A Mavlink endpoint hides away the underlying connection - e.g. UART, TCP, WB.
// It has a (implementation-specific) method to send messages (sendMessage) and
// (implementation-specific) continuously forwards new incoming messages via a
// callback. It MUST also hide away any problems that could exist with this
// endpoint - e.g. a disconnecting UART. If (for example) in case of UART the
// connection is lost, it should just try to reconnect and as soon as the
// connection has been re-established, continue working as if nothing happened.
// This "send/receive data when possible, otherwise do nothing" behaviour fits
// well with the mavlink paradigm: https://mavlink.io/en/services/heartbeat.html
// "A component is considered to be connected to the network if its HEARTBEAT
// message is regularly received, and disconnected if a number of expected
// messages are not received."
// => A endpoint is considered alive it has received any mavlink messages in the
// last X seconds.
// Mavlink 端点
// Mavlink 端点隐藏了底层连接（例如 UART、TCP、WB）。
// 它有一个（实现特定的）方法来发送消息（sendMessage），
// 并且（实现特定的）通过回调持续转发新接收到的消息。
// 它还必须隐藏此端点可能存在的任何问题（例如 UART 断开连接）。
// 如果（例如）UART 连接丢失，它应该尝试重新连接，并在连接重新建立后，
// 继续工作，就像什么都没发生一样。
// 这种“在可能的情况下发送/接收数据，否则什么都不做”的行为非常符合 MAVLink 范式：
// https://mavlink.io/en/services/heartbeat.html
// “如果一个组件的心跳消息被定期接收，则认为它已连接到网络；
// 如果未接收到预期的消息，则认为它已断开连接。”
// => 如果端点在最近 X 秒内接收到任何 MAVLink 消息，则认为它是存活的。
class MEndpoint {
   public:
    /**
     * The implementation-specific constructor SHOULD try and establish a
     * connection as soon as possible And re-establish the connection when
     * disconnected.
     * 实现特定的构造函数应尽快尝试建立连接，并在断开连接时重新建立连接。
     * @param tag a tag for debugging.
     * @param mavlink_channel the mavlink channel to use for parsing.
     */
    explicit MEndpoint(std::string tag, bool debug_mavlink_msg_packet_loss = false);

    /**
     * send one or more messages via this endpoint.
     * If the endpoint is silently disconnected, this MUST NOT FAIL/CRASH.
     * This calls the underlying implementation's sendMessageImpl() function (pure
     * virtual) and increases the sent message count
     * @param messages the messages to send
     */
    /**
     * 通过此端点发送一条或多条消息。
     * 如果端点被悄无声息地断开连接，这绝不能导致失败或崩溃。
     * 该方法调用底层实现的 sendMessageImpl() 函数（纯虚函数），并增加已发送的消息计数。
     * @param messages 要发送的消息
     */
    void sendMessages(const std::vector<MavlinkMessage>& messages);

    /**
     * register a callback that is called every time
     * this endpoint has received a new message
     * @param cb the callback function to register that is then called with a
     * message every time a new full mavlink message has been parsed
     */
    /**
     * 注册一个回调函数，该回调函数将在每次此端点接收到新消息时被调用。
     * @param cb 要注册的回调函数，该函数将在每次解析到新的完整 mavlink 消息时被调用，并传入该消息。
     */
    void registerCallback(MAV_MSG_CALLBACK cb);

    /**
     * If (for some reason) you need to reason if this endpoint is alive, just
     * check if it has received any mavlink messages in the last X seconds
     */
    /**
     * 如果（由于某种原因）你需要判断此端点是否仍然存活，
     * 只需检查它在过去 X 秒内是否接收到任何 mavlink 消息。
     */
    [[nodiscard]] bool isAlive() const;

    /**
     * @return info about this endpoint, for debugging
     */
    /**
     * @return 该端点的相关信息，用于调试。
     */
    [[nodiscard]] std::string createInfo() const;

    // can be public since immutable
    // 由于是不可变的，因此可以是公共的
    const std::string TAG;

   protected:
    // parse new data as it comes in, extract mavlink messages and forward them on
    // the registered callback (if it has been registered)
    // 在数据到达时解析新数据，提取 mavlink 消息并转发到已注册的回调函数（如果已注册）
    void parseNewData(const uint8_t* data, int data_len);

    // this one is special, since mavsdk in this case has already done the message
    // parsing
    // 这个是特别的，因为在这种情况下，mavsdk 已经完成了消息解析
    void parseNewDataEmulateForMavsdk(mavlink_message_t msg) { onNewMavlinkMessages({MavlinkMessage{msg}}); }

    // Must be overridden by the implementation
    // Returns true if the message(s) have been properly sent (e.g. a connection
    // exists on connection-based endpoints) false otherwise
    // 必须由实现类重写
    // 如果消息已正确发送（例如，在基于连接的端点上存在连接），则返回 true；否则返回 false
    virtual bool sendMessagesImpl(const std::vector<MavlinkMessage>& messages) = 0;

   private:
    MAV_MSG_CALLBACK m_callback = nullptr;
    // increases message count and forwards the messages via the callback if
    // registered.
    // 增加消息计数，并在回调已注册的情况下通过回调转发消息。
    void onNewMavlinkMessages(std::vector<MavlinkMessage> messages);
    mavlink_status_t receiveMavlinkStatus{};
    const uint8_t m_mavlink_channel;
    std::chrono::steady_clock::time_point lastMessage{};
    int m_n_messages_received = 0;
    // sendMessage() might be called by different threads.
    // sendMessage() 可能会被不同的线程调用。
    std::atomic<int> m_n_messages_sent = 0;
    std::atomic<int> m_n_messages_send_failed = 0;

    // I think mavlink channels are static, so each endpoint should use his own
    // channel. Based on mavsdk::mavlink_channels It is not clear what the limit
    // of the number of channels is, except UINT8_MAX.
    // 我认为 mavlink 通道是静态的，因此每个端点应该使用自己的通道。
    // 根据 mavsdk::mavlink_channels，目前并不清楚通道的数量限制是多少，除了 UINT8_MAX。
    static int checkoutFreeChannel();

   private:
    // Used to measure incoming / outgoing bits per second
    // 用于测量每秒传入/传出位数
    int m_tx_n_bytes = 0;
    int m_rx_n_bytes = 0;

   private:
    const bool m_debug_mavlink_msg_packet_loss;
    mavlink_status_t m_last_status;
};

#endif  // XMAVLINKSERVICE_MENDPOINT_H
