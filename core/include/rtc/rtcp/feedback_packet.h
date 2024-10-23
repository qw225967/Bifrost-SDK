/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 15:28
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : feedback_packet.h
 * @description : TODO
 *******************************************************/

#ifndef FEEDBACK_PACKET_H
#define FEEDBACK_PACKET_H

#include "rtcp_packet.h"

namespace RTC {
		namespace RTCP {

				template<typename T>
				class FeedbackPacket : public RtcpPacket {
				public:
						// Feedback 头，带有发送的ssrc和当前流的ssrc
						struct Header {
								uint32_t sender_ssrc;
								uint32_t media_ssrc;
						};

				public:
						static Type rtcp_type_;
						static std::shared_ptr<FeedbackPacket<T>> Parse(const uint8_t* data,
						                                                size_t len);
						static const std::string& MessageType2String(
						    typename T::MessageType type);

				private:
						static std::map<typename T::MessageType, std::string> type_to_string_;

				public:
						typename T::MessageType GetMessageType() const {
								return this->message_type_;
						}
						uint32_t GetSenderSsrc() const {
								return uint32_t{ ntohl(this->header_->sender_ssrc) };
						}
						void SetSenderSsrc(uint32_t ssrc) {
								this->header_->sender_ssrc = uint32_t{ htonl(ssrc) };
						}
						uint32_t GetMediaSsrc() const {
								return uint32_t{ ntohl(this->header_->media_ssrc) };
						}
						void SetMediaSsrc(uint32_t ssrc) {
								this->header_->media_ssrc = uint32_t{ htonl(ssrc) };
						}

						/* Pure virtual methods inherited from Packet. */
				public:
						void Dump() const override {
						}
						size_t Serialize(uint8_t* buffer) override;
						size_t GetCount() const override {
								return static_cast<size_t>(GetMessageType());
						}
						size_t GetSize() const override {
								return sizeof(CommonHeader) + sizeof(Header);
						}

				protected:
						explicit FeedbackPacket(CommonHeader* common_header);
						FeedbackPacket(typename T::MessageType message_type,
						               uint32_t sender_ssrc, uint32_t media_ssrc);

				public:
						~FeedbackPacket() override;

				private:
						Header* header_{ nullptr };
						uint8_t* raw_{ nullptr };
						typename T::MessageType message_type_;
				};

				class FeedbackPs {
				public:
						enum class MessageType : uint8_t {
								PLI   = 1,
								SLI   = 2,
								RPSI  = 3,
								FIR   = 4,
								TSTR  = 5,
								TSTN  = 6,
								VBCM  = 7,
								PSLEI = 8,
								ROI   = 9,
								AFB   = 15,
								EXT   = 31
						};
				};

				class FeedbackRtp {
				public:
						enum class MessageType : uint8_t {
								NACK   = 1,
								TMMBR  = 3,
								TMMBN  = 4,
								SR_REQ = 5,
								RAMS   = 6,
								TLLEI  = 7,
								ECN    = 8,
								PS     = 9,
								TCC    = 15,
								QUICFB = 16,
								EXT    = 31
						};
				};

				using FeedbackPsPacket  = FeedbackPacket<FeedbackPs>;
				using FeedbackRtpPacket = FeedbackPacket<FeedbackRtp>;
		} // namespace RTCP
} // namespace RTC

#endif  // FEEDBACK_PACKET_H
