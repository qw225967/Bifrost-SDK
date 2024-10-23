/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-15 11:32
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : utils.h
 * @description : TODO
 *******************************************************/

#ifndef UTILS_H
#define UTILS_H
#include <uv.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace RTCUtils {

		class NetTools {
		public:
				static int GetFamily(const std::string& ip);

				static bool GetAddressInfo(const struct sockaddr* addr, std::string& ip,
				                           uint16_t& port);

				static char* ParseHttpHeads(const uv_buf_t* buf, const char* key_name);

				static void TwCloseClient(uv_stream_t* client,
				                          char (*on_close)(uv_stream_t*));

				static void AfterUvCloseClient(uv_handle_t* client);
		};

		class Byte {
		public:
				static std::string BytesToHex(const uint8_t* buf, std::size_t len,
				                              std::size_t num_per_line = 8) {
						if (buf == nullptr || len == 0 || num_per_line == 0) {
								return std::string();
						}
						std::ostringstream oss;
						for (std::size_t i = 0; i < len; i++) {
								oss << std::right << std::setw(3) << std::hex
								    << static_cast<int>(buf[i]);
								if ((i + 1) % num_per_line == 0) {
										oss << '\n';
								}
						}
						if (len % num_per_line != 0) {
								oss << '\n';
						}
						return oss.str();
				}

				static uint8_t Get1Byte(const uint8_t* data, size_t i) {
						return data[i];
				}

				static uint16_t Get2Bytes(const uint8_t* data, size_t i) {
						return uint16_t{ data[i + 1] } | uint16_t{ data[i] } << 8;
				}

				static uint32_t Get3Bytes(const uint8_t* data, size_t i) {
						return uint32_t{ data[i + 2] } | uint32_t{ data[i + 1] } << 8
						       | uint32_t{ data[i] } << 16;
				}

				static uint32_t Get4Bytes(const uint8_t* data, size_t i) {
						return uint32_t{ data[i + 3] } | uint32_t{ data[i + 2] } << 8
						       | uint32_t{ data[i + 1] } << 16 | uint32_t{ data[i] } << 24;
				}

				static uint64_t Get8Bytes(const uint8_t* data, size_t i) {
						return uint64_t{ Byte::Get4Bytes(data, i) } << 32
						       | Byte::Get4Bytes(data, i + 4);
				}

				static void Set1Byte(uint8_t* data, size_t i, uint8_t value) {
						data[i] = value;
				}

				static void Set2Bytes(uint8_t* data, size_t i, uint16_t value) {
						data[i + 1] = static_cast<uint8_t>(value);
						data[i]     = static_cast<uint8_t>(value >> 8);
				}

				static void Set3Bytes(uint8_t* data, size_t i, uint32_t value) {
						data[i + 2] = static_cast<uint8_t>(value);
						data[i + 1] = static_cast<uint8_t>(value >> 8);
						data[i]     = static_cast<uint8_t>(value >> 16);
				}

				static void Set4Bytes(uint8_t* data, size_t i, uint32_t value) {
						data[i + 3] = static_cast<uint8_t>(value);
						data[i + 2] = static_cast<uint8_t>(value >> 8);
						data[i + 1] = static_cast<uint8_t>(value >> 16);
						data[i]     = static_cast<uint8_t>(value >> 24);
				}

				static void Set8Bytes(uint8_t* data, size_t i, uint64_t value) {
						data[i + 7] = static_cast<uint8_t>(value);
						data[i + 6] = static_cast<uint8_t>(value >> 8);
						data[i + 5] = static_cast<uint8_t>(value >> 16);
						data[i + 4] = static_cast<uint8_t>(value >> 24);
						data[i + 3] = static_cast<uint8_t>(value >> 32);
						data[i + 2] = static_cast<uint8_t>(value >> 40);
						data[i + 1] = static_cast<uint8_t>(value >> 48);
						data[i]     = static_cast<uint8_t>(value >> 56);
				}

				static uint16_t PadTo4Bytes(uint16_t size) {
						// If size is not multiple of 32 bits then pad it.
						if (size & 0x03) {
								return (size & 0xFFFC) + 4;
						} else {
								return size;
						}
				}

				static uint32_t PadTo4Bytes(uint32_t size) {
						// If size is not multiple of 32 bits then pad it.
						if (size & 0x03) {
								return (size & 0xFFFFFFFC) + 4;
						} else {
								return size;
						}
				}
		};

		class Bits {
		public:
				static size_t CountSetBits(const uint16_t mask) {
						return static_cast<size_t>(__builtin_popcount(mask));
				}
		};

		class Time {
		public:
				static uint64_t GetMilliseconds() {
						std::chrono::time_point<std::chrono::system_clock> nowMs
						    = std::chrono::system_clock::now();

						return nowMs.time_since_epoch().count();
				}
		};

} // namespace RTCUtils
#endif // UTILS_H
