/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-22 下午3:56
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_factory.h
 * @description : TODO
 *******************************************************/

#ifndef RTC_FACTORY_H
#define RTC_FACTORY_H

#include "io/udp_socket.h"
#include "rtc_interface.h"
#include "rtc/rtc_transport.h"

#include <memory>

namespace RTCApi {
		class RtcFactory final : public RtcInterface,
		                         public RTC::RtcTransport::Listener {
		public:
				static std::unique_ptr<RtcInterface> CreateRtc(
				    RtcInterface::DataCallBackObserver* listener, std::string target_ip,
				    int port);

		public:
				// RtcInterface
				bool CreateRtpSenderStream(uint32_t ssrc) override;

				bool DeleteRtpSenderStream(uint32_t ssrc) override;

				bool CreateRtpReceiverStream(uint32_t ssrc) override;

				bool DeleteRtpReceiverStream(uint32_t ssrc) override;

				void OnSendAudio(uint32_t ssrc, uint8_t* data, uint32_t len) override;

				void OnSendText(uint32_t ssrc, uint8_t* data, uint32_t len) override;

				// RtcTransport
				void OnPacketReceived(uint8_t* data, uint32_t len) override;

				void OnPacketSent(uint8_t* data, uint32_t len,
				                  struct sockaddr_storage addr) override;

		public:
				explicit RtcFactory(DataCallBackObserver* listener,
				                    const std::string& target_ip, int port);
				~RtcFactory() override;

		private:
				std::shared_ptr<CoreIO::UdpSocket> udp_socket_;

				std::shared_ptr<CoreIO::NetworkThread> network_thread_;

				std::shared_ptr<RTC::RtcTransport> rtc_transport_;

				DataCallBackObserver* listener_;
		};

} // namespace RTCApi

#endif // RTC_FACTORY_H
