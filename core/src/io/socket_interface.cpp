/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 14:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : socket_interface.cpp
 * @description : TODO
 *******************************************************/

#include "io/socket_interface.h"

#include "io/packet_dispatcher_interface.h"

namespace CoreIO {
		bool SocketInterface::AddDispatcher(
		    const std::shared_ptr<PacketDispatcherInterface>& dispatcher) {
				SPDLOG_TRACE();

				std::unique_lock<std::mutex> lock(dispatchers_mutex_);
				bool inserted;
				std::tie(std::ignore, inserted) = dispatchers_.insert(dispatcher);
				if (inserted) {
						dispatcher->AddSocket(shared_from_this());
				}

				return inserted;
		}

		void SocketInterface::RemoveDispatcher(
		    const std::shared_ptr<PacketDispatcherInterface>& dispatcher) {
				SPDLOG_TRACE();

				std::unique_lock<std::mutex> lock(dispatchers_mutex_);
				dispatchers_.erase(dispatcher);
		}

} // namespace CoreIO