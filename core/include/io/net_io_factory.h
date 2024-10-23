/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-25 下午2:39
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : net_io_factory.h
 * @description : TODO
 *******************************************************/

#ifndef NET_IO_FACTORY_H
#define NET_IO_FACTORY_H

#include "udp_socket.h"
#include "websocket_client.h"
#include <string>

namespace CoreIO {
		class NetIOFactory {
		public:
				NetIOFactory();
				~NetIOFactory();
				NetIOFactory(NetIOFactory&)            = delete;
				NetIOFactory& operator=(NetIOFactory&) = delete;

				bool Init();
				std::shared_ptr<NetworkThread> GetNetworkThread() { return network_thread_; }

				std::shared_ptr<UdpSocket> CreateUdpSocket(std::string ip = "", uint16_t port = 0);
				void DestroyUdpSocket(std::shared_ptr<UdpSocket> socket);

				std::shared_ptr<WebsocketClient> CreateWebsocketClient(std::string ip, uint16_t port, const std::string& subpath, bool ssl_enable);
				void DestroyWebsocketClient(std::shared_ptr<WebsocketClient> socket);
			
		private:
				std::atomic<bool> is_closed_{ true };
				std::shared_ptr<NetworkThread> network_thread_;
		};

} // namespace CoreIO

#endif // NET_IO_FACTORY_H
