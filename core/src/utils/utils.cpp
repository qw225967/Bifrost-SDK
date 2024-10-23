/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-15 11:34
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : utils.cpp
 * @description : TODO
 *******************************************************/

#include "utils/utils.h"

#include <string>

#include "spdlog/spdlog.h"
#include "uv.h"

namespace RTCUtils {

		int NetTools::GetFamily(const std::string& ip) {
				// SPDLOG_TRACE();

				if (ip.size() >= INET6_ADDRSTRLEN) {
						return AF_UNSPEC;
				}

				auto ipPtr                      = ip.c_str();
				char ipBuffer[INET6_ADDRSTRLEN] = { 0 };

				if (uv_inet_pton(AF_INET, ipPtr, ipBuffer) == 0) {
						return AF_INET;
				} else if (uv_inet_pton(AF_INET6, ipPtr, ipBuffer) == 0) {
						return AF_INET6;
				} else {
						return AF_UNSPEC;
				}
		}

		bool NetTools::GetAddressInfo(const struct sockaddr* addr,
		                              std::string& ip,
		                              uint16_t& port) {
				// SPDLOG_TRACE();

				char ipBuffer[INET6_ADDRSTRLEN] = { 0 };

				switch (addr->sa_family) {
				case AF_INET:
				{
						int err = uv_inet_ntop(
						    AF_INET,
						    std::addressof(
						        reinterpret_cast<const struct sockaddr_in*>(addr)->sin_addr),
						    ipBuffer,
						    sizeof(ipBuffer));
						if (err) {
								// SPDLOG_ERROR("uv_inet_ntop() failed: {}", uv_strerror(err));
								return false;
						}

						port = static_cast<uint16_t>(ntohs(
						    reinterpret_cast<const struct sockaddr_in*>(addr)->sin_port));
						break;
				}
				case AF_INET6:
				{
						int err = uv_inet_ntop(
						    AF_INET,
						    std::addressof(
						        reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_addr),
						    ipBuffer,
						    sizeof(ipBuffer));
						if (err) {
								// SPDLOG_ERROR("uv_inet_ntop() failed: {}", uv_strerror(err));
								return false;
						}

						port = static_cast<uint16_t>(ntohs(
						    reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port));
						break;
				}
				default:
				{
						// SPDLOG_ERROR("unknown network family: {}",
						//           static_cast<int>(addr->sa_family));
						return false;
				}
				}

				ip.assign(ipBuffer);

				return true;
		}

		std::string NetTools::ParseHttpHeads(const uv_buf_t* buf) {
				std::string respone(buf->base);
				auto start_index = respone.find("sec-websocket-accept:");
				auto end_index   = respone.find("\r\n\r\n");

				if (start_index == std::string::npos) {
						start_index = respone.find("Sec-WebSocket-Accept:");
				}

				if (start_index != std::string::npos && end_index != std::string::npos) {
						start_index += 21;
						end_index;
				} else {
						return {};
				}
				// 跳过websocket标记，并截取尾部
				return respone.substr(start_index, end_index - start_index);
		}

		void NetTools::TwCloseClient(uv_stream_t* uv_stream,
		                             char (*on_close)(uv_stream_t*)) {
				// 关闭连接回调
				if (on_close) {
						on_close(uv_stream);
				}
		}
} // namespace RTCUtils