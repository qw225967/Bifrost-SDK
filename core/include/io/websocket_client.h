/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 13:48
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : web_socket_client.h
 * @description : TODO
 *******************************************************/

#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include "io/network_thread.h"
#include "io/packet_dispatcher_interface.h"
#include "io/socket_interface.h"
#include "net/http/websocket/websocket_client.hpp"
#include "net/http/websocket/websocket_pub.hpp"
#include "utils/copy_on_write_buffer.h"
#include "utils/cpp11_adaptor.h"
#include "utils/utils.h"
#include <spdlog/spdlog.h>

#include <atomic>
#include <future>
#include <memory>

namespace CoreIO {

		using namespace cpp_streamer;

		class WebsocketClient : public WebSocketConnectionCallBackI,
		                        public TimerInterface,
		                        public SocketInterface {
		public:
				WebsocketClient(std::shared_ptr<NetworkThread> networkThread)
				    : TimerInterface(networkThread->GetLoop(), 0),
				      network_thread_(networkThread) {
						this->mem_buf_.membuf_init(128);
				}

				~WebsocketClient() {
						this->mem_buf_.membuf_uninit();
				}

				bool Init(const std::string& ip, uint16_t port,
				          const std::string& subpath, bool ssl_enable) {
						if (!closed_.load()) {
								return false;
						}

						std::cout << "------------- WebsocketClient::Init() 111" << std::endl;
						ws_client_ = Cpp11Adaptor::make_unique<WebSocketClient>(
						    network_thread_->GetLoop(), ip, port, subpath, ssl_enable, this);
						std::cout << "------------- WebsocketClient::Init() 222" << std::endl;

						closed_.store(false);
						return true;
				}

				// bool Connect(uint32_t timeout_ms = 300){
				//     if(closed_.load()|| is_connected_.load() || !ws_client_){
				//         return false;
				//     }

				//     ws_client_->AsyncConnect();

				//     SetTimeout(timeout_ms);
				//     StartTimer();

				//     return true;
				// }

				std::future<bool> ConnectInvoke(uint32_t timeout_ms = 300) {
						return network_thread_
						    ->Post<std::future<bool>>([this,
						                               timeout_ms]() -> std::future<bool> {
								    SetTimeout(timeout_ms);
								    StartTimer();

								    if (!closed_.load() && !is_connected_.load() && ws_client_) {
										    try {
												    ws_client_->AsyncConnect();
										    } catch (std::exception& e) {
												    SPDLOG_INFO("--------------------: ", e.what());
										    }
								    }

								    return connect_promise_.get_future();
						    })
						    .get();
				}

				void SendText(const std::string& text) {
						if (!ws_client_ || !is_connected_.load()) {
								return;
						}
						std::cout << "SendText" << std::endl;
						ws_client_->AsyncWriteText(text);
				}

				void SendTextInvoke(const std::string& text) {
						network_thread_->Post<void>([this, text] { SendText(text); });
				}

				void SendData(RTCUtils::CopyOnWriteBuffer data) {
						if (!ws_client_ || !is_connected_.load()) {
								return;
						}

						ws_client_->AsyncWriteData(data.data(), data.size());
				}

				void SendDataInvoke(RTCUtils::CopyOnWriteBuffer data) {
						network_thread_->Post<void>([this, data] { SendData(data); });
				}

		public:
				void OnTimer() override {
						SPDLOG_WARN("websocket connect timeout");
						StopTimer();
						if (!is_connected_.load()) {
								connect_promise_.set_value(false);
						}
				}
				void OnConnection() override {
						SPDLOG_INFO("websocket is connected ...");
						is_connected_.store(true);
						StopTimer();
						connect_future_ready_ = true;
						connect_promise_.set_value(true);
				}

				void OnClose(int code, const std::string& desc) override {
						is_connected_.store(false);
						StopTimer();
						if (!connect_future_ready_) {
								connect_promise_.set_value(false);
								connect_future_ready_ = true;
						}

						SPDLOG_INFO("websocket is closed, code:{}, desc:{}, ----- {}", code,
						            desc, uv_strerror(code));
				}

				void OnReadText(int code, const std::string& text) override {
						if (code < 0) {
								is_connected_.store(false);
								SPDLOG_ERROR("websocket read text error: {}", code);
								return;
						}

						SPDLOG_INFO("OnReadText: {}", text);
				}

				void OnReadData(int code, const uint8_t* data, size_t len) override {
						if (code < 0) {
								is_connected_.store(false);
								SPDLOG_ERROR("websocket read data error: {}", code);
								return;
						}

						this->mem_buf_.membuf_append_data(data, len);
						if (len >= 4096) {
						} else {
								std::lock_guard<std::mutex> lock(dispatchers_mutex_);
								for (auto& dispatcher : dispatchers_) {
										dispatcher->Dispatch(this->mem_buf_.GetData(),
										                     this->mem_buf_.GetLen(), this,
										                     Protocol::WEBSOCKET);
								}
								this->mem_buf_.membuf_clear(0);
						}
				}

				void Close() {
						closed_.store(true);
						is_connected_.store(false);
				}

		private:
				std::atomic<bool> is_connected_{ false };
				std::atomic<bool> closed_{ true };
				std::unique_ptr<WebSocketClient> ws_client_;
				std::shared_ptr<NetworkThread> network_thread_;
				std::promise<bool> connect_promise_;
				std::future<bool> connect_future_;
				bool connect_future_ready_{ false };

				// RTP buf
				RTCUtils::MemBuffer mem_buf_;
		};

} // namespace CoreIO

#endif