/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:09
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_stream_sender.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtp_stream_sender.h"

#include "spdlog/spdlog.h"

namespace RTC {
		// Limit max number of items in the retransmission buffer.
		static constexpr size_t kRetransmissionBufferMaxItems{ 2500u };
		// 17: 16 bit mask + the initial sequence number.
		static constexpr size_t kMaxRequestedPackets{ 17u };
		thread_local static std::vector<RTC::RtpRetransmissionBuffer::Item*> kRetransmissionContainer(
		    kMaxRequestedPackets + 1);
		static constexpr uint16_t kDefaultRtt{ 100u };

		const uint32_t RtpStreamSender::kMaxRetransmissionDelayForAudioMs{ 1000u };

		RtpStreamSender::RtpStreamSender(
		    Listener* listener, Params& params,
		    const std::shared_ptr<CoreIO::NetworkThread>& thread)
		    : RtpStream(listener, params, thread) {
				SPDLOG_TRACE();

				if (this->params_.use_nack) {
						this->retransmission_buffer_
						    = std::make_unique<RTC::RtpRetransmissionBuffer>(
						        kRetransmissionBufferMaxItems,
						        kMaxRetransmissionDelayForAudioMs, params.clock_rate);
				}
		}

		RtpStreamSender::~RtpStreamSender() = default;

		bool RtpStreamSender::ReceivePacket(RtpPacketPtr rtp_packet) {
				SPDLOG_TRACE();

				// Call the parent method.
				if (!RtpStream::ReceiveStreamPacket(rtp_packet)) {
						return false;
				}

				// If NACK is enabled, store the packet into the buffer.
				if (this->retransmission_buffer_) {
						StorePacket(rtp_packet);
				}

				// Increase transmission counter.
				this->wait_transmission_counter_.Update(rtp_packet);

				return true;
		}

		void RtpStreamSender::StorePacket(RtpPacketPtr& rtp_packet) const {
				SPDLOG_TRACE();

				if (rtp_packet->GetSize() > RTC::kMtuSize) {
						SPDLOG_WARN("packet too big [ssrc:%" PRIu32 ", seq:%" PRIu16
						            ", size:%zu]",
						            rtp_packet->GetSsrc(),
						            rtp_packet->GetSequenceNumber(),
						            rtp_packet->GetSize());

						return;
				}

				this->retransmission_buffer_->Insert(rtp_packet);
		}

		void RtpStreamSender::ReceiveNack(
		    const std::shared_ptr<RTCP::FeedbackRtpNackPacket>& nack_packet) {
				SPDLOG_TRACE();

				this->nack_count_++;

				for (auto it = nack_packet->Begin(); it != nack_packet->End(); ++it) {
						const auto& nack_item = *it;

						this->nack_packet_count_ += nack_item->CountRequestedPackets();

						FillRetransmissionContainer(nack_item->GetPacketId(),
						                            nack_item->GetLostPacketBitmask());

						for (const auto* item : kRetransmissionContainer) {
								if (!item) {
										break;
								}

								// Note that this is an already RTX encoded packet if RTX is
								// used (FillRetransmissionContainer() did it).
								auto rtp_packet = item->rtp_packet;

								// Retransmit the packet.
								dynamic_cast<RTC::RtpStreamSender::Listener*>(this->listener_)
								    ->OnRtpStreamRetransmitRtpPacket(this, rtp_packet);

								// Mark the packet as retransmitted.
								RTC::RtpStream::PacketRetransmitted(rtp_packet);
						}
				}
		}

		void RtpStreamSender::FillRetransmissionContainer(const uint16_t seq,
		                                                  uint16_t bitmask) const {
				SPDLOG_TRACE();

				// Ensure the container's first element is 0.
				kRetransmissionContainer[0] = nullptr;

				// If NACK is not supported, exit.
				if (!this->retransmission_buffer_) {
						SPDLOG_WARN("NACK not supported");

						return;
				}

				// Look for each requested packet.
				const uint64_t now_ms = RTCUtils::Time::GetMilliseconds();
				const uint16_t rtt
				    = (this->rtt_ > 0.0f ? static_cast<uint16_t>(this->rtt_)
				                         : kDefaultRtt);
				uint16_t current_seq = seq;
				bool requested{ true };
				size_t container_idx{ 0 };

				// Variables for debugging.
				const uint16_t origBitmask = bitmask;
				uint16_t sentBitmask{ 0b0000000000000000 };
				bool isFirstPacket{ true };
				bool firstPacketSent{ false };
				uint8_t bitmaskCounter{ 0 };

				while (requested || bitmask != 0) {
						bool sent = false;

						if (requested) {
								auto* item = this->retransmission_buffer_->Get(current_seq);
								RtpPacketPtr wait_send_packet{ nullptr };

								// 把新的信息设置进重传包中
								if (item) {
										wait_send_packet = item->rtp_packet;
										// Put correct info into the packet.
										wait_send_packet->SetSsrc(item->ssrc);
										wait_send_packet->SetSequenceNumber(item->sequence_number);
										wait_send_packet->SetTimestamp(item->timestamp);
								}

								// Packet not found.
								if (!item) {
										// Do nothing.
								} else if (item->resent_at_ms != 0u
								           && now_ms - item->resent_at_ms <= static_cast<uint64_t>(
								                  rtt)) // 小于一个rtt不会立即重传
								{
										SPDLOG_DEBUG(
										    "ignoring retransmission for a packet already resent in"
										    " the last RTT ms [seq:%" PRIu16 ", rtt:%" PRIu32 "]",
										    packet->GetSequenceNumber(),
										    rtt);
								}
								// Stored packet is valid for retransmission. Resend it.
								else
								{
										// Save when this packet was resent.
										item->resent_at_ms = now_ms;

										// Increase the number of times this packet was sent.
										item->sent_times++;

										// Store the item in the container and then increment its index.
										kRetransmissionContainer[container_idx++] = item;

										sent = true;

										if (isFirstPacket) {
												firstPacketSent = true;
										}
								}
						}

						requested = (bitmask & 1) != 0;
						bitmask >>= 1;
						++current_seq;

						if (!isFirstPacket) {
								sentBitmask |= (sent ? 1 : 0) << bitmaskCounter;
								++bitmaskCounter;
						} else {
								isFirstPacket = false;
						}
				}
				// Set the next container element to null.
				kRetransmissionContainer[container_idx] = nullptr;
		}
} // namespace RTC