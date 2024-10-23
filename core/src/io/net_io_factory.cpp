/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-25 下午2:39
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : net_io_factory.cpp
 * @description : TODO
 *******************************************************/

#include "io/net_io_factory.h"

namespace CoreIO {
		NetIOFactory::NetIOFactory() {}
		NetIOFactory::~NetIOFactory() {}

		bool NetIOFactory::Init(){
			network_thread_ = std::make_shared<NetworkThread>();
			if(!network_thread_->Start()){
				return false;
			}
			is_closed_.store(false);
			return true;
		}

		std::shared_ptr<UdpSocket> NetIOFactory::CreateUdpSocket(std::string ip, uint16_t port){
			SPDLOG_INFO("create udp socket, ip = {}, port = {}", ip, port);
			if (is_closed_.load()) {
				SPDLOG_ERROR("net io factory is closed");
				return nullptr;
			}

			return network_thread_->Post<std::shared_ptr<UdpSocket>>([this, ip, port]() -> std::shared_ptr<UdpSocket> {
				Type type = (ip != "" && port != 0) ? Type::SERVER : Type::CLIENT;
				auto udp_socket = std::make_shared<UdpSocket>(network_thread_, type, ip, port);
				if(udp_socket->Init()){
					return udp_socket;
				}
				return nullptr;
			}).get();
		}

		void NetIOFactory::DestroyUdpSocket(std::shared_ptr<UdpSocket> socket){
			network_thread_->Post<void>([this, socket]() { socket->Close(); }).get();
		}

		std::shared_ptr<WebsocketClient> NetIOFactory::CreateWebsocketClient(std::string ip, uint16_t port, const std::string& subpath, bool ssl_enable){
			if (is_closed_.load()) {
				SPDLOG_ERROR("net io factory is closed");
				return nullptr;
			}

			return network_thread_->Post<std::shared_ptr<WebsocketClient>>([this, ip, port, subpath, ssl_enable]() -> std::shared_ptr<WebsocketClient> {
				auto websocket_client_socket = std::make_shared<WebsocketClient>(network_thread_);
				std::cout<<"NetIOFactory::CreateWebsocketClient 111"<<std::endl;
				if(websocket_client_socket->Init(ip, port, subpath, ssl_enable)){
					std::cout<<"NetIOFactory::CreateWebsocketClient 222"<<std::endl;
					return websocket_client_socket;
				}
				return nullptr;
			}).get();
		}
		
		void NetIOFactory::DestroyWebsocketClient(std::shared_ptr<WebsocketClient> socket){
			network_thread_->Post<void>([this, socket]() { socket->Close(); }).get();
		}


} // namespace CoreIO