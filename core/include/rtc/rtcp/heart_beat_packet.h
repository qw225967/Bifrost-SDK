/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 14:34
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : heart_beat.h
 * @description : TODO
 *******************************************************/

#ifndef HEART_BEAT_H
#define HEART_BEAT_H

#include <stdint.h>

#include <memory>

#include "io/packet_dispatcher_interface.h"
#include "utils/copy_on_write_buffer.h"

namespace RTCP {
		class HeartbeatPacket {
		public:
				enum class Type { REQ, RES };

				enum class Network { UDP = 1, TCP };

				enum class Protocol { IPV4, IPV6 };

		public:
				static bool IsHeartbeatPacket(const uint8_t* data, size_t len) {
						return ((len == 20) && (data[0] < 3)
						        && (data[4] == HeartbeatPacket::kMagicCookie[0])
						        && (data[5] == HeartbeatPacket::kMagicCookie[1])
						        && (data[6] == HeartbeatPacket::kMagicCookie[2])
						        && (data[7] == HeartbeatPacket::kMagicCookie[3]));
				}

				static std::unique_ptr<HeartbeatPacket> Parse(const uint8_t* data,
				                                              size_t len);

		private:
				static const uint8_t kMagicCookie[];
				static const size_t kPacketLength;

		public:
				HeartbeatPacket(int version,
				                uint32_t ssrc,
				                uint64_t time,
				                Type type,
				                Network network,
				                Protocol protocol,
				                bool persistent = false);

				static size_t BlockLength() {
						return kPacketLength;
				}

				// 获取接口 内联
				[[nodiscard]] int GetVersion() const {
						return version_;
				}

				[[nodiscard]] uint32_t GetSsrc() const {
						return ssrc_;
				}

				[[nodiscard]] uint64_t GetTime() const {
						return time_;
				}

				[[nodiscard]] Type GetType() const {
						return type_;
				}

				[[nodiscard]] Network GetNetwork() const {
						return network_;
				}

				[[nodiscard]] Protocol GetProtocol() const {
						return protocol_;
				}

				[[nodiscard]] bool GetPersistent() const {
						return persistent_;
				}

				RTCUtils::CopyOnWriteBuffer GetPacket();

				void SetTime(uint64_t time);

				bool Create(uint8_t* packet, size_t len);

		private:
				int version_;
				uint32_t ssrc_;
				uint64_t time_;
				Type type_;
				Network network_;
				Protocol protocol_;
				bool persistent_;
				RTCUtils::CopyOnWriteBuffer packet_;
		};
} // namespace RTCP

#endif // HEART_BEAT_H
