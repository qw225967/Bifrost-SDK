/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午5:03
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_retransmission_buffer.h
 * @description : TODO
 *******************************************************/

#ifndef RTP_RETRANSMISSION_BUFFER_H
#define RTP_RETRANSMISSION_BUFFER_H

#include "rtc_common.h"
#include "rtc/rtp_packet.h"
#include <deque>
#include <utility>

namespace RTC {
		// Special container that stores `Item`* elements addressable by their
		// `uint16_t` sequence number, while only taking as little memory as
		// necessary to store the range covering a maximum of
		// `MaxRetransmissionDelayForVideoMs` or
		//  `MaxRetransmissionDelayForAudioMs` ms.
		class RtpRetransmissionBuffer {
		public:
				struct Item {
						Item() = default;
						Item(RtpPacketPtr& rtp_packet, uint32_t ssrc,
						     uint16_t sequence_number, uint32_t timestamp,
						     uint64_t resent_at_ms, uint8_t sent_times)
						    : rtp_packet(rtp_packet), ssrc(ssrc),
						      sequence_number(sequence_number), timestamp(timestamp),
						      resent_at_ms(resent_at_ms), sent_times(sent_times) {
						}
						void Reset();

						// Original packet.
						RtpPacketPtr rtp_packet{ nullptr };
						// Correct SSRC since original packet may not have the same.
						uint32_t ssrc{ 0u };
						// Correct sequence number since original packet may not have the same.
						uint16_t sequence_number{ 0u };
						// Correct timestamp since original packet may not have the same.
						uint32_t timestamp{ 0u };
						// Last time this packet was resent.
						uint64_t resent_at_ms{ 0u };
						// Number of times this packet was resent.
						uint8_t sent_times{ 0u };
				};

		public:
				RtpRetransmissionBuffer(uint16_t maxItems,
				                        uint32_t maxRetransmissionDelayMs,
				                        uint32_t clockRate);
				~RtpRetransmissionBuffer();

				Item* Get(uint16_t seq) const;
				void Insert(RtpPacketPtr& rtp_packet);
				void Clear();

		private:
				Item* GetOldest() const;
				Item* GetNewest() const;
				void RemoveOldest();
				void RemoveOldest(uint16_t numItems);
				bool ClearTooOldByTimestamp(uint32_t newestTimestamp);
				bool IsTooOldTimestamp(uint32_t timestamp, uint32_t newestTimestamp) const;

		protected:
				// Make buffer protected for testing purposes.
				std::deque<Item*> buffer_;

		private:
				// Given as argument.
				uint16_t max_items_;
				uint32_t max_retransmission_delay_ms_;
				uint32_t clock_rate_;
		};
} // namespace RTC

#endif // RTP_RETRANSMISSION_BUFFER_H
