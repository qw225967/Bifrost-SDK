/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 上午11:08
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_packet.h
 * @description : TODO
 *******************************************************/

#ifndef RTP_PACKET_H
#define RTP_PACKET_H

#include "rtc/rtc_common.h"
#define MS_LITTLE_ENDIAN 1
namespace RTC {
		// 网络传输最大 MTU
		constexpr size_t kMtuSize{ 1500u };

		class RtpPacket {
		public:
				// rtp 头部信息
				struct Header {
#if defined(MS_LITTLE_ENDIAN)
						uint8_t csrc_count : 4;
						uint8_t extension : 1;
						uint8_t padding : 1;
						uint8_t version : 2;
						uint8_t payload_type : 7;
						uint8_t marker : 1;
#elif defined(MS_BIG_ENDIAN)
						uint8_t version : 2;
						uint8_t padding : 1;
						uint8_t extension : 1;
						uint8_t csrc_count : 4;
						uint8_t marker : 1;
						uint8_t payload_type : 7;
#endif
						uint16_t sequence_number;
						uint32_t timestamp;
						uint32_t ssrc;
				};

		public:
				// 通用rtp头大小
				static const size_t kHeaderSize{ 12 };

				// 判断是否是rtp包
				static bool IsRtp(const uint8_t* data, size_t len) {
						// NOTE: RtcpPacket::IsRtcp() must always be called before this method.

						auto* header
						    = const_cast<Header*>(reinterpret_cast<const Header*>(data));

						// clang-format off
		return (
			(len >= kHeaderSize) &&
			// DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
			(data[0] > 127 && data[0] < 192) &&
			// RTP Version must be 2.
			(header->version == 2)
		);
						// clang-format on
				}

				static std::shared_ptr<RtpPacket> Parse(const uint8_t* data, size_t len);

		public:
				RtpPacket(const uint8_t* data, size_t size);
				RtpPacket(Header header);
				~RtpPacket();

				// 获取函数
				[[nodiscard]] const uint8_t* GetData() const {
						return (const uint8_t*)this->header_;
				}

				[[nodiscard]] size_t GetSize() const {
						return this->size_;
				}

				[[nodiscard]] uint8_t GetPayloadType() const {
						return this->header_->payload_type;
				}

				[[nodiscard]] uint16_t GetSequenceNumber() const {
						return uint16_t{ ntohs(this->header_->sequence_number) };
				}

				[[nodiscard]] uint32_t GetTimestamp() const {
						return uint32_t{ ntohl(this->header_->timestamp) };
				}

				[[nodiscard]] uint32_t GetSsrc() const {
						return uint32_t{ ntohl(this->header_->ssrc) };
				}

				[[nodiscard]] uint8_t* GetPayload() const {
					return this->payload_;
				}

				[[nodiscard]] uint32_t GetPayloadLength() const {
					return this->payload_length_;
				}

				// 设置函数
				void SetVersion(uint8_t version) {
						this->header_->version = version;
				}

				void SetPayloadType(uint8_t payload_type) {
						this->header_->payload_type = payload_type;
				}

				void SetTimestamp(uint32_t timestamp) {
						this->header_->timestamp = uint32_t{ htonl(timestamp) };
				}

				bool HasMarker() const {
						return this->header_->marker;
				}

				void SetMarker(bool marker) {
						this->header_->marker = marker;
				}

				void SetPayloadPaddingFlag(bool flag) {
						this->header_->padding = flag;
				}

				void SetSequenceNumber(uint16_t seq) {
						this->header_->sequence_number = uint16_t{ htons(seq) };
				}

				void SetSsrc(uint32_t ssrc) {
						this->header_->ssrc = uint32_t{ htonl(ssrc) };
				}

				void CopyPayload(uint8_t* payload, size_t len) {
						memcpy(this->data_ + kHeaderSize, payload, len);
						this->size_ += kHeaderSize + len;
				}

		private:
				// 头部指针
				Header* header_{ nullptr };
				// payload 指针
				uint8_t* payload_{ nullptr };

				// 数据存储地址
				uint8_t* data_{ nullptr };

				uint32_t size_{ 0 };
				uint32_t ssrc_{ 0 };
				uint32_t timestamp_{ 0 };
				uint32_t payload_length_{ 0 };
		};

		typedef std::shared_ptr<RtpPacket> RtpPacketPtr;
}

#endif //RTP_PACKET_H
