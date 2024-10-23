/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 13:48
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : web_socket_client.h
 * @description : TODO
 *******************************************************/

#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include <stdint.h>

#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include "io/network_thread.h"
#include "io/packet_dispatcher_interface.h"
#include "io/socket_interface.h"
#include "uv.h"
#include "ws_common.h"
#include "ws_util.h"
#include "utils/copy_on_write_buffer.h"

namespace CoreIO {
		/**
		 * @brief  客户端的状态
		 */
		typedef enum WebsocketClientStatus {
				HANDSHAKING, /// 握手中
				WORKING,     /// 工作中
		} WebsocketClientStatus;

		class WebsocketClient : public SocketInterface {
		public:
				class Listener {
				public:
						virtual void OnConnect(Protocol protocol) = 0;

						virtual void OnClose(Protocol protocol) = 0;
				};

				struct UvWriteData {
						explicit UvWriteData(size_t store_size) {
								store = new uint8_t[store_size];
						}

						UvWriteData(const UvWriteData&) = delete;

						~UvWriteData() {
								delete[] store;
						}

						uv_write_t req;
						uint8_t* store{ nullptr };
				};

		public:
				WebsocketClient(std::shared_ptr<NetworkThread> network_thread,
				                const std::string& ip,
				                uint16_t port);

				~WebsocketClient();

				WebsocketClient(const WebsocketClient&) = delete;

				WebsocketClient& operator=(const WebsocketClient&) = delete;

				void AddListener(WebsocketClient::Listener* listener);

				void RemoveListener(WebsocketClient::Listener* listener);

				bool GetConnected() {
						return connected_.load();
				}

				std::string GetClientKey() {
						return client_key_;
				}

				void SetClientKey(const std::string& client_key) {
						client_key_ = client_key;
				}

				std::string GetUri() {
						return uri_;
				}

				uint16_t GetPort() {
						return port_;
				}

				std::string GetIp() {
						return ip_;
				}

				WebsocketClientStatus GetWebsocketClientStatus() {
						return status_;
				}

				void SetWebsocketClientStatus(WebsocketClientStatus status) {
						status_ = status;
				}

				bool Init();

				bool InitInvoke();

				void Close();

				void CloseInvoke();

				void Send(RTCUtils::CopyOnWriteBuffer data1,
				          RTCUtils::CopyOnWriteBuffer data2
				          = RTCUtils::CopyOnWriteBuffer());

				void SendInvoke(RTCUtils::CopyOnWriteBuffer data1,
				                RTCUtils::CopyOnWriteBuffer data2
				                = RTCUtils::CopyOnWriteBuffer());

				void OnUvAlloc(size_t suggested_size, uv_buf_t* buf);

				void OnUvConnect();

				void OnUvError();

				void OnUvRead(websocket_handle* handle,
				              uv_stream_t* uv_stream,
				              void* buffer,
				              size_t buffer_len,
				              uchar type);

				void OnUvWrite(int status);

		private:
				std::shared_ptr<NetworkThread> network_thread_{ nullptr };

				std::string ip_;
				uint16_t port_;
				std::string uri_;
				std::string client_key_; /// 客户端的key

				uv_tcp_t* uv_handle_{ nullptr };

				uint8_t* read_buffer_{ nullptr };
				size_t buffer_data_len_{ 0u };
				uv_connect_t connect_req_;
				struct sockaddr_storage remote_addr_;
				bool has_error_{ false };
				std::mutex listeners_mutex_;
				std::set<Listener*> listeners_;
				std::atomic<bool> connected_{ false };

				WebsocketClientStatus status_; // 客户端状态
		};

} // namespace CoreIO

#endif //WEB_SOCKET_H
