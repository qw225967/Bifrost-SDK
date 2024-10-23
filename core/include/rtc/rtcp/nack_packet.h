/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 15:58
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : nack_packet.h
 * @description : TODO
 *******************************************************/

/* RFC 4585
 * Generic NACK message (NACK)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |              PID              |             BPL               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#ifndef NACK_PACKET_H
#define NACK_PACKET_H

#include "rtc/rtcp/feedback_item.h"
#include "rtc/rtcp/feedback_packet.h"
#include "rtc/rtcp/feedback_rtp_item.h"
#include "utils/utils.h"

namespace RTC {
		namespace RTCP {
				class FeedbackRtpNackItem : public FeedbackItem {
				public:
						struct Header {
								uint16_t packet_id;
								uint16_t lost_packet_bitmask;
						};

				public:
						static constexpr FeedbackRtp::MessageType message_type_{
								FeedbackRtp::MessageType::NACK
						};

				public:
						explicit FeedbackRtpNackItem(Header* header) : header_(header) {
						}
						explicit FeedbackRtpNackItem(FeedbackRtpNackItem* item)
						    : header_(item->header_) {
						}
						FeedbackRtpNackItem(uint16_t packet_id, uint16_t lost_packet_bitmask);
						~FeedbackRtpNackItem() override = default;

						uint16_t GetPacketId() const {
								return uint16_t{ ntohs(this->header_->packet_id) };
						}
						uint16_t GetLostPacketBitmask() const {
								return uint16_t{ ntohs(this->header_->lost_packet_bitmask) };
						}
						size_t CountRequestedPackets() const {
								return RTCUtils::Bits::CountSetBits(
								           this->header_->lost_packet_bitmask)
								       + 1;
						}

						/* Virtual methods inherited from FeedbackItem. */
				public:
						void Dump() const override {
						}
						size_t Serialize(uint8_t* buffer) override;
						size_t GetSize() const override {
								return sizeof(Header);
						}

				private:
						Header* header_{ nullptr };
				};

				// Nack packet declaration.
				using FeedbackRtpNackPacket = FeedbackRtpItemsPacket<FeedbackRtpNackItem>;
		} // namespace RTCP
} // namespace RTC

#endif // NACK_PACKET_H
