/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:08
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_stream_receiver.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtp_stream_receiver.h"

#include "spdlog/spdlog.h"

namespace RTC {
		RtpStreamReceiver::TransmissionCounter::TransmissionCounter(
		    const size_t window_size)
		    : rtp_data_counter_(window_size) {
		}

		void RtpStreamReceiver::TransmissionCounter::Update(
		    const RtpPacketPtr& rtp_packet) {
				// SPDLOG_TRACE();

				rtp_data_counter_.Update(rtp_packet);
		}

		uint32_t RtpStreamReceiver::TransmissionCounter::GetBitrate(
		    const uint64_t now_ms) {
				// SPDLOG_TRACE();

				return rtp_data_counter_.GetBitrate(now_ms);
		}

		size_t RtpStreamReceiver::TransmissionCounter::GetPacketCount() const {
				// SPDLOG_TRACE();

				return rtp_data_counter_.GetPacketCount();
		}

		size_t RtpStreamReceiver::TransmissionCounter::GetBytes() const {
				// SPDLOG_TRACE();

				return rtp_data_counter_.GetBytes();
		}

		RtpStreamReceiver::RtpStreamReceiver(
		    Listener* listener, Params& params,
		    const std::shared_ptr<CoreIO::NetworkThread>& thread,
		    unsigned int send_nack_delay_ms, bool dynamic_addr)
		    : RtpStream(listener, params, thread), is_dynamic_addr_(dynamic_addr) {
				if (params.use_nack) {
						this->nack_generator_ = Cpp11Adaptor::make_unique<NackGenerator>(
						    this, thread, send_nack_delay_ms);
				}
		}

		RtpStreamReceiver::~RtpStreamReceiver() = default;

		bool RtpStreamReceiver::ReceivePacket(RtpPacketPtr& rtp_packet) {
				// Call the parent method.
				if (!RTC::RtpStream::ReceiveStreamPacket(rtp_packet)) {
						// SPDLOG_WARN("packet discarded");

						return false;
				}

				// Pass the packet to the NackGenerator.
				if (this->params_.use_nack) {
						if (this->nack_generator_->ReceivePacket(rtp_packet,
						                                         /*is_recovered*/ false))
						{
								// Mark the packet as retransmitted and repaired.
								RtpStream::PacketRetransmitted(rtp_packet);
						}
				}

				this->media_transmission_counter_.Update(rtp_packet);

				 SPDLOG_INFO("receive rtp packet seq:{}, size:{}, ssrc:{}, timstamp:{}",
				           rtp_packet->GetSequenceNumber(), rtp_packet->GetSize(),
				          rtp_packet->GetSsrc(), rtp_packet->GetTimestamp());

				dynamic_cast<RTC::RtpStreamReceiver::Listener*>(this->listener_)
				    ->OnRtpStreamReceiveRtpPacket(this, rtp_packet);

				//				// SPDLOG_INFO(RTCUtils::Byte::BytesToHex(rtp_packet->GetData(),
				//rtp_packet->GetSize()));

				return true;
		}

		std::shared_ptr<RTCP::ReceiverReport> RtpStreamReceiver::GetRtcpReceiverReport() {
				// SPDLOG_TRACE();
				auto report = std::make_shared<RTCP::ReceiverReport>();

				report->SetSsrc(GetSsrc());

				const uint32_t prev_packets_lost = this->packets_lost_;

				// Calculate Packets Expected and Lost.
				auto expected = GetExpectedPackets();

				if (expected > this->media_transmission_counter_.GetPacketCount()) {
						this->packets_lost_
						    = expected - this->media_transmission_counter_.GetPacketCount();
				} else {
						this->packets_lost_ = 0u;
				}

				// Calculate Fraction Lost.
				const uint32_t expected_interval = expected - this->expected_prior_;

				this->expected_prior_ = expected;

				const uint32_t received_interval
				    = this->media_transmission_counter_.GetPacketCount()
				      - this->received_prior_;

				this->received_prior_
				    = this->media_transmission_counter_.GetPacketCount();

				const int32_t lostInterval = expected_interval - received_interval;

				if (expected_interval == 0 || lostInterval <= 0) {
						this->fraction_lost_ = 0;
				} else {
						this->fraction_lost_ = std::round(
						    (static_cast<double>(lostInterval << 8) / expected_interval));
				}

				this->reported_packet_lost_ += (this->packets_lost_ - prev_packets_lost);

				report->SetTotalLost(this->reported_packet_lost_);
				report->SetFractionLost(this->fraction_lost_);

				// Fill the rest of the report.
				report->SetLastSeq(static_cast<uint32_t>(this->max_seq_) + this->cycles_);
				report->SetJitter(this->jitter_);

				if (this->last_sr_received_ != 0) {
						// Get delay in milliseconds.
						auto delayMs = static_cast<uint32_t>(RTCUtils::Time::GetTimeMs()
						                                     - this->last_sr_received_);
						// Express delay in units of 1/65536 seconds.
						uint32_t dlsr = (delayMs / 1000) << 16;

						dlsr |= uint32_t{ (delayMs % 1000) * 65536 / 1000 };

						report->SetDelaySinceLastSenderReport(dlsr);
						report->SetLastSenderReport(this->last_sr_timestamp_);
				} else {
						report->SetDelaySinceLastSenderReport(0);
						report->SetLastSenderReport(0);
				}

				return report;
		}

		void RtpStreamReceiver::ReceiveRtcpSenderReport(
		    const std::shared_ptr<RTCP::SenderReport>& report) {
				// SPDLOG_TRACE();

				this->last_sr_received_  = RTCUtils::Time::GetTimeMs();
				this->last_sr_timestamp_ = report->GetNtpSec() << 16;
				this->last_sr_timestamp_ += report->GetNtpFrac() >> 16;

				// Update info about last Sender Report.
				RTCUtils::Time::Ntp ntp{}; // NOLINT(cppcoreguidelines-pro-type-member-init)

				ntp.seconds   = report->GetNtpSec();
				ntp.fractions = report->GetNtpFrac();

				this->last_sender_report_ntp_ms_ = RTCUtils::Time::Ntp2TimeMs(ntp);
				this->last_sender_report_ts_     = report->GetRtpTs();

				// SPDLOG_INFO(
				//   "receive last_sr_timestamp:{}, last_sender_report_ntp_ms:{}, last_sender_report_ts:{}",
				//  this->last_sr_timestamp_, this->last_sender_report_ntp_ms_,
				//  this->last_sender_report_ts_);
		}
} // namespace RTC