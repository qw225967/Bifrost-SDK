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
#include "ws_util.h"

namespace RTCUtils {

		int NetTools::GetFamily(const std::string& ip) {
				SPDLOG_TRACE();

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
				SPDLOG_TRACE();

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
								SPDLOG_ERROR("uv_inet_ntop() failed: {}", uv_strerror(err));
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
								SPDLOG_ERROR("uv_inet_ntop() failed: {}", uv_strerror(err));
								return false;
						}

						port = static_cast<uint16_t>(ntohs(
						    reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port));
						break;
				}
				default:
				{
						SPDLOG_ERROR("unknown network family: {}",
						             static_cast<int>(addr->sa_family));
						return false;
				}
				}

				ip.assign(ipBuffer);

				return true;
		}

		char* NetTools::ParseHttpHeads(const uv_buf_t* buf, const char* key_name) {
				char *key = nullptr, *end = nullptr;
				char* data = strstr(buf->base, "\r\n\r\n");
				if (data) {
						*data = 0;
						// 是否有 Sec-WebSocket-Key
						key = strstr(buf->base, key_name);
						if (key) {
								key += strlen(key_name) + 1;
								while (isspace(*key)) {
										key++;
								}
								end = strchr(key, '\r');
								if (end) {
										*end = 0;
								}
								return key;
						}
				}
				return nullptr;
		}

		void NetTools::TwCloseClient(uv_stream_t* uv_stream,
		                             char (*on_close)(uv_stream_t*)) {
				// 关闭连接回调
				if (on_close) {
						on_close(uv_stream);
				}
				if (!uv_is_closing((uv_handle_t*)uv_stream)) {
						uv_close((uv_handle_t*)uv_stream, AfterUvCloseClient);
				} else {
				}
		}

		void NetTools::AfterUvCloseClient(uv_handle_t* client) {
				if (client->data) {
						websocket_handle* hd       = (websocket_handle*)client->data;
						endpoint_type endpointType = hd->endpoint_type;
						membuf_uninit(&hd->proto_buf);
						membuf_uninit(&hd->merge_buf);
						free(hd);
						client->data = nullptr;
						if (uv_is_closing((uv_handle_t*)client)) {
						}
						if (endpointType == ENDPOINT_SERVER) {
								free(client);
						}
				}
		}
} // namespace RTCUtils