/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-22 下午3:56
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_factory.cpp
 * @description : TODO
 *******************************************************/

#include "rtc_factory.h"
#include "utils/copy_on_write_buffer.h"
#include "utils/cpp11_adaptor.h"

#include <cmath>
#include <algorithm>

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
				std::ostringstream uri;
				uri << "/wsapi/tts/stream?dip=" << 321 // 产品代码
				    << "&div=" << 321 << "&tid=" << 321 << "&diu=" << 321
				    << "&adiu=" << 321 << "&keepAlive=" << 30 << "&sign=" << 321;

				// 网络
				this->net_io_factory_ = Cpp11Adaptor::make_unique<CoreIO::NetIOFactory>();
				this->net_io_factory_->Init();
				this->network_thread_ = this->net_io_factory_->GetNetworkThread();
				websocket_client_     = this->net_io_factory_->CreateWebsocketClient(
            "pre-traffic-tts-gateway.amap.com", 443,
            uri.str() /*"/wsapi/tts/stream?connId=333222111&adiu=321321321"*/,
            true);
				if (!websocket_client_->ConnectInvoke().get()) {
						SPDLOG_INFO("websocket client connect falsed");
						return;
				}

				// 数据回调线程，用于解码或者其他耗时处理
				this->packet_call_thread_
				    = Cpp11Adaptor::make_unique<RTC::CallBackQueueThread>(
				        this, this->network_thread_);

				this->rtc_transport_
				    = std::make_shared<RTC::RtcTransport>(this, network_thread_);

				if (websocket_client_) {
						this->websocket_client_->AddDispatcher(this->rtc_transport_);
				}
		}

		RtcFactory::~RtcFactory() = default;

		bool RtcFactory::CreateRtpSenderStream(uint32_t ssrc, std::string target_ip,
		                                       int port, bool dynamic_addr) {
				if (this->rtc_transport_) {
						this->rtc_transport_->CreateRtpStream(
						    ssrc, RTC::RtcTransport::StreamType::StreamSender, target_ip,
						    port, dynamic_addr);
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
				//					this->packet_call_thread_->ReceiveRtpPacket(3,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(2,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(6,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(4,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(1,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(0,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(5,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(7,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
				//				  this->packet_call_thread_->ReceiveRtpPacket(8,
				// RTCUtils::CopyOnWriteBuffer()); 				  usleep(200);
		}

		void RtcFactory::OnSendText(std::string data) {
				SPDLOG_INFO(data);
				if (websocket_client_) {
						websocket_client_->SendTextInvoke(data);
						//										this->packet_call_thread_->ReceiveRtpPacket(2,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(3,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(4,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(0,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(1,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(0,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(5,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(7,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
						//									  this->packet_call_thread_->ReceiveRtpPacket(8,
						// RTCUtils::CopyOnWriteBuffer()); 									  usleep(200);
				}
		}

		void RtcFactory::OnPacketReceived(uint16_t seq,
		                                  RTCUtils::CopyOnWriteBuffer buffer) {
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

		std::string RtcFactory::GetConnectId() {
				return this->websocket_client_->GetWebsocketEventResponse().header.connect_id;
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

		// DecodedSmoothPCMQueue
		DecodedSmoothPCMQueue::DecodedSmoothPCMQueue(size_t overlap_frame_size)
		    : overlap_frame_size_(overlap_frame_size) {
		}
		DecodedSmoothPCMQueue::~DecodedSmoothPCMQueue() {
				auto ite = frames_.begin();
				while (ite != frames_.end()) {
						auto frame = *ite;
						delete[] frame.data;
						frame.data = nullptr;

						ite = frames_.erase(ite);
				}
		}

		void DecodedSmoothPCMQueue::Push(char* data, size_t len, bool is_need_smooth) {
				auto* frame_data = new uint8_t[len];
				memcpy(frame_data, data, len);

				RestoreFrame frame;
				frame.data        = frame_data;
				frame.smooth_flag = is_need_smooth;
				this->frame_size_ = len;
				if (is_need_smooth) {
						this->smooth_frame_count_++;
				}

				this->frames_.push_back(frame);
		}
		bool DecodedSmoothPCMQueue::Pop(char* data, size_t& len, bool &range_pop) {
				if (this->frames_.empty()) {
						return false;
				}

				auto ite = frames_.begin();
				if (ite != frames_.end() && !ite->smooth_flag) {
						memcpy(data, ite->data, this->frame_size_);
						len = this->frame_size_;

						delete[] ite->data;
						ite->data = nullptr;

						frames_.erase(ite);
						return true;
				}

				// 两倍代表前后两段都已经准备就绪，需要进行加权平滑
				if (this->smooth_frame_count_ >= this->overlap_frame_size_ * 2) {

						std::vector<float> pre = GetOverLapSizeFrameToFloat();
						std::vector<float> last = GetOverLapSizeFrameToFloat();

						std::vector<float> new_data = SmoothPCM(pre, last);

						// 重新拷贝
						auto cp_ite = new_data.begin();
						while(cp_ite != new_data.end()) {
								auto* frame_data = new uint8_t[this->frame_size_];
								auto* cp_ptr = frame_data;
								auto cp_len      = this->frame_size_;

								while (cp_len > 0) {
										memcpy(cp_ptr, &(*cp_ite), 4);

										cp_ptr += 4;
										cp_len -= 4;

										cp_ite = new_data.erase(cp_ite);
								}

								RestoreFrame smooth_frame;
								smooth_frame.data        = frame_data;
								smooth_frame.smooth_flag = false;

								this->frames_.push_back(smooth_frame);
						}
						range_pop = true;
				}

				return false;
		}

		std::vector<float> DecodedSmoothPCMQueue::GetOverLapSizeFrameToFloat() {
				std::vector<float> segment;

				auto ite = this->frames_.begin();
				for (int i = 0;
				     i < this->overlap_frame_size_ && ite != this->frames_.end(); i++)
				{
						auto frame = *ite;

						auto cp_len = this->frame_size_;
						auto data   = frame.data;
						while (cp_len > 0) {
								float two_sample = 0.;
								memcpy(&two_sample, data, 4);
								segment.push_back(two_sample / 32768.0f);

								data += 4;
								cp_len -= 4;
						}

						delete [] frame.data;
						frame.data = nullptr;

						ite = this->frames_.erase(ite);
				}

				return segment;
		}

		std::vector<float> DecodedSmoothPCMQueue::SmoothPCM(
		    std::vector<float>& segment_pre, std::vector<float>& segment_last) {
				// 平滑两个 PCM 段的重叠区域
				std::vector<float> smooth_pcm;
				auto overlap = segment_pre.size();

				// 创建权重
				std::vector<float> weights1(overlap);
				std::vector<float> weights2(overlap);
				std::vector<float> smoothed_overlap(overlap);

				// 线性空间填充权重
				float step1 = 1.0f / (overlap - 1);
				float step2 = 1.0f / (overlap - 1);
				for (int i = 0; i < overlap; ++i) {
						weights1[i] = 1.0f - step1 * i;
						weights2[i] = step2 * i;
				}

				// 加权平均
				for (int i = 0; i < overlap; ++i) {
						smoothed_overlap[i] = ((segment_pre[i] * weights1[i] + segment_last[i] * weights2[i]) / (weights1[i] + weights2[i])) * 32768.0f;
				}

				return smoothed_overlap;
		}

		bool DecodedSmoothPCMQueue::ForcePop(char* data, size_t& len, bool &range_pop) {
				if (this->frames_.empty()) {
						return false;
				}

				auto ite = frames_.begin();
				if (ite != frames_.end()) {
						memcpy(data, ite->data, this->frame_size_);
						len = this->frame_size_;

						delete[] ite->data;
						ite->data = nullptr;

						frames_.erase(ite);
						return true;
				}

				return false;
		}

} // namespace RTCApi