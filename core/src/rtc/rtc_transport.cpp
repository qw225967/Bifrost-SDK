/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_transport.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtc_transport.h"
#include "rtc/rtp_stream_receiver.h"
#include "rtc/rtp_stream_sender.h"

namespace RTC {
		static constexpr unsigned int kSendNackDelay{ 10u }; // In ms.
		static constexpr size_t KRtpMinHeaderLength  = 12;
		static constexpr size_t kRtcpMinHeaderLength = 4;
		static constexpr uint8_t kExpectedVersion    = 2;

		static bool IsRtp(const uint8_t* data, size_t len) {
				return len >= KRtpMinHeaderLength && (data[0] > 127 && data[0] < 192)
				       && (data[0] >> 6) == kExpectedVersion;
		}

		static bool IsRtcp(const uint8_t* data, size_t len) {
				return len >= kRtcpMinHeaderLength && (data[0] > 127 && data[0] < 192)
				       && ((data[0] >> 6) == kExpectedVersion)
				       && (data[1] >= 192 && data[1] <= 223);
		}

		RtcTransport::RtcTransport(Listener* listener,
		                           const std::shared_ptr<CoreIO::NetworkThread>& thread)
		    : listener_(listener), thread_(thread) {
		}
		RtcTransport::~RtcTransport() = default;

		void RtcTransport::CreateRtpStream(const uint32_t ssrc,
		                                   const StreamType stream_type) {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				switch (stream_type) {
				case StreamType::StreamReceiver:
				{
						// 所有的流ssrc不能重复
						auto stream_ite = this->rtp_receive_streams_.find(ssrc);
						if (stream_ite != this->rtp_receive_streams_.end()) {
								SPDLOG_WARN("rtc transport already exists");
								return;
						}

						// 参数：流标识，载荷类型，采样率，是否使用nack
						RtpStream::Params params(ssrc, 100, 16000, true);

						// 创建接收流
						RtpStreamReceiverPtr rtp_stream = std::make_shared<RtpStreamReceiver>(
						    this, params, this->thread_, kSendNackDelay);

						this->rtp_receive_streams_.emplace(ssrc, rtp_stream);
						break;
				}

				case StreamType::StreamSender:
				{
						// 所有的流ssrc不能重复
						auto stream_ite = this->rtp_send_streams_.find(ssrc);
						if (stream_ite != this->rtp_send_streams_.end()) {
								SPDLOG_WARN("rtc transport already exists");
								return;
						}

						// 参数：流标识，载荷类型，采样率，是否使用nack
						RtpStream::Params params(ssrc, 100, 16000, true);

						// 创建接收流
						RtpStreamSenderPtr rtp_stream
						    = std::make_shared<RtpStreamSender>(this, params, this->thread_);

						this->rtp_send_streams_.emplace(ssrc, rtp_stream);

						break;
				}

				default:
				{
						SPDLOG_WARN("Unknown stream type: {}", uint8_t(stream_type));
				}
				}
		}
		void RtcTransport::DeleteRtpStream(const uint32_t ssrc) {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				auto stream_send_ite = this->rtp_send_streams_.find(ssrc);
				if (stream_send_ite != this->rtp_send_streams_.end()) {
						this->rtp_send_streams_.erase(ssrc);
						return;
				}

				auto stream_receive_ite = this->rtp_receive_streams_.find(ssrc);
				if (stream_receive_ite != this->rtp_receive_streams_.end()) {
						this->rtp_receive_streams_.erase(ssrc);
						return;
				}
		}

		void RtcTransport::Dispatch(uint8_t* data,
		                            size_t len,
		                            CoreIO::SocketInterface* socket,
		                            CoreIO::Protocol protocol,
		                            const struct sockaddr* addr) {
				SPDLOG_TRACE();

				switch (protocol) {
				case CoreIO::Protocol::UDP:
				{
						if (IsRtcp(data, len)) {
								auto rtcp_packet = RTCP::RtcpPacket::Parse(data, len);

								this->ReceiveRtcpPacket(rtcp_packet);
						} else if (IsRtp(data, len)) {
								auto rtp_packet = RtpPacket::Parse(data, len);

								this->ReceiveRtpPacket(rtp_packet);
						} else {
								SPDLOG_WARN("Unknown packet");
						}
						break;
				}
				case CoreIO::Protocol::TCP:
				{
						break;
				}
				case CoreIO::Protocol::WEBSOCKET:
				{
						break;
				}
				default:
				{
						SPDLOG_WARN("Unknown protocol: {}", uint8_t(protocol));
						break;
				}
				}
		}

		RtpStreamSenderPtr RtcTransport::GetStreamSenderBySsrc(uint32_t ssrc) {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				auto it = this->rtp_send_streams_.find(ssrc);
				if (it == this->rtp_send_streams_.end()) {
						return nullptr;
				}

				return it->second;
		}
		RtpStreamReceiverPtr RtcTransport::GetStreamReceiverBySsrc(uint32_t ssrc) {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				auto it = this->rtp_receive_streams_.find(ssrc);
				if (it == this->rtp_receive_streams_.end()) {
						return nullptr;
				}

				return it->second;
		}

		void RtcTransport::ReceiveRtcpPacket(RTCP::RtcpPacketPtr& rtcp_packet) {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				auto rtcp_type = rtcp_packet->GetType();

				switch (rtcp_type) {
				case RTCP::Type::SR:
				{
						auto sr_packet = std::dynamic_pointer_cast<RTCP::SenderReportPacket>(
						    rtcp_packet);

						for (auto it = sr_packet->Begin(); it != sr_packet->End(); ++it) {
								auto& report = *it;
								auto rtp_stream
								    = this->GetStreamReceiverBySsrc(report->GetSsrc());

								if (!rtp_stream.get()) {
										SPDLOG_DEBUG(
										    "no Consumer found for received Sender Report "
										    "[ssrc:%" PRIu32 "]",
										    report->GetSsrc());

										continue;
								}

								rtp_stream->ReceiveRtcpSenderReport(report);
						}

						break;
				}
				case RTCP::Type::RR:
				{
						auto rr_packet
						    = std::dynamic_pointer_cast<RTCP::ReceiverReportPacket>(
						        rtcp_packet);

						for (auto it = rr_packet->Begin(); it != rr_packet->End(); ++it) {
								auto& report = *it;
								auto rtp_stream = this->GetStreamSenderBySsrc(report->GetSsrc());

								if (!rtp_stream.get()) {
										SPDLOG_DEBUG(
										    "no Consumer found for received Receiver Report "
										    "[ssrc:%" PRIu32 "]",
										    report->GetSsrc());

										continue;
								}

								rtp_stream->ReceiveRtcpReceiverReport(report);
						}
						break;
				}
				case RTCP::Type::SDES:
				{
						break;
				}
				case RTCP::Type::BYE:
				{
						break;
				}
				case RTCP::Type::APP:
				{
						break;
				}
				case RTCP::Type::RTPFB:
				{
						break;
				}
				case RTCP::Type::PSFB:
				{
						break;
				}
				case RTCP::Type::XR:
				{
						break;
				}
				case RTCP::Type::NAT:
				{
						break;
				}
				default:
				{
						break;
				}
				}
		}

		void RtcTransport::ReceiveRtpPacket(RtpPacketPtr& rtp_packet) {
				if (const auto rtp_send_stream
				    = this->GetStreamSenderBySsrc(rtp_packet->GetSsrc()))
				{
						rtp_send_stream->ReceivePacket(rtp_packet);
				} else if (const auto rtp_receive_stream
				           = this->GetStreamReceiverBySsrc(rtp_packet->GetSsrc()))
				{
						rtp_receive_stream->ReceivePacket(rtp_packet);
				}
		}

		void RtcTransport::OnsSendPacket(uint32_t ssrc, uint8_t* data, uint32_t len) {
				auto rtp_send_stream = this->GetStreamSenderBySsrc(ssrc);
				if (!rtp_send_stream.get()) {
						return;
				}

				auto rtp_packet = std::make_shared<RtpPacket>(RtpPacket::Header());

				SPDLOG_INFO("send packet seq: {}", test_seq_);

				rtp_packet->CopyPayload(data, len);
				rtp_packet->SetVersion(2);
				rtp_packet->SetSsrc(ssrc);
				rtp_packet->SetSequenceNumber(test_seq_++);
				rtp_packet->SetTimestamp(timestamp_ += 100);
				rtp_packet->SetPayloadType(100);

				rtp_send_stream->ReceivePacket(rtp_packet);

			  this->listener_->OnPacketSent(const_cast<uint8_t*>(rtp_packet->GetData()),
			  	rtp_packet->GetSize());
		}

}