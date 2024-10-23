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
				SPDLOG_TRACE();

				rtp_data_counter_.Update(rtp_packet);
		}

		uint32_t RtpStreamReceiver::TransmissionCounter::GetBitrate(
		    const uint64_t now_ms) {
				SPDLOG_TRACE();

				return rtp_data_counter_.GetBitrate(now_ms);
		}

		size_t RtpStreamReceiver::TransmissionCounter::GetPacketCount() const {
				SPDLOG_TRACE();

				return rtp_data_counter_.GetPacketCount();
		}

		size_t RtpStreamReceiver::TransmissionCounter::GetBytes() const {
				SPDLOG_TRACE();

				return rtp_data_counter_.GetBytes();
		}

		RtpStreamReceiver::RtpStreamReceiver(
		    Listener* listener, Params& params,
		    const std::shared_ptr<CoreIO::NetworkThread>& thread,
		    unsigned int send_nack_delay_ms)
		    : RtpStream(listener, params, thread) {
				if (params.use_nack) {
						this->nack_generator_ = std::make_unique<NackGenerator>(
						    this, thread, send_nack_delay_ms);
				}
		}

		RtpStreamReceiver::~RtpStreamReceiver() = default;

		bool RtpStreamReceiver::ReceivePacket(RtpPacketPtr& rtp_packet) {
				// Call the parent method.
				if (!RTC::RtpStream::ReceiveStreamPacket(rtp_packet)) {
						SPDLOG_WARN("packet discarded");

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

				std::string data_str((char*)rtp_packet->GetPayload());

				SPDLOG_INFO("receive data: {}", data_str);
				return true;
		}
} // namespace RTC