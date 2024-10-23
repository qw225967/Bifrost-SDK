/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 13:48
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : udp_socket.h
 * @description : TODO
 *******************************************************/

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H
#include <stdint.h>

#include <atomic>
#include <memory>
#include <set>
#include <string>

#include "io/network_thread.h"
#include "io/packet_dispatcher_interface.h"
#include "io/socket_interface.h"
#include "uv.h"
#include "utils/copy_on_write_buffer.h"

namespace CoreIO {
		class UdpSocket : public SocketInterface {
		public:
				struct UvSendData {
						explicit UvSendData(size_t store_size)
						    : store_(new uint8_t[store_size]) {
						}

						UvSendData(const UvSendData&) = delete;

						~UvSendData() {
								delete[] store_;
						}

						uv_udp_send_t req_;
						uint8_t* store_;
				};

		public:
				UdpSocket(std::shared_ptr<NetworkThread> network_thread,
				          Type type,
				          const std::string& ip = "",
				          uint16_t port         = 0);

				~UdpSocket();

				UdpSocket(const UdpSocket&) = delete;

				UdpSocket& operator=(const UdpSocket&) = delete;

				bool Init();

				bool InitInvoke();

				void Close();

				void CloseInvoke();

				void Send(RTCUtils::CopyOnWriteBuffer buf, const struct sockaddr* addr);

				void SendInvoke(RTCUtils::CopyOnWriteBuffer buf,
				                const struct sockaddr* addr);

				void OnUvRecvAlloc(size_t, uv_buf_t* buf);

				void OnUvRecv(ssize_t n_read,
				              const uv_buf_t* buf,
				              const struct sockaddr* addr,
				              unsigned int flags);

				void OnUvSend(int status);

		private:
				std::shared_ptr<NetworkThread> network_thread_{ nullptr };

				Type type_;
				std::string ip_;
				uint16_t port_;
				uv_udp_t* uv_handle_{ nullptr };
				uint8_t* read_buffer_{ nullptr };
				std::atomic<bool> closed_{ false };

				struct sockaddr_storage local_addr_;
		};
} // namespace CoreIO
#endif // UDP_SOCKET_H
