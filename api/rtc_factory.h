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

#include "io/net_io_factory.h"
#include "rtc_interface.h"
#include "rtc/callback_queue_thread.h"
#include "rtc/rtc_transport.h"

#include <memory>
#include <vector>

namespace RTCApi {
		class RtcFactory final : public RtcInterface,
		                         public RTC::RtcTransport::Listener,
		                         public RTC::CallBackQueueThread::Listener {
		private:
				struct OpusFileHeader {
						uint16_t total_len;
						uint16_t tag_len;
				};

		public:
				static std::unique_ptr<RtcInterface> CreateRtc(
				    RtcInterface::DataCallBackObserver* listener,
				    const std::string& local_ip, int local_port);

		public:
				// RtcInterface
				bool CreateRtpSenderStream(uint32_t ssrc, std::string target_ip,
				                           int port, bool dynamic_addr) override;

				bool DeleteRtpSenderStream(uint32_t ssrc) override;

				bool CreateRtpReceiverStream(uint32_t ssrc, std::string target_ip,
				                             int port, bool dynamic_addr) override;

				bool DeleteRtpReceiverStream(uint32_t ssrc) override;

				void OnSendAudio(uint32_t ssrc, uint8_t* data, uint32_t len) override;

				void OnSendText(uint8_t* data, uint32_t len) override;
				void OnSendText(std::string data) override;

				std::string GetConnectId() override;

				// RtcTransport
				void OnPacketReceived(uint16_t seq,
				                      RTCUtils::CopyOnWriteBuffer buffer) override;

				void OnPacketSent(uint8_t* data, uint32_t len,
				                  struct sockaddr_storage addr) override;

				void OnReceiveEvent() override {}

				// CallBackQueue
				void HasContinuousPacketCall(const RTCUtils::CopyOnWriteBuffer buffer) override;
				void ReceiveDataFinishCall() override {
						this->listener_->OnReceiveEvent();
				}

		public:
				explicit RtcFactory(DataCallBackObserver* listener,
				                    const std::string& local_ip, int local_port);
				~RtcFactory() override;

		private:
				// 网络io
				std::unique_ptr<CoreIO::NetIOFactory> net_io_factory_;
				std::shared_ptr<CoreIO::UdpSocket> udp_socket_;
				std::shared_ptr<CoreIO::WebsocketClient> websocket_client_;

				// 线程对象
				std::shared_ptr<CoreIO::NetworkThread> network_thread_;

				// rtc传输对象
				std::shared_ptr<RTC::RtcTransport> rtc_transport_;

				// 处理数据线程
				std::unique_ptr<RTC::CallBackQueueThread> packet_call_thread_;

				// 监听
				DataCallBackObserver* listener_;
		};

		class DecodedSmoothPCMQueue {
		private:
				struct RestoreFrame {
						uint8_t* data{ nullptr };
						bool smooth_flag{ false };
				};

		public:
				DecodedSmoothPCMQueue(size_t overlap_frame_size);
				~DecodedSmoothPCMQueue();

				bool IsEmpty() { return this->frames_.empty(); }
				void SetAllFrameNeedSmooth() {
						auto ite = this->frames_.begin();
						while(ite != this->frames_.end()) {
								ite->smooth_flag = true;
								this->smooth_frame_count_++;
								ite++;
						}
				}

				void Push(char* data, size_t len, bool is_need_smooth);
				bool Pop(char* data, size_t& len, bool &range_pop);
				bool ForcePop(char* data, size_t& len, bool &range_pop);

		private:
				std::vector<float> SmoothPCM(std::vector<float>& segment_pre,
				                             std::vector<float>& segment_last);
				std::vector<float> GetOverLapSizeFrameToFloat();

		private:
				size_t overlap_frame_size_{ 0 };
				size_t smooth_frame_count_{ 0 };
				size_t frame_size_{ 0 };
				std::vector<RestoreFrame> frames_;
		};

} // namespace RTCApi

#endif // RTC_FACTORY_H
