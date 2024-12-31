/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_transport.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtc_transport.h"
#include "rtc/rtcp/compound_packet.h"
#include "rtc/rtp_stream_receiver.h"
#include "rtc/rtp_stream_sender.h"
#include "utils/copy_on_write_buffer.h"
#include "utils/stringex.hpp"

#include <stack>

namespace RTC {
		static constexpr unsigned int kSendNackDelay{ 10u }; // In ms.
		static constexpr size_t KRtpMinHeaderLength  = 12;
		static constexpr size_t kRtcpMinHeaderLength = 4;
		static constexpr uint8_t kExpectedVersion    = 2;
		static constexpr uint64_t kTimerRtcpIntervalMs{ 1000u }; // ms

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
		    : listener_(listener), thread_(thread),
		      rtcp_send_timer_(new RTCUtils::TimerHandle(this, thread)) {
				// 初始化定时器
				this->rtcp_send_timer_->InitInvoke();
				this->rtcp_send_timer_->StartInvoke(kTimerRtcpIntervalMs,
				                                    kTimerRtcpIntervalMs);

				// 立刻发送一个rr触发媒体流建联
				// this->SendRtcpPacket();
		}
		RtcTransport::~RtcTransport() {
				this->rtcp_send_timer_->StopInvoke();
				this->rtcp_send_timer_->CloseInvoke();
				delete rtcp_send_timer_;
				rtcp_send_timer_ = nullptr;
		}

		void RtcTransport::CreateRtpStream(const uint32_t ssrc,
		                                   const StreamType stream_type,
		                                   std::string target_ip, int port,
		                                   bool dynamic_addr) {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				// 初始化目标网络传输信息
				struct sockaddr_storage target_addr;
				int err
				    = uv_ip4_addr(target_ip.c_str(), port,
				                  reinterpret_cast<struct sockaddr_in*>(&target_addr));
				if (err != 0) {
						// SPDLOG_ERROR("uv_ip4_addr() failed: {}", uv_strerror(err));
						return;
				}

				switch (stream_type) {
				case StreamType::StreamReceiver:
				{
						// 所有的流ssrc不能重复
						auto stream_ite = this->rtp_receive_streams_.find(ssrc);
						if (stream_ite != this->rtp_receive_streams_.end()) {
								// SPDLOG_WARN("rtc transport already exists");
								return;
						}

						// 参数：流标识，载荷类型，采样率，是否使用nack
						RtpStream::Params params(ssrc, 100, 16000, true);

						// 创建接收流
						RtpStreamReceiverPtr rtp_stream = std::make_shared<RtpStreamReceiver>(
						    this, params, this->thread_, kSendNackDelay, dynamic_addr);
						rtp_stream->SetUdpRemoteTargetAddr(target_addr);

						this->rtp_receive_streams_.emplace(ssrc, rtp_stream);
						break;
				}

				case StreamType::StreamSender:
				{
						// 所有的流ssrc不能重复
						auto stream_ite = this->rtp_send_streams_.find(ssrc);
						if (stream_ite != this->rtp_send_streams_.end()) {
								// SPDLOG_WARN("rtc transport already exists");
								return;
						}

						// 参数：流标识，载荷类型，采样率，是否使用nack
						RtpStream::Params params(ssrc, 100, 16000, true);

						// 创建接收流
						RtpStreamSenderPtr rtp_stream = std::make_shared<RtpStreamSender>(
						    this, params, this->thread_, dynamic_addr);
						rtp_stream->SetUdpRemoteTargetAddr(target_addr);

						this->rtp_send_streams_.emplace(ssrc, rtp_stream);

						break;
				}

				default:
				{
						// SPDLOG_WARN("Unknown stream type: {}",
						//           static_cast<uint8_t>(stream_type));
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
				// SPDLOG_TRACE();

				switch (protocol) {
				case CoreIO::Protocol::UDP:
				{
						if (IsRtcp(data, len)) {
								auto rtcp_packet = RTCP::RtcpPacket::Parse(data, len);

								this->ReceiveRtcpPacket(rtcp_packet, addr);
						} else if (IsRtp(data, len)) {
								auto rtp_packet = RtpPacket::Parse(data, len);

								this->ReceiveRtpPacket(rtp_packet, addr);
						} else {
								// SPDLOG_WARN("Unknown packet");
						}
						break;
				}
				case CoreIO::Protocol::TCP:
				{
						break;
				}
				case CoreIO::Protocol::WEBSOCKET:
				{
					if (socket->IsClosed()) {
						return;
					}

					auto rtp_packet       = RtpPacket::Parse(data, len);
					//
					// // SPDLOG_INFO(RTCUtils::Byte::BytesToHex(data, len));

					if (!rtp_packet) {
						// SPDLOG_WARN("rtp packet is null");
//						// SPDLOG_WARN(RTCUtils::Byte::BytesToHex(data, len));
						return;
					}
					this->ReceiveRtpPacket(rtp_packet, addr);

					break;
				}
				case CoreIO::Protocol::WEBSOCKET_TEXT:
				{
						if (socket->IsClosed()) {
								return;
						}
						WebsocketEventResponse response;

						this->ReadResponseJson(response, std::string((char*)data, len));
						this->listener_->OnReceiveEvent();

						break;
				}
				default:
				{
						// SPDLOG_WARN("Unknown protocol: {}", uint8_t(protocol));
						break;
				}
				}
		}

		std::pair<size_t, size_t> RtcTransport::FindParentheses(std::string text) {
				std::stack<std::pair<char, size_t>> parentheses;
				size_t i = 0;

				for (; i < text.size(); i++) {
						if (text[i] == '{') {
								parentheses.push(std::pair<char, size_t>(text[i], i));
						}
						if (text[i] == '}') {
								if (parentheses.size() == 1) {
										auto result = parentheses.top();
										return std::pair<size_t, size_t>(result.second + 1, i);
								}
								parentheses.pop();
						}
				}
		}

		void RtcTransport::ReadResponseJson(WebsocketEventResponse& response, std::string text) {
				auto json_str = text.substr(1, text.size() - 2);

				// 解析头部
				auto pos_header = text.find("header");
				if (pos_header != std::string::npos) {
						auto header_str  = text.substr(pos_header + 8);
						auto split_index = FindParentheses(header_str);
						header_str       = header_str.substr(
                split_index.first, split_index.second - split_index.first);

						std::vector<std::string> lines_vec;
						cpp_streamer::StringSplit(header_str, ",", lines_vec);
						for (auto ite = lines_vec.begin(); ite != lines_vec.end(); ite++)
						{
								auto message_type_pos = ite->find("messageType");
								if (message_type_pos != std::string::npos) {
										response.header.message_type
										    = ite->substr(message_type_pos + 14,
										                  ite->size() - (message_type_pos + 15));
								}

								auto connect_id_pos = ite->find("connectId");
								if (connect_id_pos != std::string::npos) {
										response.header.connect_id
										    = ite->substr(connect_id_pos + 12,
										                  ite->size() - (connect_id_pos + 13));
								}

								auto status_message_pos = ite->find("statusMessage");
								if (status_message_pos != std::string::npos) {
										response.header.status_message = ite->substr(
										    status_message_pos + 16,
										    ite->size() - (status_message_pos + 17));
								}

								auto status_pos = ite->find("status\"");
								if (status_pos != std::string::npos) {
										auto size              = ite->size();
										response.header.status = std::stoi(ite->substr(
										    status_pos + 8, size - (status_pos + 8)));
								}
						}
				}

				// 解析载荷
				// 解析头部
				auto pos_paylad = text.find("payload");
				if (pos_paylad != std::string::npos) {
						auto payload_str = text.substr(pos_header + 9);
						auto split_index = FindParentheses(payload_str);
						payload_str      = payload_str.substr(
                split_index.first, split_index.second - split_index.first);

						std::vector<std::string> lines_vec;
						cpp_streamer::StringSplit(payload_str, ",", lines_vec);
						for (auto ite = lines_vec.begin(); ite != lines_vec.end(); ite++)
						{
								auto request_id_pos = ite->find("requestId");
								if (request_id_pos != std::string::npos) {
										response.payload.request_id
										    = ite->substr(request_id_pos + 12,
										                  ite->size() - (request_id_pos + 13));
								}

								auto simple_rate_pos = ite->find("simpleRate");
								if (simple_rate_pos != std::string::npos) {
										response.payload.simple_rate
										    = ite->substr(simple_rate_pos + 13,
										                  ite->size() - (simple_rate_pos + 14));
								}

								auto pack_overlap_pos = ite->find("packOverlap");
								if (pack_overlap_pos != std::string::npos) {
										response.payload.pack_overlap
										    = ite->substr(pack_overlap_pos + 14,
										                  ite->size() - (pack_overlap_pos + 15));
								}
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

				// 临时测试TODO:后续改掉
				auto it = this->rtp_receive_streams_.find(ssrc);
				if (it == this->rtp_receive_streams_.end()) {
						return rtp_receive_streams_.begin()->second;
				}

				return it->second;
		}

		void RtcTransport::ReceiveRtcpPacket(RTCP::RtcpPacketPtr& rtcp_packet,
		                                     const struct sockaddr* addr) {
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
								// SPDLOG_INFO("SR receive ssrc: {}", report->GetSsrc());

								if (!rtp_stream.get()) {
										// SPDLOG_DEBUG(
										//  "no Consumer found for received Sender Report "
										//  "[ssrc:{}]",
										//  report->GetSsrc());

										continue;
								}

								rtp_stream->ReceiveRtcpSenderReport(report);

								// 更新addr地址
								if (addr) {
										rtp_stream->SetUdpRemoteTargetAddr(*(
										    reinterpret_cast<const struct sockaddr_storage*>(addr)));
								}
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
								// SPDLOG_INFO("RR receive ssrc: {}", report->GetSsrc());

								if (!rtp_stream.get()) {
										// SPDLOG_DEBUG(
										//   "no Consumer found for received Receiver Report "
										//  "[ssrc:{}]",
										//  report->GetSsrc());

										continue;
								}

								rtp_stream->ReceiveRtcpReceiverReport(report);

								// 更新addr地址
								if (addr) {
										rtp_stream->SetUdpRemoteTargetAddr(*(
										    reinterpret_cast<const struct sockaddr_storage*>(addr)));
								}
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

		void RtcTransport::ReceiveRtpPacket(RtpPacketPtr& rtp_packet,
		                                    const struct sockaddr* addr) {
				if (const auto rtp_send_stream
				    = this->GetStreamSenderBySsrc(rtp_packet->GetSsrc()))
				{
						rtp_send_stream->ReceivePacket(rtp_packet);
				}
				if (const auto rtp_receive_stream
				    = this->GetStreamReceiverBySsrc(rtp_packet->GetSsrc()))
				{
						rtp_receive_stream->ReceivePacket(rtp_packet);
				}
		}

		void RtcTransport::OnSendPacket(uint32_t ssrc, uint8_t* data, uint32_t len) {
				const auto rtp_stream = this->GetStreamSenderBySsrc(ssrc);

				if (!rtp_stream.get()) {
						return;
				}

				auto rtp_packet = std::make_shared<RtpPacket>(RtpPacket::Header());

				// SPDLOG_INFO("send packet seq: {}", test_seq_);

				rtp_packet->CopyPayload(data, len);
				rtp_packet->SetVersion(2);
				rtp_packet->SetSsrc(ssrc);
				rtp_packet->SetSequenceNumber(test_seq_++);
				rtp_packet->SetTimestamp(timestamp_ += 100);
				rtp_packet->SetPayloadType(100);

				rtp_stream->ReceivePacket(rtp_packet);
		}

		void RtcTransport::SendRtcpPacket() {
				std::unique_lock<std::mutex> lock(thread_mutex_);

				for (auto& rtp_receive_stream : this->rtp_receive_streams_) {
						auto rtcp_packet = Cpp11Adaptor::make_unique<RTCP::CompoundPacket>();
						auto receive_report
						    = rtp_receive_stream.second->GetRtcpReceiverReport();
						if (receive_report) {
								rtcp_packet->AddReceiverReport(receive_report);
						}

						uint8_t data[rtcp_packet->GetSize()];
						rtcp_packet->Serialize(data);

						this->listener_->OnPacketSent(
						    const_cast<uint8_t*>(rtcp_packet->GetData()),
						    rtcp_packet->GetSize(),
						    rtp_receive_stream.second->GetUdpRemoteTargetAddr());
				}

				for (auto& rtp_send_stream : this->rtp_send_streams_) {
						auto rtcp_packet = Cpp11Adaptor::make_unique<RTCP::CompoundPacket>();
						auto send_report = rtp_send_stream.second->GetRtcpSenderReport(
						    RTCUtils::Time::GetTimeMs());
						if (send_report) {
								rtcp_packet->AddSenderReport(send_report);
						}

						uint8_t data[rtcp_packet->GetSize()];
						rtcp_packet->Serialize(data);

						this->listener_->OnPacketSent(
						    const_cast<uint8_t*>(rtcp_packet->GetData()),
						    rtcp_packet->GetSize(),
						    rtp_send_stream.second->GetUdpRemoteTargetAddr());
				}
		}

		void RtcTransport::OnTimer(RTCUtils::TimerHandle* timer) {
				if (timer == this->rtcp_send_timer_) {
						// this->SendRtcpPacket();
				}
		}

}