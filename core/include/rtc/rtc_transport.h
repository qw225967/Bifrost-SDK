/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 12:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_transport.h
 * @description : TODO
 *******************************************************/

#ifndef RTC_TRANSPORT_H
#define RTC_TRANSPORT_H

#include "io/packet_dispatcher_interface.h"
#include "rtp_stream_receiver.h"
#include "rtp_stream_sender.h"
#include "rtc/rtp_stream.h"

namespace RTC {
		class RtcTransport : public CoreIO::PacketDispatcherInterface,
		                     public RtpStreamReceiver::Listener,
		                     public RtpStreamSender::Listener {
		public:
				enum class StreamType { StreamSender, StreamReceiver };

				class Listener {
				public:
						virtual ~Listener() = default;
						virtual void OnPacketReceived(uint8_t* data, uint32_t len) = 0;

						virtual void OnPacketSent(uint8_t* data, uint32_t len) = 0;
				};

		public:
				explicit RtcTransport(Listener* listener, const std::shared_ptr<CoreIO::NetworkThread>& thread);
				~RtcTransport() override;

		public:
				// 继承函数
				// CoreIO::PacketDispatcherInterface
				void Dispatch(uint8_t* data,
				              size_t len,
				              CoreIO::SocketInterface* socket,
				              CoreIO::Protocol protocol,
				              const struct sockaddr* addr = nullptr) override;

				// RtpStreamSender
				void OnRtpStreamRetransmitRtpPacket(RtpStreamSender* rtpStream,
				                                    RtpPacketPtr rtp_packet) override {
				}
				// RtpStreamReceiver
				void OnRtpStreamSendRtcpPacket(RtpStreamReceiver* rtp_stream,
				                               RTCP::RtcpPacketPtr rtcp_packet) override {
				}

				// RtpStream
				void OnRtpStreamScore(RtpStream* rtp_stream,
				                      uint8_t score,
				                      uint8_t previous_score) override {
				}

		public:
				// 创建流
				void CreateRtpStream(uint32_t ssrc, StreamType stream_type);
				// 删除流
				void DeleteRtpStream(uint32_t ssrc);

				// 发送数据
				void OnsSendPacket(uint32_t ssrc, uint8_t* data, uint32_t len);

		private:
				RtpStreamSenderPtr GetStreamSenderBySsrc(uint32_t ssrc);
				RtpStreamReceiverPtr GetStreamReceiverBySsrc(uint32_t ssrc);
				void ReceiveRtcpPacket(RTCP::RtcpPacketPtr& rtcp_packet);
				void ReceiveRtpPacket(RtpPacketPtr& rtp_packet);

		private:
				// 记录网络线程对象
				std::shared_ptr<CoreIO::NetworkThread> thread_{ nullptr };
				std::mutex thread_mutex_;

				// 记录流信息
				std::unordered_map<uint32_t, RtpStreamSenderPtr> rtp_send_streams_;
				std::unordered_map<uint32_t, RtpStreamReceiverPtr> rtp_receive_streams_;

				Listener* listener_{nullptr};
				uint16_t test_seq_{0u};
				uint32_t timestamp_{ 2000u };
		};
} // namespace RTC

#endif // RTC_TRANSPORT_H
