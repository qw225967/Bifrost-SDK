/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-22 下午3:56
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_factory.cpp
 * @description : TODO
 *******************************************************/

#include "rtc_factory.h"
#include "utils/cpp11_adaptor.h"
#include "utils/copy_on_write_buffer.h"

namespace RTCApi {

		std::unique_ptr<RtcInterface> RtcFactory::CreateRtc(
		    RtcInterface::DataCallBackObserver* listener,
		    const std::string& local_ip, int local_port) {
				return Cpp11Adaptor::make_unique<RtcFactory>(listener, local_ip,
				                                             local_port);
		}

		RtcFactory::RtcFactory(DataCallBackObserver* listener,
		                       const std::string& local_ip, int local_port)
		    : listener_(listener) {

				// 网络
				this->net_io_factory_ = Cpp11Adaptor::make_unique<CoreIO::NetIOFactory>();
				this->net_io_factory_->Init();
				this->network_thread_   = this->net_io_factory_->GetNetworkThread();
				websocket_client_ = this->net_io_factory_->CreateWebsocketClient("11.123.33.218", 80, "/wsapi/tts/stream?connId=333222111&adiu=321321321", false);
				if(!websocket_client_->ConnectInvoke().get()){
					SPDLOG_INFO("websocket client connect falsed");
					return;
				}

				// 数据回调线程，用于解码或者其他耗时处理
				this->packet_call_thread_
				    = Cpp11Adaptor::make_unique<RTC::CallBackQueueThread>(this, this->network_thread_);

				
				this->rtc_transport_ = std::make_shared<RTC::RtcTransport>(this, network_thread_);


				if (websocket_client_) {
						this->websocket_client_->AddDispatcher(this->rtc_transport_);
				}
		}

		RtcFactory::~RtcFactory() = default;

		bool RtcFactory::CreateRtpSenderStream(uint32_t ssrc, std::string target_ip,
		                                       int port, bool dynamic_addr) {
				if (this->rtc_transport_) {
						this->rtc_transport_->CreateRtpStream(
						    ssrc, RTC::RtcTransport::StreamType::StreamSender, target_ip, port,
						    dynamic_addr);
						return true;
				}
				return false;
		}

		bool RtcFactory::DeleteRtpSenderStream(uint32_t ssrc) {
				if (this->rtc_transport_) {
						this->rtc_transport_->DeleteRtpStream(ssrc);
						return true;
				}
				return false;
		}

		bool RtcFactory::CreateRtpReceiverStream(uint32_t ssrc, std::string target_ip,
		                                         int port, bool dynamic_addr) {
				if (this->rtc_transport_) {
						this->rtc_transport_->CreateRtpStream(
						    ssrc, RTC::RtcTransport::StreamType::StreamReceiver, target_ip,
						    port, dynamic_addr);
						return true;
				}
				return false;
		}

		bool RtcFactory::DeleteRtpReceiverStream(uint32_t ssrc) {
				if (this->rtc_transport_) {
						this->rtc_transport_->DeleteRtpStream(ssrc);
						return true;
				}
				return false;
		}

		void RtcFactory::OnSendAudio(uint32_t ssrc, uint8_t* data, uint32_t len) {
				if (this->rtc_transport_) {
						this->rtc_transport_->OnSendPacket(ssrc, data, len);
				}
		}

		void RtcFactory::OnSendText(uint8_t* data, uint32_t len) {
				SPDLOG_INFO("RtcFactory::OnSendText()");

				
				if (websocket_client_) {
						// websocket_client_->SendWebsocketMessage(data, len,
						//                                         FLAG_WEBSOCKET_TEXT);
				}
//					this->packet_call_thread_->ReceiveRtpPacket(3, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(2, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(6, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(4, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(1, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(0, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(5, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(7, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
//				  this->packet_call_thread_->ReceiveRtpPacket(8, RTCUtils::CopyOnWriteBuffer());
//				  usleep(200);
		}

		void RtcFactory::OnSendText(std::string data){
			if (websocket_client_) {
//				websocket_client_->SendTextInvoke(data);
										this->packet_call_thread_->ReceiveRtpPacket(2, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
									  this->packet_call_thread_->ReceiveRtpPacket(3, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
									  this->packet_call_thread_->ReceiveRtpPacket(4, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
									  this->packet_call_thread_->ReceiveRtpPacket(0, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
									  this->packet_call_thread_->ReceiveRtpPacket(1, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
//									  this->packet_call_thread_->ReceiveRtpPacket(0, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
//									  this->packet_call_thread_->ReceiveRtpPacket(5, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
//									  this->packet_call_thread_->ReceiveRtpPacket(7, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
//									  this->packet_call_thread_->ReceiveRtpPacket(8, RTCUtils::CopyOnWriteBuffer());
//									  usleep(200);
			}
		}

		void RtcFactory::OnPacketReceived(uint16_t seq, RTCUtils::CopyOnWriteBuffer buffer) {
				this->packet_call_thread_->ReceiveRtpPacket(seq, buffer);
		}

		void RtcFactory::OnPacketSent(uint8_t* data, uint32_t len,
		                              struct sockaddr_storage addr) {
				SPDLOG_INFO("RtcFactory::OnPacketSent()");
				if (udp_socket_) {
						RTCUtils::CopyOnWriteBuffer buf(data, len);
						udp_socket_->Send(std::move(buf), (const struct sockaddr*)&addr);
				}
		}

		void RtcFactory::HasContinuousPacketCall(
		    const RTCUtils::CopyOnWriteBuffer buffer) {
				if (buffer.size() < 20) {
						return;
				}
				auto temp    = const_cast<uint8_t*>(buffer.data());
				auto* header = reinterpret_cast<OpusFileHeader*>(temp);

				//				SPDLOG_INFO(RTCUtils::Byte::BytesToHex(data,
				//				                                       len));
				this->listener_->OnReceiveAudio(temp, header->total_len);
		}

} // namespace RTCApi