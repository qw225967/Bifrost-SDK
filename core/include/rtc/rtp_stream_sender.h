/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:09
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_stream_sender.h
 * @description : TODO
 *******************************************************/

#ifndef RTP_STREAM_SENDER_H
#define RTP_STREAM_SENDER_H

#include "rtcp/nack_packet.h"
#include "rtp_retransmission_buffer.h"
#include "rtp_stream.h"

#include "data_calculator.h"

namespace RTC {
		class RtpStreamSender : public RtpStream {
		public:
				// 最大重传音频延迟
				const static uint32_t kMaxRetransmissionDelayForAudioMs;

		public:
				class Listener : public RtpStream::Listener {
				public:
						virtual void OnRtpStreamRetransmitRtpPacket(RtpStreamSender* rtpStream,
						                                            RtpPacketPtr rtp_packet)
						    = 0;
				};

		public:
				RtpStreamSender(Listener* listener, Params& params,
				                const std::shared_ptr<CoreIO::NetworkThread>& thread);
				~RtpStreamSender() override;

				bool ReceivePacket(RtpPacketPtr rtp_packet);
				void ReceiveNack(
				    const std::shared_ptr<RTCP::FeedbackRtpNackPacket>& nack_packet);
				void ReceiveRtcpReceiverReport(
				    const std::shared_ptr<RTCP::ReceiverReport>& report) {
				}

		private:
				void StorePacket(RtpPacketPtr& rtp_packet) const;
				void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask) const;

		private:
				std::unique_ptr<RTC::RtpRetransmissionBuffer> retransmission_buffer_{
						nullptr
				};

				RTC::RtpDataCounter wait_transmission_counter_; // 待发送统计
				RTC::RtpDataCounter sent_transmission_counter_; // 已发送统计
				size_t nack_count_{ 0u };
				size_t nack_packet_count_{ 0u };
		};

		typedef std::shared_ptr<RtpStreamSender> RtpStreamSenderPtr;
} // namespace RTC

#endif // RTP_STREAM_SENDER_H
