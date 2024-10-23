/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午2:34
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : nack_generator.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/nack_generator.h"
#include "spdlog/spdlog.h"
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream
#include <utility>  // std::make_pair()
#include <utils/utils.h>

namespace RTC {
		/* Static. */

		static constexpr size_t kMaxPacketAge{ 10000u };
		static constexpr size_t kMaxNackPackets{ 1000u };
		static constexpr uint32_t kDefaultRtt{ 100u };
		static constexpr uint8_t kMaxNackRetries{ 10u };
		static constexpr uint64_t kTimerInterval{ 40u };

		/* Instance methods. */

		NackGenerator::NackGenerator(Listener* listener,
		                             const std::shared_ptr<CoreIO::NetworkThread>& thread,
		                             unsigned int send_nack_delay_ms)
		    : listener_(listener), send_nack_delay_ms_(send_nack_delay_ms),
		      timer_(new RTCUtils::TimerHandle(this, thread)), rtt_(kDefaultRtt) {
				 this->timer_->InitInvoke();
				// SPDLOG_TRACE();
		}

		NackGenerator::~NackGenerator() {
				// SPDLOG_TRACE();

				// Close the timer.
				this->timer_->CloseInvoke();
				delete this->timer_;
		}

		// Returns true if this is a found nacked packet. False otherwise.
		bool NackGenerator::ReceivePacket(const RtpPacketPtr& rtp_packet,
		                                  const bool is_recovered) {
				// SPDLOG_TRACE();

				const uint16_t seq      = rtp_packet->GetSequenceNumber();
				const bool is_key_frame = true;

				if (!this->started_) {
						this->started_  = true;
						this->last_seq_ = seq;

						if (is_key_frame) {
								this->key_frame_list_.insert(seq);
						}

						return false;
				}

				// Obviously never nacked, so ignore.
				if (seq == this->last_seq_) {
						return false;
				}

				// May be an out of order packet, or already handled retransmitted
				// packet, or a retransmitted packet.
				if (SeqManager<uint16_t>::IsSeqLowerThan(seq, this->last_seq_)) {
						auto it = this->nack_list_.find(seq);

						// It was a nacked packet.
						if (it != this->nack_list_.end()) {
								// SPDLOG_DEBUG("NACKed packet received [ssrc:%" PRIu32
								//             ", seq:%" PRIu16 ", recovered:%s]",
								//             packet->GetSsrc(),
								//             packet->GetSequenceNumber(),
								//             is_recovered ? "true" : "false");

								auto retries = it->second.retries_;

								this->nack_list_.erase(it);

								return retries != 0;
						}

						// Out of order packet or already handled NACKed packet.
						if (!is_recovered) {
								// SPDLOG_DEBUG(
								//    "ignoring older packet not present in the NACK list "
								//    "[ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
								//    packet->GetSsrc(),
								//    packet->GetSequenceNumber());
						}

						return false;
				}

				// If we are here it means that we may have lost some packets so seq is
				// newer than the latest seq seen.

				if (is_key_frame) {
						this->key_frame_list_.insert(seq);
				}

				// Remove old keyframes.
				{
						auto it = this->key_frame_list_.lower_bound(seq - kMaxPacketAge);

						if (it != this->key_frame_list_.begin()) {
								this->key_frame_list_.erase(this->key_frame_list_.begin(), it);
						}
				}

				if (is_recovered) {
						this->recovered_list_.insert(seq);

						// Remove old ones so we don't accumulate recovered packets.
						auto it = this->recovered_list_.lower_bound(seq - kMaxPacketAge);

						if (it != this->recovered_list_.begin()) {
								this->recovered_list_.erase(this->recovered_list_.begin(), it);
						}

						// Do not let a packet pass if it's newer than last seen seq and
						// came via RTX.
						return false;
				}

				AddPacketsToNackList(this->last_seq_ + 1, seq);

				this->last_seq_ = seq;

				// Check if there are any nacks that are waiting for this seq number.
				const std::vector<uint16_t> nackBatch = GetNackBatch(NackFilter::SEQ);

				if (!nackBatch.empty()) {
						this->listener_->OnNackGeneratorNackRequired(nackBatch);
				}

				// This is important. Otherwise the running timer (filter:TIME) would be
				// interrupted and NACKs would never been sent more than once for each seq.
				// if (!this->timer_->IsActiveInvoke()) {
				// 		MayRunTimer();
				// }

				return false;
		}

		void NackGenerator::AddPacketsToNackList(uint16_t seq_start,
		                                         uint16_t seq_end) {
				// SPDLOG_TRACE();

				// Remove old packets.
				auto it = this->nack_list_.lower_bound(seq_end - kMaxPacketAge);

				this->nack_list_.erase(this->nack_list_.begin(), it);

				// If the nack list is too large, remove packets from the nack list
				// until the latest first packet of a keyframe. If the list is still too
				// large, clear it and request a keyframe.
				const uint16_t num_new_nacks = seq_end - seq_start;

				if (static_cast<uint16_t>(this->nack_list_.size()) + num_new_nacks
				    > kMaxNackPackets)
				{
						// clang-format off
			while (
				RemoveNackItemsUntilKeyFrame() &&
				static_cast<uint16_t>(this->nack_list_.size()) + num_new_nacks > kMaxNackPackets
			)
						// clang-format on
						{}

						if (static_cast<uint16_t>(this->nack_list_.size()) + num_new_nacks
						    > kMaxNackPackets)
						{
								// SPDLOG_WARN(
								//    "NACK list full, clearing it and requesting a key frame"
								//    " [seqEnd:%" PRIu16 "]",
								//    seq_end);

								this->nack_list_.clear();
								this->listener_->OnNackGeneratorKeyFrameRequired();

								return;
						}
				}

				for (uint16_t seq = seq_start; seq != seq_end; ++seq) {
						// Do not send NACK for packets that are already recovered by RTX.
						if (this->recovered_list_.find(seq) != this->recovered_list_.end()) {
								continue;
						}

						this->nack_list_.emplace(
						    std::make_pair(seq,
						                   NackInfo{
						                       RTCUtils::Time::GetTimeMs(),
						                       seq,
						                       seq,
						                   }));
				}
		}

		bool NackGenerator::RemoveNackItemsUntilKeyFrame() {
				// SPDLOG_TRACE();

				while (!this->key_frame_list_.empty()) {
						auto it
						    = this->nack_list_.lower_bound(*this->key_frame_list_.begin());

						if (it != this->nack_list_.begin()) {
								// We have found a keyframe that actually is newer than at least
								// one packet in the nack list.
								this->nack_list_.erase(this->nack_list_.begin(), it);

								return true;
						}

						// If this keyframe is so old it does not remove any packets from
						// the list, remove it from the list of keyframes and try the next
						// keyframe.
						this->key_frame_list_.erase(this->key_frame_list_.begin());
				}

				return false;
		}

		std::vector<uint16_t> NackGenerator::GetNackBatch(NackFilter filter) {
				// SPDLOG_TRACE();

				const uint64_t now_ms = RTCUtils::Time::GetTimeMs();
				std::vector<uint16_t> nackBatch;

				auto it = this->nack_list_.begin();

				while (it != this->nack_list_.end()) {
						NackInfo& nack_info = it->second;
						const uint16_t seq  = nack_info.seq_;

						if (this->send_nack_delay_ms_ > 0
						    && now_ms - nack_info.created_at_ms_ < this->send_nack_delay_ms_)
						{
								++it;
								continue;
						}

						// clang-format off
			if (
				filter == NackFilter::SEQ &&
				nack_info.sent_at_ms_ == 0 &&
				(
					nack_info.send_at_seq_ == this->last_seq_ ||
					SeqManager<uint16_t>::IsSeqHigherThan(this->last_seq_, nack_info.send_at_seq_)
				)
			)
						// clang-format on
						{
								nackBatch.emplace_back(seq);
								nack_info.retries_++;
								nack_info.sent_at_ms_ = now_ms;

								if (nack_info.retries_ >= kMaxNackRetries) {
										// SPDLOG_WARN(
										//   "sequence number removed from the NACK list due to max retries "
										//   "[filter:seq, seq:%" PRIu16 "]",
										//  seq);

										it = this->nack_list_.erase(it);
								} else {
										++it;
								}

								continue;
						}

						if (filter == NackFilter::TIME
						    && (nack_info.sent_at_ms_ == 0
						        || now_ms - nack_info.sent_at_ms_
						               >= (this->rtt_ > 0u ? this->rtt_ : kDefaultRtt)))
						{
								nackBatch.emplace_back(seq);
								nack_info.retries_++;
								nack_info.sent_at_ms_ = now_ms;

								if (nack_info.retries_ >= kMaxNackRetries) {
										// SPDLOG_WARN(
										//    "sequence number removed from the NACK list due to max retries "
										//  "[filter:time, seq:%" PRIu16 "]",
										//  seq);

										it = this->nack_list_.erase(it);
								} else {
										++it;
								}

								continue;
						}

						++it;
				}

#if MS_LOG_DEV_LEVEL == 3
				if (!nackBatch.empty()) {
						std::ostringstream seqsStream;
						std::copy(nackBatch.begin(),
						          nackBatch.end() - 1,
						          std::ostream_iterator<uint32_t>(seqsStream, ","));
						seqsStream << nackBatch.back();

						if (filter == NackFilter::SEQ) {
								MS_DEBUG_DEV("[filter:SEQ, asking seqs:%s]",
								             seqsStream.str().c_str());
						} else {
								MS_DEBUG_DEV("[filter:TIME, asking seqs:%s]",
								             seqsStream.str().c_str());
						}
				}
#endif

				return nackBatch;
		}

		void NackGenerator::Reset() {
				// SPDLOG_TRACE();

				this->nack_list_.clear();
				this->key_frame_list_.clear();
				this->recovered_list_.clear();
				this->started_  = false;
				this->last_seq_ = 0u;
		}

		inline void NackGenerator::MayRunTimer() const {
				if (this->nack_list_.empty()) {
						this->timer_->StopInvoke();
				} else {
						this->timer_->StartInvoke(kTimerInterval);
				}
		}

		inline void NackGenerator::OnTimer(RTCUtils::TimerHandle* /*timer*/) {
				// SPDLOG_TRACE();

				const std::vector<uint16_t> nack_batch = GetNackBatch(NackFilter::TIME);

				if (!nack_batch.empty()) {
						this->listener_->OnNackGeneratorNackRequired(nack_batch);
				}

				// MayRunTimer();
		}
} // namespace RTC