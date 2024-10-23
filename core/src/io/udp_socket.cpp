/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 13:48
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : udp_socket.cpp
 * @description : TODO
 *******************************************************/

#include "io/udp_socket.h"

#include <tuple>

#include "spdlog/spdlog.h"
#include "utils/utils.h"

static constexpr size_t kReadBufferSize{ 65536 };

namespace CoreIO {
		static void OnAlloc(uv_handle_t* uv_handle,
		                    size_t suggested_size,
		                    uv_buf_t* buf) {
				SPDLOG_TRACE();

				if (auto* socket = static_cast<UdpSocket*>(uv_handle->data)) {
						socket->OnUvRecvAlloc(suggested_size, buf);
				}
		}

		static void OnRecv(uv_udp_t* uv_udp,
		                   ssize_t n_read,
		                   const uv_buf_t* buf,
		                   const struct sockaddr* addr,
		                   unsigned int flags) {
				SPDLOG_TRACE();

				if (auto* socket = static_cast<UdpSocket*>(uv_udp->data)) {
						socket->OnUvRecv(n_read, buf, addr, flags);
				}
		}

		static void OnSend(uv_udp_send_t* uv_udp, int status) {
				SPDLOG_TRACE();

				auto* send_data = static_cast<UdpSocket::UvSendData*>(uv_udp->data);
				auto* handle    = uv_udp->handle;
				if (auto* socket = static_cast<UdpSocket*>(handle->data)) {
						socket->OnUvSend(status);
				}

				delete send_data;
		}

		static void OnCloseUdp(uv_handle_t* uv_handle) {
				SPDLOG_TRACE();

				delete reinterpret_cast<uv_udp_t*>(uv_handle);
		}

		UdpSocket::UdpSocket(std::shared_ptr<NetworkThread> network_thread,
		                     Type type,
		                     const std::string& ip,
		                     uint16_t port)
		    : network_thread_(std::move(network_thread)), type_(type), ip_(ip),
		      port_(port), read_buffer_(new uint8_t[kReadBufferSize]) {
				SPDLOG_TRACE();
		}

		UdpSocket::~UdpSocket() {
				SPDLOG_TRACE();

				if (!closed_.load()) {
						Close();
				}
		}

		bool UdpSocket::Init() {
				SPDLOG_TRACE();

				uv_handle_ = new uv_udp_t();
				uv_udp_init_ex(network_thread_->GetLoop(), uv_handle_, UV_UDP_RECVMMSG);

				uv_handle_->data = static_cast<void*>(this);

				if (type_ == Type::SERVER) {
						if (ip_.empty() || port_ == 0) {
								return false;
						}

						struct sockaddr_storage bind_addr;
						int flags = 0;

						switch (const int family = RTCUtils::NetTools::GetFamily(ip_)) {
						case AF_INET:
						{
								const int err = uv_ip4_addr(
								    ip_.c_str(),
								    port_,
								    reinterpret_cast<struct sockaddr_in*>(&bind_addr));
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
								    reinterpret_cast<struct sockaddr_in6*>(&bind_addr));
								if (err != 0) {
										SPDLOG_ERROR("uv_ip6_addr() failed: {}", uv_strerror(err));
										return false;
								}
								flags |= UV_UDP_IPV6ONLY;
								break;
						}
						default:
						{
								SPDLOG_ERROR("unknown IP family");
								return false;
						}
						}

						const int err = uv_udp_bind(
						    uv_handle_,
						    reinterpret_cast<const struct sockaddr*>(&bind_addr),
						    flags);
						if (err != 0) {
								SPDLOG_ERROR("uv_udp_bind() failed, ip: {}, port: {}, err: {}",
								             ip_,
								             port_,
								             uv_strerror(err));
								return false;
						}
				}

				if (const int err = uv_udp_recv_start(uv_handle_, OnAlloc, OnRecv);
				    err != 0)
				{
						uv_close(reinterpret_cast<uv_handle_t*>(uv_handle_), OnCloseUdp);
						return false;
				}

				return true;
		}

		bool UdpSocket::InitInvoke() {
				SPDLOG_TRACE();

				return network_thread_->Post<bool>([&]() -> bool { return Init(); }).get();
		}

		void UdpSocket::Close() {
				SPDLOG_TRACE();

				if (closed_.load()) {
						return;
				}

				closed_.store(true);
				uv_handle_->data = nullptr;

				int err = uv_udp_recv_stop(uv_handle_);
				if (err != 0) {
						SPDLOG_ERROR("uv_udp_recv_stop() failed: {}", uv_strerror(err));
						return;
				}

				uv_close(reinterpret_cast<uv_handle_t*>(uv_handle_), OnCloseUdp);

				delete[] read_buffer_;
				dispatchers_.clear();
		}

		void UdpSocket::CloseInvoke() {
				SPDLOG_TRACE();

				network_thread_->Post<void>([&]() { Close(); }).get();
		}

		void UdpSocket::Send(RTCUtils::CopyOnWriteBuffer buf,
		                     const struct sockaddr* addr) {
				SPDLOG_TRACE();

				if (closed_.load() || buf.size() == 0) {
						return;
				}
				uv_buf_t buffer = uv_buf_init(
				    reinterpret_cast<char*>(const_cast<unsigned char*>(buf.data())),
				    buf.size());

				int sent = uv_udp_try_send(uv_handle_, &buffer, 1, addr);
				if (sent == static_cast<int>(buf.size())) {
						return;
				} else if (sent >= 0) {
						SPDLOG_WARN("datagram truncated (just {} of {} bytes were sent",
						            sent,
						            buf.size());
						return;
				} else if (sent != UV_EAGAIN) {
						SPDLOG_WARN("uv_udp_try_send() failed, trying uv_udp_send(): {}",
						            uv_strerror(sent));
				}

				auto* send_data      = new UvSendData(buf.size());
				send_data->req_.data = static_cast<void*>(send_data);
				std::memcpy(send_data->store_, buf.data(), buf.size());

				buffer  = uv_buf_init(reinterpret_cast<char*>(send_data->store_),
				                      buf.size());
				int err = uv_udp_send(&send_data->req_,
				                      uv_handle_,
				                      &buffer,
				                      1,
				                      addr,
				                      static_cast<uv_udp_send_cb>(OnSend));
				if (err != 0) {
						SPDLOG_WARN("uv_udp_send() failed: {}", uv_strerror(err));
						delete send_data;
				}
		}

		void UdpSocket::SendInvoke(RTCUtils::CopyOnWriteBuffer buf,
		                           const struct sockaddr* addr) {
				SPDLOG_TRACE();

				network_thread_->Post<void>(
				    [&, data = std::move(buf)]() { Send(data, addr); }).get();
		}

		void UdpSocket::OnUvRecvAlloc(size_t, uv_buf_t* buf) {
				SPDLOG_TRACE();

				buf->base = reinterpret_cast<char*>(read_buffer_);
				buf->len  = kReadBufferSize;
		}

		void UdpSocket::OnUvRecv(ssize_t n_read,
		                         const uv_buf_t* buf,
		                         const struct sockaddr* addr,
		                         unsigned int flags) {
				SPDLOG_TRACE();

				if (n_read == 0) {
						return;
				}

				if ((flags & UV_UDP_PARTIAL) != 0) {
						SPDLOG_ERROR(
						    "received datagram was truncated due to insufficient buffer, "
						    "ignoring "
						    "it");
						return;
				}

				if (n_read > 0) {
						// SPDLOG_INFO("----------------- recv data: {}", buf->base);

						std::unique_lock<std::mutex> lock(dispatchers_mutex_);
						for (auto& dispatcher : dispatchers_) {
								dispatcher->Dispatch(reinterpret_cast<uint8_t*>(buf->base),
								                     n_read,
								                     this,
								                     Protocol::UDP,
								                     addr);
						}
				} else {
						SPDLOG_ERROR("read error: {}", uv_strerror(n_read));
				}
		}

		void UdpSocket::OnUvSend(int status) {
				SPDLOG_TRACE();
		}
} // namespace CoreIO
