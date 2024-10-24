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
		std::unique_ptr<RtcInterface> RtcFactory::CreateRtc(
		    RtcInterface::DataCallBackObserver* listener,
		    const std::string& local_ip, int local_port) {
				return std::make_unique<RtcFactory>(listener, local_ip, local_port);
		}

		RtcFactory::RtcFactory(DataCallBackObserver* listener,
		                       const std::string& local_ip, int local_port)
		    : listener_(listener) {
				this->network_thread_ = std::make_shared<CoreIO::NetworkThread>();
				network_thread_->Start();

				this->udp_socket_ = std::make_shared<CoreIO::UdpSocket>(
				    this->network_thread_, CoreIO::Type::SERVER, local_ip, local_port);
				this->udp_socket_->InitInvoke();

				this->rtc_transport_
				    = std::make_shared<RTC::RtcTransport>(this, this->network_thread_);

				this->udp_socket_->AddDispatcher(this->rtc_transport_);
		}

		RtcFactory::~RtcFactory() = default;

		bool RtcFactory::CreateRtpSenderStream(uint32_t ssrc, std::string target_ip,
		                                       int port, bool dynamic_addr) {
				this->rtc_transport_->CreateRtpStream(
				    ssrc, RTC::RtcTransport::StreamType::StreamSender, target_ip, port,
				    dynamic_addr);
				return true;
		}

		bool RtcFactory::DeleteRtpSenderStream(uint32_t ssrc) {
				this->rtc_transport_->DeleteRtpStream(ssrc);
				return true;
		}

		bool RtcFactory::CreateRtpReceiverStream(uint32_t ssrc, std::string target_ip,
		                                         int port, bool dynamic_addr) {
				this->rtc_transport_->CreateRtpStream(
				    ssrc, RTC::RtcTransport::StreamType::StreamReceiver, target_ip,
				    port, dynamic_addr);
				return true;
		}

		bool RtcFactory::DeleteRtpReceiverStream(uint32_t ssrc) {
				this->rtc_transport_->DeleteRtpStream(ssrc);
				return true;
		}

		void RtcFactory::OnSendAudio(uint32_t ssrc, uint8_t* data, uint32_t len) {
				this->rtc_transport_->OnsSendPacket(ssrc, data, len);
		}

		void RtcFactory::OnSendText(uint32_t ssrc, uint8_t* data, uint32_t len) {
				this->rtc_transport_->OnsSendPacket(ssrc, data, len);
		}

		void RtcFactory::OnPacketReceived(uint8_t* data, uint32_t len) {
				this->listener_->OnReceiveAudio(data, len);
		}

		void RtcFactory::OnPacketSent(uint8_t* data, uint32_t len,
		                              struct sockaddr_storage addr) {
				SPDLOG_INFO("RtcFactory::OnPacketSent()");
				if (udp_socket_) {
						RTCUtils::CopyOnWriteBuffer buf(data, len);
						udp_socket_->Send(std::move(buf), (const struct sockaddr*)&addr);
				}
		}

}