/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:08
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_stream_receive.h
 * @description : TODO
 *******************************************************/

#ifndef RTP_STREAM_RECEIVE_H
#define RTP_STREAM_RECEIVE_H

#include "rtcp/receiver_report_packet.h"
#include "rtcp/sender_report_packet.h"
#include "rtc/data_calculator.h"
#include "rtc/nack_generator.h"
#include "rtc/rtcp/rtcp_packet.h"
#include "rtc/rtp_stream.h"

#include "utils/time_handler.h"

namespace RTC {
		class RtpStreamReceiver : public RtpStream,
		                          public NackGenerator::Listener,
		                          public RTCUtils::TimerHandle::Listener {
		public:
				class Listener : public RtpStream::Listener {
				public:
						virtual void OnRtpStreamSendRtcpPacket(RtpStreamReceiver* rtp_stream,
						                                       RTCP::RtcpPacketPtr rtcp_packet)
						    = 0;
				};

		public:
				class TransmissionCounter {
				public:
						explicit TransmissionCounter(size_t window_size);
						void Update(const RtpPacketPtr& rtp_packet);
						uint32_t GetBitrate(uint64_t now_ms);
						[[nodiscard]] size_t GetPacketCount() const;
						[[nodiscard]] size_t GetBytes() const;

				private:
						RtpDataCounter rtp_data_counter_;
				};

		public:
				RtpStreamReceiver(Listener* listener, Params& params,
				                  const std::shared_ptr<CoreIO::NetworkThread>& thread,
				                  unsigned int send_nack_delay_ms);
				~RtpStreamReceiver() override;

				// TimerHandle::Listener
				void OnTimer(RTCUtils::TimerHandle* timer) override {
				}

				// NackGenerator::Listener
				void OnNackGeneratorNackRequired(
				    const std::vector<uint16_t>& seq_numbers) override {
				}
				void OnNackGeneratorKeyFrameRequired() override {
				}

		public:
				bool ReceivePacket(RtpPacketPtr& packet);
				void ReceiveRtcpSenderReport(
				    const std::shared_ptr<RTCP::SenderReport>& report) {
				}

		private:
				std::unique_ptr<RTC::NackGenerator> nack_generator_;
		};
		typedef std::shared_ptr<RtpStreamReceiver> RtpStreamReceiverPtr;
} // namespace RTC

#endif // RTP_STREAM_RECEIVE_H
