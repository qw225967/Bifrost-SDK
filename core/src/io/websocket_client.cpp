/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 13:49
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : web_socket_client.cpp
 * @description : TODO
 *******************************************************/

#include "io/websocket_client.h"

#include "io/packet_dispatcher_interface.h"
#include "spdlog/spdlog.h"
#include "ws_common.h"
#include "ws_util.h"
#include "utils/utils.h"

namespace CoreIO {
		static constexpr size_t kReadBufferSize{ 65536 };

		static char OnClose(uv_stream_t* uv_stream) {
				if (!uv_stream->loop->data) {
						auto* websocket_client
						    = static_cast<WebsocketClient*>(uv_stream->loop->data);
						websocket_client->Close();
				}
				return 0;
		}

		static void OnAlloc(uv_handle_t* uv_handle,
		                    size_t suggested_size,
		                    uv_buf_t* buf) {
				SPDLOG_TRACE();
				if (auto* socket = static_cast<WebsocketClient*>(uv_handle->data)) {
						socket->OnUvAlloc(suggested_size, buf);
				}
		}

		// on_read_WebSocket
		static void OnReadWebsocket(WebsocketClient* websocket_client,
		                            websocket_handle* ws_handle,
		                            uv_stream_t* uv_stream,
		                            char* data,
		                            size_t len) {
				char* data_ptr         = data;
				size_t have_read_count = 0L;
				do {
						try_parse_protocol(ws_handle, data_ptr, len - have_read_count);
						if (ws_handle->parse_state == PARSE_COMPLETE) {
								// 正常解析到了数据包
								switch (ws_handle->type) {
								case 0: // 0x0表示附加数据帧
								{
										if (ws_handle->is_eof == 0) {
												// 这是中间的分片
												SPDLOG_DEBUG("continuous fragment, len = %u",
												             hd->protoBuf.size);
												membuf_append_data(&ws_handle->merge_buf,
												                   ws_handle->proto_buf.data,
												                   ws_handle->proto_buf.size);
										} else {
												// 最后一个分片
												SPDLOG_DEBUG("last fragment, len = %u",
												             hd->protoBuf.size);
												membuf_append_data(&ws_handle->merge_buf,
												                   ws_handle->proto_buf.data,
												                   ws_handle->proto_buf.size);
												// 接收数据回调
												websocket_client->OnUvRead(ws_handle,
												                           uv_stream,
												                           ws_handle->merge_buf.data,
												                           ws_handle->merge_buf.size,
												                           ws_handle->merge_buf.flag);
										}
										break;
								}
								case 1: // 0x1表示文本数据帧
								{
										SET_FLAG(ws_handle->proto_buf.flag, FLAG_WEBSOCKET_TEXT);
										if (ws_handle->is_eof == 0) {
												// 这是第一个分片
												SPDLOG_DEBUG("first text fragment, len = %u",
												             hd->protoBuf.size);
												membuf_clear(&ws_handle->merge_buf,
												             0); // 清理原来的缓冲数据
												SET_FLAG(ws_handle->merge_buf.flag, FLAG_WEBSOCKET_TEXT);
												membuf_append_data(&ws_handle->merge_buf,
												                   ws_handle->proto_buf.data,
												                   ws_handle->proto_buf.size);
										} else {
												// 就一个分片
												SPDLOG_DEBUG("one text datapacket, len = %u",
												             hd->protoBuf.size); // 接收数据回调
												websocket_client->OnUvRead(ws_handle,
												                           uv_stream,
												                           ws_handle->proto_buf.data,
												                           ws_handle->proto_buf.size,
												                           ws_handle->proto_buf.flag);
										}
										break;
								}
								case 2: // 0x2表示二进制数据帧
								{
										SET_FLAG(ws_handle->proto_buf.flag, FLAG_WEBSOCKET_BIN);
										if (ws_handle->is_eof == 0) {
												// 这是第一个分片
												SPDLOG_DEBUG("first bin fragment, len = %u",
												             hd->protoBuf.size);
												membuf_clear(&ws_handle->merge_buf,
												             0); // 清理原来的缓冲数据
												SET_FLAG(ws_handle->merge_buf.flag, FLAG_WEBSOCKET_BIN);
												membuf_append_data(&ws_handle->merge_buf,
												                   ws_handle->proto_buf.data,
												                   ws_handle->proto_buf.size);
										} else {
												// 就一个分片
												SPDLOG_DEBUG("one bin datapacket, len = %u",
												             hd->protoBuf.size); // 接收数据回调
												websocket_client->OnUvRead(ws_handle,
												                           uv_stream,
												                           ws_handle->proto_buf.data,
												                           ws_handle->proto_buf.size,
												                           ws_handle->proto_buf.flag);
										}
										break;
								}
								case 3:
								case 4:
								case 5:
								case 6:
								case 7: // 0x3 - 7暂时无定义，为以后的非控制帧保留
										// DONOTHIND
										break;
								case 8: // 0x8表示连接关闭
								{
										SPDLOG_DEBUG("received websocket control frame : close");
										RTCUtils::NetTools::TwCloseClient(uv_stream, OnClose);
										break;
								}
								case 9: // 0x9表示ping
								{
										SPDLOG_DEBUG("received websocket control frame : ping");
										uchar* ping_packet = generate_pong_control_frame();
										websocket_client->Send(
										    RTCUtils::CopyOnWriteBuffer(ping_packet, 2));
										break;
								}
								case 10: // 0xA表示pong
								default: // 0xB - F暂时无定义，为以后的控制帧保留
										break;
								}
						} else if (ws_handle->parse_state == PARSE_FAILED) {
								// 解析错误，关闭连接
								// RTCUtils::TwCloseClient(handle, OnClose);
						} else {
								// 未读完一个完整的包,等待下回继续读
								// DONOHTING
								// break;
						}
						have_read_count += ws_handle->read_count;
						data_ptr += ws_handle->read_count;
				} while (have_read_count < len);
		}

		static void OnRead(uv_stream_t* uv_stream,
		                   ssize_t n_read,
		                   const uv_buf_t* buf) {
				SPDLOG_TRACE();

				// 获取websocket句柄
				auto* ws_handle = static_cast<websocket_handle*>(uv_stream->data);
				// 异常处理  TODO:释放申请的内存
				if (ws_handle) {
						ws_handle = static_cast<websocket_handle*>(
						    calloc(1, sizeof(websocket_handle)));
						ws_handle->endpoint_type = ENDPOINT_CLIENT;
						ws_handle->parse_state   = PARSE_COMPLETE;
						uv_stream->data          = ws_handle;
				}

				// 缓冲区buffer确认
				if (nullptr == ws_handle->proto_buf.data) {
						membuf_init(&ws_handle->proto_buf, 128);
				}
				if (nullptr == ws_handle->merge_buf.data) {
						membuf_init(&ws_handle->merge_buf, 128);
				}

				// 获取上下文
				void* ctx = uv_stream->loop->data;
				if (!ctx) {
						return;
				}

				// 根据上下文中的状态进行读操作
				auto* websocket_client = static_cast<WebsocketClient*>(ctx);
				if (n_read > 0) {
						switch (websocket_client->GetWebsocketClientStatus()) {
						// 还在握手阶段接到数据证明通了，切位工作状态
						case HANDSHAKING:
						{
								char* key = RTCUtils::NetTools::ParseHttpHeads(
								    buf, "Sec-WebSocket-Accept:");
								if (key && strlen(key) > 0) {
										SPDLOG_DEBUG("handshake response");

										websocket_client->SetWebsocketClientStatus(WORKING);
								}
								break;
						}
						case WORKING:
						{
								OnReadWebsocket(
								    websocket_client, ws_handle, uv_stream, buf->base, n_read);
								break;
						}
						}
				} else if (n_read == 0) {
						SPDLOG_DEBUG(
						    "libuv requested a buffer through the alloc callback, but "
						    "then decided that it didn't need that buffer");
				} else {
						if (n_read != UV_EOF) {
								char err_str[60] = { 0 };
								sprintf(err_str,
								        "%d:%s,%s",
								        static_cast<int>(n_read),
								        uv_err_name(static_cast<int>(n_read)),
								        uv_strerror(static_cast<int>(n_read)));
								websocket_client->OnUvError();
						} else {
								// LOG_WARN("the remote endpoint is offline");
						}
						// RTCUtils::TwCloseClient(handle, OnClose);
						// TODO:重连
				}
				// 每次使用完要释放
				if (buf->base) {
						free(buf->base);
				}
		}

		static void OnWrite(uv_write_t* uv_write, int status) {
				SPDLOG_TRACE();

				auto* writeData
				    = static_cast<WebsocketClient::UvWriteData*>(uv_write->data);
				auto* handle = uv_write->handle;
				if (auto* websocket_client = static_cast<WebsocketClient*>(handle->data))
				{
						websocket_client->OnUvWrite(status);
				}

				delete writeData;
		}

		static void OnCloseTcp(uv_handle_t* uv_handle) {
				SPDLOG_TRACE();
				delete reinterpret_cast<uv_tcp_t*>(uv_handle);
		}

		static void OnCloseShutdown(uv_handle_t* uv_handle) {
				SPDLOG_TRACE();
				delete reinterpret_cast<uv_shutdown_t*>(uv_handle);
		}

		static void OnShutdown(uv_shutdown_t* uv_shutdown, int /*status*/) {
				SPDLOG_TRACE();

				auto* uv_stream = uv_shutdown->handle;
				delete uv_shutdown;

				uv_close(reinterpret_cast<uv_handle_t*>(uv_stream), OnCloseShutdown);
		}

		static void OnConnect(uv_connect_t* uv_connection, int status) {
				SPDLOG_TRACE();

				if (0 == status) {
						if (auto* websocket_client = static_cast<WebsocketClient*>(
						        uv_connection->handle->loop->data))
						{
								// TODO 重连尝试

								// 初始化句柄data TODO:释放申请的内存
								auto* hd = static_cast<websocket_handle*>(
								    calloc(1, sizeof(websocket_handle)));
								hd->endpoint_type           = ENDPOINT_CLIENT;
								uv_connection->handle->data = hd;

								// Websocket握手
								char random[16];
								get_random(random, 16);

								// 内部申请地址 需要释放
								char* key = sha1_and_base64(random, 16);
								websocket_client->SetClientKey(key);

								// 内部申请地址 需要释放
								char* handshake_packet = generate_websocket_client_handshake(
								    websocket_client->GetIp().c_str(),
								    websocket_client->GetPort(),
								    websocket_client->GetUri().c_str(),
								    websocket_client->GetClientKey().c_str());

								// 发送握手包
								websocket_client->Send(RTCUtils::CopyOnWriteBuffer(
								    handshake_packet, strlen(handshake_packet)));
								websocket_client->SetWebsocketClientStatus(HANDSHAKING);

								// 释放
								free(handshake_packet);
								free(key);
						}
				} else {
						SPDLOG_ERROR("websocket connect failed!");
						return;
				}

				// 注册读事件
				auto* socket = static_cast<WebsocketClient*>(uv_connection->data);
				if (socket) {
						socket->OnUvConnect();
				}
		}

		WebsocketClient::WebsocketClient(std::shared_ptr<NetworkThread> network_thread,
		                                 const std::string& ip,
		                                 uint16_t port)
		    : network_thread_(network_thread), ip_(ip), port_(port),
		      read_buffer_(new uint8_t[kReadBufferSize]) {
				SPDLOG_TRACE();
		}

		WebsocketClient::~WebsocketClient() {
				SPDLOG_TRACE();

				if (!closed_.load()) {
						Close();
				}

				delete[] read_buffer_;
		}

		bool WebsocketClient::Init() {
				SPDLOG_TRACE();

				switch (const int family = RTCUtils::NetTools::GetFamily(ip_)) {
				case AF_INET:
				{
						const int err = uv_ip4_addr(
						    ip_.c_str(),
						    port_,
						    reinterpret_cast<struct sockaddr_in*>(&remote_addr_));
						if (err != 0) {
								SPDLOG_ERROR("uv_ip4_addr() failed: {}", uv_strerror(err));
								return false;
						}
						break;
				}
				case AF_INET6:
				{
						const int err = uv_ip6_addr(
						    ip_.c_str(),
						    port_,
						    reinterpret_cast<struct sockaddr_in6*>(&remote_addr_));
						if (err != 0) {
								SPDLOG_ERROR("uv_ip6_addr() failed: {}", uv_strerror(err));
								return false;
						}
						break;
				}
				default:
				{
						SPDLOG_ERROR("unknown IP family");
						return false;
				}
				}

				uv_handle_        = new uv_tcp_t();
				uv_handle_->data  = static_cast<void*>(this);
				connect_req_.data = static_cast<void*>(this);

				int err = uv_tcp_init(network_thread_->GetLoop(), uv_handle_);
				if (err != 0) {
						SPDLOG_ERROR("uv_tcp_init() failed: {}", uv_strerror(err));
						return false;
				}

				err = uv_tcp_connect(&connect_req_,
				                     uv_handle_,
				                     (const struct sockaddr*)&remote_addr_,
				                     OnConnect);
				if (err != 0) {
						SPDLOG_ERROR("uv_tcp_connect() failed: {}", uv_strerror(err));
						return false;
				}

				return true;
		}

		bool WebsocketClient::InitInvoke() {
				SPDLOG_TRACE();

				return network_thread_->Post<bool>([&]() -> bool { return Init(); }).get();
		}

		void WebsocketClient::Close() {
				SPDLOG_TRACE();

				if (closed_.load()) {
						return;
				}

				closed_.store(true);

				uv_handle_->data = nullptr;

				int err = uv_read_stop(reinterpret_cast<uv_stream_t*>(uv_handle_));
				if (err != 0) {
						SPDLOG_ERROR("uv_read_stop() failed: {}", uv_strerror(err));
						return;
				}

				if (!has_error_) {
						auto* req = new uv_shutdown_t;
						req->data = static_cast<void*>(this);
						err       = uv_shutdown(
                req, reinterpret_cast<uv_stream_t*>(uv_handle_), OnShutdown);
						if (err != 0) {
								SPDLOG_ERROR("uv_shutdown() failed: {}", uv_strerror(err));
								return;
						}
				} else {
						uv_close(reinterpret_cast<uv_handle_t*>(uv_handle_), OnCloseTcp);
				}
		}

		void WebsocketClient::CloseInvoke() {
				SPDLOG_TRACE();

				network_thread_->Post<void>([&]() { Close(); }).get();
		}

		void WebsocketClient::OnUvAlloc(size_t /*suggestedSize*/, uv_buf_t* buf) {
				SPDLOG_TRACE();

				buf->base = reinterpret_cast<char*>(read_buffer_ + buffer_data_len_);
				if (kReadBufferSize > buffer_data_len_) {
						buf->len = kReadBufferSize - buffer_data_len_;
				} else {
						buf->len = 0;
				}
		}

		void WebsocketClient::OnUvConnect() {
				SPDLOG_TRACE();

				connected_.store(true);

				const int err = uv_read_start(
				    reinterpret_cast<uv_stream_t*>(uv_handle_), OnAlloc, OnRead);
				if (err != 0) {
						SPDLOG_ERROR("uv_read_start() failed: {}", uv_strerror(err));
						return;
				}

				for (auto& listener : listeners_) {
						listener->OnConnect(Protocol::WEBSOCKET);
				}
		}

		void WebsocketClient::OnUvError() {
		}

		void WebsocketClient::Send(RTCUtils::CopyOnWriteBuffer data1,
		                           RTCUtils::CopyOnWriteBuffer data2) {
				SPDLOG_TRACE();

				if (closed_.load() || data1.size() == 0) {
						return;
				}

				unsigned int buffer_size = 1;
				uv_buf_t buffer[2];
				buffer[0] = uv_buf_init(
				    reinterpret_cast<char*>(const_cast<unsigned char*>(data1.data())),
				    data1.size());

				if (data2.size() != 0) {
						buffer[1] = uv_buf_init(
						    reinterpret_cast<char*>(const_cast<unsigned char*>(data2.data())),
						    data2.size());
						buffer_size = 2;
				}

				size_t total_len = data1.size() + data2.size();

				int written = uv_try_write(
				    reinterpret_cast<uv_stream_t*>(uv_handle_), buffer, buffer_size);
				if (written == static_cast<int>(total_len)) {
						return;
				} else if (written == UV_EAGAIN || written == UV_ENOSYS) {
						written = 0;
				} else if (written < 0) {
						SPDLOG_INFO("uv_try_write() failed, trying uv_write(): {}",
						            uv_strerror(written));
						written = 0;
				}

				const size_t pending_len = total_len - written;
				auto* write_data         = new UvWriteData(pending_len);
				write_data->req.data     = static_cast<void*>(write_data);

				if (static_cast<size_t>(written) < data1.size()) {
						::memcpy(write_data->store,
						         data1.data() + static_cast<size_t>(written),
						         data1.size() - static_cast<size_t>(written));
						::memcpy(write_data->store
						             + (data1.size() - static_cast<size_t>(written)),
						         data2.data(),
						         data2.size());
				} else {
						::memcpy(
						    write_data->store,
						    data2.data() + (static_cast<size_t>(written) - data1.size()),
						    data2.size() - (static_cast<size_t>(written) - data1.size()));
				}

				const uv_buf_t buf = uv_buf_init(
				    reinterpret_cast<char*>(write_data->store), pending_len);
				const int err = uv_write(&write_data->req,
				                         reinterpret_cast<uv_stream_t*>(uv_handle_),
				                         &buf,
				                         1,
				                         OnWrite);
				if (err != 0) {
						SPDLOG_ERROR("uv_write() failed: {}", uv_strerror(err));
						delete write_data;
				}
		}

		void WebsocketClient::SendInvoke(RTCUtils::CopyOnWriteBuffer data1,
		                                 RTCUtils::CopyOnWriteBuffer data2) {
				SPDLOG_TRACE();

				network_thread_->Post<void>(
				    [&, data1 = std::move(data1), data2 = std::move(data2)] {
						    Send(std::move(data1), std::move(data2));
				    }).get();
		}

		void WebsocketClient::OnUvRead(websocket_handle* handle,
		                               uv_stream_t* uv_handle,
		                               void* buffer,
		                               size_t n_read,
		                               uchar type) {
				SPDLOG_TRACE();

				if (n_read == 0) {
						return;
				}

				if (n_read > 0) {
						buffer_data_len_ += n_read;

						std::unique_lock<std::mutex> lock(dispatchers_mutex_);
						for (auto& dispatcher : dispatchers_) {
								dispatcher->Dispatch(
								    read_buffer_, buffer_data_len_, this, Protocol::TCP);
						}
						buffer_data_len_ = 0;
				} else if (n_read == UV_EOF || n_read == UV_ECONNRESET) {
						Close();

						for (auto& listener : listeners_) {
								listener->OnClose(Protocol::TCP);
						}
				} else {
						has_error_ = true;
						for (auto& listener : listeners_) {
								listener->OnClose(Protocol::TCP);
						}
				}
		}

		void WebsocketClient::OnUvWrite(int status) {
				SPDLOG_TRACE();

		    if (status != UV_EPIPE || status != UV_ENOTCONN) {
		        has_error_ = true;
		        for (auto& listener : listeners_) {
		            listener->OnClose(Protocol::TCP);
		        }
		    }
		}

		void WebsocketClient::AddListener(Listener* listener) {
		    SPDLOG_TRACE();

		    std::unique_lock<std::mutex> lock(listeners_mutex_);
		    listeners_.insert(listener);
		}

		void WebsocketClient::RemoveListener(Listener* listener) {
		    SPDLOG_TRACE();

		    std::unique_lock<std::mutex> lock(listeners_mutex_);
		    listeners_.erase(listener);
		}
}
