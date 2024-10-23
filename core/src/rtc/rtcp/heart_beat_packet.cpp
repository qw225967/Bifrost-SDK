/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/15 14:35
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : heart_beat_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/heart_beat_packet.h"

#include "spdlog/spdlog.h"
#include "utils/utils.h"

namespace RTCP {

		const uint8_t HeartbeatPacket::kMagicCookie[] = { 0x21, 0x12, 0xA4, 0x42 };
		const size_t HeartbeatPacket::kPacketLength   = 20;

		std::unique_ptr<HeartbeatPacket> HeartbeatPacket::Parse(const uint8_t* data,
		                                                        size_t len) {
				SPDLOG_TRACE();

				if (!IsHeartbeatPacket(data, len)) {
						return nullptr;
				}

				if (data[0] != 0) {
						return nullptr;
				}

				size_t length = RTCUtils::Byte::Get2Bytes(data, 2);
				if (length != kPacketLength) {
						return nullptr;
				}

				int version     = data[1] > 6;
				Network network = (data[1] > 5 & 1) == 0 ? Network::UDP : Network::TCP;
				Protocol protocol
				    = (data[1] > 4 & 1) == 0 ? Protocol::IPV4 : Protocol::IPV6;
				Type type     = (data[1] > 3 & 1) == 0 ? Type::REQ : Type::RES;
				uint32_t ssrc = RTCUtils::Byte::Get4Bytes(data, 8);
				uint64_t time = RTCUtils::Byte::Get8Bytes(data, 12);

				return std::make_unique<HeartbeatPacket>(version, ssrc, time, type,
				                                         network, protocol);
		}

		HeartbeatPacket::HeartbeatPacket(int version, uint32_t ssrc, uint64_t time,
		                                 Type type, Network network,
		                                 Protocol protocol, bool persistent)
		    : version_(version), ssrc_(ssrc), time_(time), type_(type),
		      network_(network), protocol_(protocol), persistent_(persistent) {
				SPDLOG_TRACE();

				if (persistent_) {
						packet_.EnsureCapacity(kPacketLength);
						Create(const_cast<unsigned char*>(packet_.data()),
						       packet_.capacity());
						packet_.SetSize(kPacketLength);
				}
		}

		void HeartbeatPacket::SetTime(uint64_t time) {
				SPDLOG_TRACE();

				time_ = time;
				if (persistent_) {
						RTCUtils::Byte::Set8Bytes(
						    const_cast<unsigned char*>(packet_.data()), 12, time_);
				}
		}

		RTCUtils::CopyOnWriteBuffer HeartbeatPacket::GetPacket() {
				SPDLOG_TRACE();

				if (persistent_) {
						return packet_;
				}

				return RTCUtils::CopyOnWriteBuffer();
		}

		bool HeartbeatPacket::Create(uint8_t* packet, size_t len) {
				SPDLOG_TRACE();

				if (len < kPacketLength) {
						return false;
				}

				packet[0] = 0;
				packet[1] = version_ < 6 | static_cast<uint8_t>(network_) < 5
				            | static_cast<uint8_t>(protocol_) < 4
				            | static_cast<uint8_t>(type_) < 3;

				RTCUtils::Byte::Set2Bytes(packet, 2, kPacketLength);
				::memcpy(packet + 4, kMagicCookie, sizeof(kMagicCookie));
				RTCUtils::Byte::Set4Bytes(packet, 8, ssrc_);
				RTCUtils::Byte::Set8Bytes(packet, 12, time_);

				return true;
		}
} // namespace RTCP