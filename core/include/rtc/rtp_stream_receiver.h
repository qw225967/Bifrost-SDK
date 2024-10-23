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
						virtual void OnRtpStreamReceiveRtpPacket(RtpStreamReceiver* rtp_stream,
						                                       RtpPacketPtr rtp_packet)
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
				                  unsigned int send_nack_delay_ms, bool dynamic_addr);
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
				    const std::shared_ptr<RTCP::SenderReport>& report);
				std::shared_ptr<RTCP::ReceiverReport> GetRtcpReceiverReport();

		private:
				[[nodiscard]] uint32_t GetExpectedPackets() const {
						return (this->cycles_ + this->max_seq_) - this->base_seq_ + 1;
				}

		private:
				std::unique_ptr<RTC::NackGenerator> nack_generator_;

				// 网络抖动
				float jitter_{ 0 };
				// 周期预期接包计算
				uint32_t expected_prior_{ 0u };
				// 周期实际接包计算
				uint32_t received_prior_{ 0u };
				// 上报丢包数
				uint32_t reported_packet_lost_{ 0u };
				// 最新接到sr的本机定时器的时间戳
				uint64_t last_sr_received_{ 0u };
				// 最新接收sr中记录的时间戳
				uint32_t last_sr_timestamp_{ 0u };
				// 传输计算
				RtpDataCounter media_transmission_counter_;

			  bool is_dynamic_addr_{ false };
		};
		typedef std::shared_ptr<RtpStreamReceiver> RtpStreamReceiverPtr;
} // namespace RTC

#endif // RTP_STREAM_RECEIVE_H
