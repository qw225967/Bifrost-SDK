/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 14:04
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : socket_interface.h
 * @description : TODO
 *******************************************************/

#ifndef SOCKET_INTERFACE_H
#define SOCKET_INTERFACE_H
#include <atomic>
#include <set>
#include <mutex>

#include "spdlog/spdlog.h"

namespace CoreIO {
		class PacketDispatcherInterface;

		enum class Protocol { UDP = 1, TCP = 2, WEBSOCKET = 3 };

		enum class Type { CLIENT = 1, SERVER };

		class SocketInterface : public std::enable_shared_from_this<SocketInterface> {
		public:
				bool AddDispatcher(
				    const std::shared_ptr<PacketDispatcherInterface>& dispatcher);

				void RemoveDispatcher(
				    const std::shared_ptr<PacketDispatcherInterface>& dispatcher);

				bool IsClosed() const {
						return closed_.load();
				}

		protected:
				std::mutex dispatchers_mutex_;
				std::set<std::shared_ptr<PacketDispatcherInterface>> dispatchers_;
				std::atomic<bool> closed_;
		};
} // namespace CoreIO
#endif // SOCKET_INTERFACE_H
