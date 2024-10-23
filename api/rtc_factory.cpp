/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-22 下午3:56
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_factory.cpp
 * @description : TODO
 *******************************************************/

#include "rtc_factory.h"

namespace RTCApi {
		RtcFactory::RtcFactory(DataCallBackObserver* listener) : listener_(listener) {
				this->network_thread_ = std::make_shared<CoreIO::NetworkThread>();
				network_thread_->Start();

				this->udp_socket_ = std::make_shared<CoreIO::UdpSocket>(
				    this->network_thread_, CoreIO::Type::SERVER, "0.0.0.0", 9001);
				this->udp_socket_->InitInvoke();

				this->rtc_transport_
				    = std::make_shared<RTC::RtcTransport>(this, this->network_thread_);

				this->udp_socket_->AddDispatcher(this->rtc_transport_);

				std::string ip("127.0.0.1");
				uint32_t port(9000);

				struct sockaddr_storage server_addr;
				int err
				    = uv_ip4_addr(ip.c_str(), port,
				                  reinterpret_cast<struct sockaddr_in*>(&server_addr));
				if (err != 0) {
						SPDLOG_ERROR("uv_ip4_addr() failed: {}", uv_strerror(err));
						return;
				}

				this->udp_remote_addr_ = server_addr;
		}

		bool RtcFactory::CreateRtpSenderStream(uint32_t ssrc) {
				this->rtc_transport_->CreateRtpStream(
				    ssrc, RTC::RtcTransport::StreamType::StreamSender);
				return true;
		}

		bool RtcFactory::DeleteRtpSenderStream(uint32_t ssrc) {
				this->rtc_transport_->DeleteRtpStream(ssrc);
				return true;
		}

		bool RtcFactory::CreateRtpReceiverStream(uint32_t ssrc) {
				this->rtc_transport_->CreateRtpStream(
				    ssrc, RTC::RtcTransport::StreamType::StreamReceiver);
				return true;
		}

		bool RtcFactory::DeleteRtpReceiverStream(uint32_t ssrc) {
				this->rtc_transport_->DeleteRtpStream(ssrc);
				return true;
		}

		void RtcFactory::OnSendPacket(uint32_t ssrc, uint8_t* data, uint32_t len) {
				this->rtc_transport_->OnsSendPacket(ssrc, data, len);
		}

		void RtcFactory::OnPacketReceived(uint8_t* data, uint32_t len) {
				this->listener_->OnReceiveAudio(data, len);
		}

		void RtcFactory::OnPacketSent(uint8_t* data, uint32_t len) {
				if (udp_socket_) {
						RTCUtils::CopyOnWriteBuffer buf(data, len);
						udp_socket_->Send(std::move(buf),
						                  (const struct sockaddr*)&this->udp_remote_addr_);
				}
		}

}