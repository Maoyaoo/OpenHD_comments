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

#ifndef OPENHD_OPENHD_UDP_H
#define OPENHD_OPENHD_UDP_H

#include <netinet/in.h>

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

//
// openhd UDP helpers
//
namespace openhd {
// Wrapper around an UDP port you can send data to
// opens port on construction, closes port on destruction
class UDPForwarder {
 public:
  explicit UDPForwarder(
      std::string client_addr1,
      int client_udp_port1);  // 接受客户端地址和 UDP 端口，打开 UDP 端口。
  UDPForwarder(const UDPForwarder &) = delete;
  UDPForwarder &operator=(const UDPForwarder &) = delete;
  ~UDPForwarder();

  // 将数据包通过 UDP 转发。
  void forwardPacketViaUDP(const uint8_t *packet, std::size_t packetSize) const;

 private:
  struct sockaddr_in saddr {};
  int sockfd;

 public:
  // 目标的 IP 地址和 UDP 端口
  const std::string client_addr;
  const int client_udp_port;
};

/**
 * Similar to UDP forwarder, but allows forwarding the same data to 0 or more
 * IP::Port tuples
 */
// 这个类允许将数据包转发到多个 IP 地址和端口。
class UDPMultiForwarder {
 public:
  explicit UDPMultiForwarder() = default;
  UDPMultiForwarder(const UDPMultiForwarder &) = delete;
  UDPMultiForwarder &operator=(const UDPMultiForwarder &) = delete;
  /**
   * Start forwarding data to another IP::Port tuple
   * 添加新的 UDP 转发目标
   */
  void addForwarder(const std::string &client_addr, int client_udp_port);
  /**
   * Remove an already existing udp forwarding instance.
   * Do nothing if such an instance is not found.
   * 移除已存在的转发目标。
   */
  void removeForwarder(const std::string &client_addr, int client_udp_port);
  /**
   * Forward data to all added IP::Port tuples via UDP
   * 将数据包转发到所有已添加的目标
   */
  void forwardPacketViaUDP(const uint8_t *packet, std::size_t packetSize);

  // 返回当前所有转发目标的列表
  [[nodiscard]] const std::list<std::unique_ptr<UDPForwarder>> &getForwarders()
      const;

 private:
  // list of host::port tuples where we send the data to.
  std::list<std::unique_ptr<UDPForwarder>> udpForwarders;
  // modifying the list of forwarders must be thread-safe
  std::mutex udpForwardersLock;
};

// Open the specified port for udp receiving
// sets SO_REUSEADDR to true if possible
// throws a runtime exception if opening the socket fails
// 这个函数打开一个 UDP 套接字用于接收数据包，并设置 SO_REUSEADDR
// 选项，允许重复使用本地地址。
static int openUdpSocketForReceiving(const std::string &address, int port);

// Set the reuse flag on the socket, so it doesn't care if there is a broken
// down process still on the socket or not.
// 设置套接字的重用标志，允许即使存在破损的进程仍能复用该套接字。
static void setSocketReuse(int sockfd);

// 此类用于接收 UDP 数据包并通过回调处理收到的数据。
class UDPReceiver {
 public:
  typedef std::function<void(const uint8_t *payload,
                             const std::size_t payloadSize)>
      OUTPUT_DATA_CALLBACK;
  static constexpr const size_t UDP_PACKET_MAX_SIZE = 65507;
  /**
   * Receive data from socket and forward it via callback until stopLooping() is
   * called
   * 构造函数：初始化 UDP 接收器，绑定到指定的客户端地址和端口，并设置回调函数。
   */
  explicit UDPReceiver(std::string client_addr, int client_udp_port,
                       OUTPUT_DATA_CALLBACK cb);
  ~UDPReceiver();

  // 开始接收数据包，直到出现错误
  void loopUntilError();
  // Now this one is kinda special - for mavsdk we need to send messages from
  // the port we are listening on to a specific IP::PORT tuple (such that the
  // source address of the then received packet matches the address we are
  // listening on).
  // 这个是有点特别的——对于 mavsdk，我们需要将从监听端口接收到的消息发送到一个特定的 IP::PORT 元组（这样接收到的包的源地址就会与我们正在监听的地址匹配）。

  // 将接收到的数据包转发到指定的目标 IP 和端口。
  void forwardPacketViaUDP(const std::string &destIp, int destPort,
                           const uint8_t *packet, std::size_t packetSize) const;
  //  停止接收数据。
  void stopLooping();
  // 用于在后台线程中运行或停止接收过程。
  void runInBackground();
  void stopBackground();

 private:
  const OUTPUT_DATA_CALLBACK mCb;
  bool receiving = true;
  int mSocket;
  std::unique_ptr<std::thread> receiverThread = nullptr;
  // Limit receive error log to every 3 seconds
  std::chrono::steady_clock::time_point m_last_receive_error_log =
      std::chrono::steady_clock::now();
  int m_last_receive_error_log_skip_count = 0;
};

// 本地回环地址 127.0.0.1。
static const std::string ADDRESS_LOCALHOST = "127.0.0.1";

// 任何 IP 地址 0.0.0.0，通常用于绑定所有可用的网络接口。
static const std::string ADDRESS_ANY = "0.0.0.0";
}  // namespace openhd

#endif  // OPENHD_OPENHD_UDP_H
