/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/11/8 14:30
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : callback_queue_thread.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/callback_queue_thread.h"

namespace RTC {
		static uint16_t kCheckFrameHasCall = 40u; // ms
		CallBackQueueThread::CallBackQueueThread(
		    Listener* listener, std::shared_ptr<CoreIO::NetworkThread> network_thread)
		    : listener_(listener),
		      check_frame_timer_(new RTCUtils::TimerHandle(this, network_thread)) {
				this->check_frame_timer_->InitInvoke();
				this->check_frame_timer_->StartInvoke(kCheckFrameHasCall, kCheckFrameHasCall);

				this->thread_queue_ = Cpp11Adaptor::make_unique<ThreadRtpPacketQueue>();

				thread_ = Cpp11Adaptor::make_unique<std::thread>(
				    [this]() { this->TryCallData(); });
		}

		CallBackQueueThread::~CallBackQueueThread() {
				this->check_frame_timer_->StopInvoke();
				this->check_frame_timer_->CloseInvoke();
				delete this->check_frame_timer_;

				this->running_ = false;

				if (thread_) {
						thread_->join();
				}
		}

		void CallBackQueueThread::ReceiveRtpPacket(
		    uint16_t seq, const RTCUtils::CopyOnWriteBuffer& buffer) {
				this->thread_queue_->Push(seq, buffer);
				wait_pop_frames_ = wait_pop_frames_ + 1;
		}

		void CallBackQueueThread::TryCallData() {
				while (this->running_) {
						std::vector<RTCUtils::CopyOnWriteBuffer> buffers;
						if (this->thread_queue_->Pop(buffers)) {
								auto buffer = buffers.begin();
								while (buffer != buffers.end()) {
										this->listener_->HasContinuousPacketCall(*buffer);
										usleep(40000);
										wait_pop_frames_ = wait_pop_frames_ - 1;
										buffer++;
								}
						}
				}
		}


		void CallBackQueueThread::OnTimer(RTCUtils::TimerHandle* timer) {
				if (timer == this->check_frame_timer_) {
						if (this->thread_queue_->FindContinuousPacket()) {
								printf("[rtc] ontimer find continuous \n");
						}
				}
		}
} // namespace RTC
