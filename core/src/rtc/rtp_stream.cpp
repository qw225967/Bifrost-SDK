/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午1:59
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_stream.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtp_stream.h"
#include "io/network_thread.h"
#include "utils/utils.h"

#include "spdlog/spdlog.h"

namespace RTC {
		static constexpr uint16_t kMaxDropout{ 3000 };
		static constexpr uint16_t kMaxMisorder{ 1500 };
		static constexpr uint32_t kRtpSeqMod{ 1 << 16 };
		static constexpr size_t kScoreHistogramLength{ 24 };

		RtpStream::RtpStream(Listener* listener, Params& params,
		                     const std::shared_ptr<CoreIO::NetworkThread>& thread)
		    : listener_(listener), params_(params), thread_(thread) {
		}
		RtpStream::~RtpStream() = default;

		bool RtpStream::ReceiveStreamPacket(RtpPacketPtr& rtp_packet) {
				// SPDLOG_TRACE();

				const uint16_t seq = rtp_packet->GetSequenceNumber();

				// If this is the first packet seen, initialize stuff.
				if (!this->started_) {
						InitSequence(seq);

						this->started_       = true;
						this->max_seq_       = seq - 1;
						this->max_packet_ts_ = rtp_packet->GetTimestamp();
						this->max_packet_ms_ = RTCUtils::Time::GetTimeMs();
				}

				// If not a valid packet ignore it.
				if (!UpdateSequence(rtp_packet)) {
						// SPDLOG_WARN("invalid packet [ssrc:{}, seq:{}]",
						//          rtp_packet->GetSsrc(),
						//           rtp_packet->GetSequenceNumber());

						return false;
				}

				// Update highest seen RTP timestamp.
				if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(
				        rtp_packet->GetTimestamp(), this->max_packet_ts_))
				{
						this->max_packet_ts_ = rtp_packet->GetTimestamp();
						this->max_packet_ms_ = RTCUtils::Time::GetTimeMs();
				}

				return true;
		}

		bool RtpStream::UpdateSequence(RtpPacketPtr& rtp_packet) {
				// SPDLOG_TRACE();

				const uint16_t seq    = rtp_packet->GetSequenceNumber();
				const uint16_t udelta = seq - this->max_seq_;

				// If the new packet sequence number is greater than the max seen but not
				// "so much bigger", accept it.
				// NOTE: udelta also handles the case of a new cycle, this is:
				//    maxSeq:65536, seq:0 => udelta:1
				if (udelta < kMaxDropout) {
						// In order, with permissible gap.
						if (seq < this->max_seq_) {
								// Sequence number wrapped: count another 64K cycle.
								this->cycles_ += kRtpSeqMod;
						}

						this->max_seq_ = seq;
				}
				// Too old packet received (older than the allowed misorder).
				// Or too new packet (more than acceptable dropout).
				else if (udelta <= kRtpSeqMod - kMaxMisorder)
				{
						// The sequence number made a very large jump. If two sequential
						// packets arrive, accept the latter.
						if (seq == this->bad_seq_) {
								// Two sequential packets. Assume that the other side restarted
								// without telling us so just re-sync (i.e., pretend this was
								// the first packet).
								// SPDLOG_WARN(
								//   "too bad sequence number, re-syncing RTP"
								//  " [ssrc:{}, seq:{}]",
								//  rtp_packet->GetSsrc(),
								//  rtp_packet->GetSequenceNumber());

								InitSequence(seq);

								this->max_packet_ts_ = rtp_packet->GetTimestamp();
								this->max_packet_ms_ = RTCUtils::Time::GetTimeMs();

								// 用于子类重置传输序号
								// UserOnSequenceNumberReset();
						} else {
								// SPDLOG_WARN(
								//   "bad sequence number, ignoring packet "
								//  "[ssrc:{}, seq:{}]",
								//  rtp_packet->GetSsrc(),
								//  rtp_packet->GetSequenceNumber());

								this->bad_seq_ = (seq + 1) & (kRtpSeqMod - 1);

								// Packet discarded due to late or early arriving.
								this->packets_discarded_++;

								return false;
						}
				}
				// Acceptable misorder.
				else
				{
						// Do nothing.
				}

				return true;
		}

		inline void RtpStream::InitSequence(uint16_t seq) {
				// SPDLOG_TRACE();

				// Initialize/reset RTP counters.
				this->base_seq_ = seq;
				this->max_seq_  = seq;
				this->bad_seq_  = kRtpSeqMod + 1; // So seq == badSeq is false.
		}

		void RtpStream::PacketRetransmitted(RtpPacketPtr& rtp_packet) {

		}

}