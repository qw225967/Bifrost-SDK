/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 14:08
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : packet_dispatcher_interface.h
 * @description : TODO
 *******************************************************/

#ifndef PACKET_DISPATCHER_INTERFACE_H
#define PACKET_DISPATCHER_INTERFACE_H
#include <uv.h>
#include <stdint.h>

#include <vector>

#include "io/socket_interface.h"

namespace CoreIO {
		class PacketDispatcherInterface
		    : public std::enable_shared_from_this<PacketDispatcherInterface> {
		public:
				PacketDispatcherInterface() = default;

				virtual ~PacketDispatcherInterface();

				void AddSocket(std::weak_ptr<SocketInterface> socket);

		public:
				virtual void Dispatch(uint8_t* data,
				                      size_t len,
				                      SocketInterface* socket,
				                      Protocol protocol,
				                      const struct sockaddr* addr = nullptr)
				    = 0;

		private:
				std::vector<std::weak_ptr<SocketInterface>> sockets_;
		};
} // namespace CoreIO
#endif // PACKET_DISPATCHER_INTERFACE_H
