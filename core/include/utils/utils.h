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

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <assert.h>

namespace RTCUtils {

		class NetTools {
		public:
				static int GetFamily(const std::string& ip);

				static bool GetAddressInfo(const struct sockaddr* addr, std::string& ip,
				                           uint16_t& port);

				static std::string ParseHttpHeads(const uv_buf_t* buf);

				static void TwCloseClient(uv_stream_t* client,
				                          char (*on_close)(uv_stream_t*));
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

		class MemBuffer {
		public:
				/**
				 * @brief 内存缓冲区
				 */
				typedef struct membuf_t {
						uint8_t* data;      /// 缓冲区指针
						size_t size;        /// 缓冲区的实际数据长度
						size_t buffer_size; /// 缓冲区尺寸
						uint8_t flag; /// Websocket的类型标志字节，为了与协议取值保持兼容，文本和二进制的值是一样的，
						              ///  ([0~7]: [0]是否WebSocket文本帧)
						              ///  [1]是否WebSocket二进制帧 [2]是否websocket协议
				} membuf_t;


				uint8_t *GetData() {
						return this->buf.data;
				}

				size_t GetLen() {
						return this->buf.size;
				}

				/**
				 * 缓冲区初始化
				 * @param buf  指向缓冲区的指针
				 * @param initial_buffer_size  缓冲区的尺寸
				 * @see membuf_uninit()
				 * @note 其销毁方法是membuf_uninit()
				 */
				void membuf_init(uint initial_buffer_size) {
						memset(&buf, 0, sizeof(membuf_t));
						buf.data        = initial_buffer_size > 0
						                       ? (uint8_t*)calloc(1, initial_buffer_size)
						                       : nullptr;
						buf.buffer_size = initial_buffer_size;
				}

				/**
				 * 销毁缓冲区
				 * @param buf  指向缓冲区的指针
				 * @see membuf_init()
				 */
				void membuf_uninit() {
						if (buf.data) {
								free(buf.data);
						}
						memset(&buf, 0, sizeof(membuf_t));
				}

				/**
				 * 清理缓冲区
				 * @param buf  指向缓冲区的指针
				 * @param maxSize   保留的缓冲区长度
				 */
				void membuf_clear(size_t maxSize) {
						if (buf.data && buf.size) {
								if (maxSize > 1 && buf.buffer_size > maxSize) {
										uint8_t* p = (uint8_t*)realloc(buf.data, maxSize);
										// 防止realloc分配失败，或返回的地址一样
										assert(p);
										if (p != buf.data) {
												buf.data = p;
										}
										buf.size        = 0;
										buf.buffer_size = maxSize;
								} else {
										buf.size = 0;
								}
								memset(buf.data, 0, buf.buffer_size);
						}
				}

				/**
				 * 确保缓冲区的可用区域的长度
				 * @param buf   指向缓冲区的指针
				 * @param extra_size   需要确保的用于保存数据尺寸
				 */
				void membuf_reserve(size_t extra_size) {
						if (extra_size > buf.buffer_size - buf.size) {
								// calculate new buffer size
								uint new_buffer_size
								    = buf.buffer_size == 0 ? extra_size : buf.buffer_size << 1;
								uint new_data_size = buf.size + extra_size;
								while (new_buffer_size < new_data_size) {
										new_buffer_size <<= 1;
								}

								// malloc/realloc new buffer
								uint8_t* p = (uint8_t*)realloc(
								    buf.data, new_buffer_size); // realloc new buffer
								// 防止realloc分配失败，或返回的地址一样
								assert(p);
								if (p != buf.data) {
										buf.data = p;
								}
								memset((buf.data + buf.size), 0, new_buffer_size - buf.size);
								buf.buffer_size = new_buffer_size;
						}
				}

				/**
				 * 截断(释放)多余的内存 或者增加内存,至 size+4 的大小; 后面4字节填充0
				 * @param buf  指向缓冲区的指针
				 */
				void membuf_trunc() {
						if (buf.buffer_size > (buf.size + 4)
						    || buf.buffer_size < (buf.size + 4))
						{
								uint8_t* p
								    = (uint8_t*)realloc(buf.data,
								                        buf.size + 4); // realloc new buffer
								// 防止realloc分配失败，或返回的地址一样
								assert(p);
								if (p && p != buf.data) {
										buf.data = p;
								}
								memset((buf.data + buf.size), 0, 4);
								buf.buffer_size = buf.size + 4;
						}
				}

				/**
				 * 向缓冲区追加数据
				 * @param buf  指向缓冲区的指针
				 * @param data  指向需要追加的数据的首位置的指针
				 * @param size  需要追加的数据的长度
				 */
				void membuf_append_data(const void* data, size_t size) {
						if (data == nullptr || size == 0) {
								return;
						}
						membuf_reserve(size);
						memmove((buf.data + buf.size), data, size);
						buf.size += size;
				}

				/**
				 * 向缓冲区追加格式化数据
				 * @param buf  指向缓冲区的指针
				 * @param fmt  类似printf的format串
				 * @param ...  类型printf的...参数
				 * @return 追加的字节数量
				 */
				uint membuf_append_format(const char* fmt, ...) {
						assert(fmt);
						va_list ap, ap2;
						va_start(ap, fmt);
						int size = vsnprintf(0, 0, fmt, ap) + 1;
						va_end(ap);
						membuf_reserve(size);
						va_start(ap2, fmt);
						vsnprintf((char*)(buf.data + buf.size), size, fmt, ap2);
						va_end(ap2);
						return size;
				}

				/**
				 * 向缓冲区中插入数据
				 * @param buf  指向缓冲区的指针
				 * @param offset  插入的位置，比如，设置为0则表示从头部插入
				 * @param data   指向需要插入的数据的首位置的指针
				 * @param size   需要插入的数据的长度
				 */
				void membuf_insert(uint offset, void* data, size_t size) {
						assert(offset < buf.size);
						membuf_reserve(size);
						memcpy((buf.data + offset + size), buf.data + offset,
						       buf.size - offset);
						memcpy((buf.data + offset), data, size);
						buf.size += size;
				}

				/**
				 * 从缓冲区的指定位置移除部分数据
				 * @param buf   指向缓冲区的指针
				 * @param offset
				 * 需要移除数据的位置，如果指定位置比数据长度还大则该操作没有任何影响
				 * @param size  需要移除的数据长度
				 */
				void membuf_remove(uint offset, size_t size) {
						if (offset >= buf.size) {
								return;
						}
						if (offset + size >= buf.size) {
								buf.size = offset;
						} else {
								memmove((buf.data + offset), buf.data + offset + size,
								        buf.size - offset - size);
								buf.size -= size;
						}
						if (buf.buffer_size >= buf.size) {
								buf.data[buf.size] = 0;
						}
				}

		private:
				membuf_t buf;
		};

		class Bits {
		public:
				static size_t CountSetBits(const uint16_t mask) {
						return static_cast<size_t>(__builtin_popcount(mask));
				}
		};

		class Time {
				// Seconds from Jan 1, 1900 to Jan 1, 1970.
				static constexpr uint32_t kUnixNtpOffset{ 0x83AA7E80 };
				// NTP fractional unit.
				static constexpr uint64_t kNtpFractionalUnit{ 1LL << 32 };

		public:
				struct Ntp {
						uint32_t seconds;
						uint32_t fractions;
				};

				static Time::Ntp TimeMs2Ntp(uint64_t ms) {
						Time::Ntp ntp{}; // NOLINT(cppcoreguidelines-pro-type-member-init)

						ntp.seconds   = ms / 1000;
						ntp.fractions = static_cast<uint32_t>(
						    (static_cast<double>(ms % 1000) / 1000) * kNtpFractionalUnit);

						return ntp;
				}

				static uint64_t Ntp2TimeMs(Time::Ntp ntp) {
						// clang-format off
				return (
					static_cast<uint64_t>(ntp.seconds) * 1000 +
					static_cast<uint64_t>(std::round((static_cast<double>(ntp.fractions) * 1000) / kNtpFractionalUnit))
				);
						// clang-format on
				}
				static uint64_t GetTimeMs() {
						return static_cast<uint64_t>(uv_hrtime() / 1000000u);
				}
				static uint64_t GetTimeUs() {
						return static_cast<uint64_t>(uv_hrtime() / 1000u);
				}
				static uint64_t GetTimeNs() {
						return uv_hrtime();
				}
				// Used within libwebrtc dependency which uses int64_t values for time
				// representation.
				static int64_t GetTimeMsInt64() {
						return static_cast<int64_t>(Time::GetTimeMs());
				}
				// Used within libwebrtc dependency which uses int64_t values for time
				// representation.
				static int64_t GetTimeUsInt64() {
						return static_cast<int64_t>(Time::GetTimeUs());
				}
		};

} // namespace RTCUtils
#endif // UTILS_H
