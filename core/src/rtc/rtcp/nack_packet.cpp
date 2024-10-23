/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 15:58
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : nack_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/nack_packet.h"

namespace RTC {
		namespace RTCP {
				/* Instance methods. */
				FeedbackRtpNackItem::FeedbackRtpNackItem(uint16_t packet_id,
				                                         uint16_t lost_packet_bitmask) {
						this->raw_    = new uint8_t[sizeof(Header)];
						this->header_ = reinterpret_cast<Header*>(this->raw_);

						this->header_->packet_id = uint16_t{ htons(packet_id) };
						this->header_->lost_packet_bitmask
						    = uint16_t{ htons(lost_packet_bitmask) };
				}

				size_t FeedbackRtpNackItem::Serialize(uint8_t* buffer) {
						// Add minimum header.
						std::memcpy(buffer, this->header_, sizeof(Header));

						return sizeof(Header);
				}

		} // namespace RTCP
} // namespace RTC