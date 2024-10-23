/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 14:09
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : packet_dispatcher_interface.cpp
 * @description : TODO
 *******************************************************/

#include "io/packet_dispatcher_interface.h"

namespace CoreIO {
		void PacketDispatcherInterface::AddSocket(std::weak_ptr<SocketInterface> socket) {
				sockets_.push_back(socket);
		}

		PacketDispatcherInterface::~PacketDispatcherInterface() {
				for (auto it = sockets_.begin(); it != sockets_.end(); ++it) {
						auto locked = it->lock();
						if (locked) {
								locked->RemoveDispatcher(shared_from_this());
						}
				}
		}

} // namespace CoreIO