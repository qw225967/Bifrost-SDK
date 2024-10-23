/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/11/8 11:28
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : callback_queue_thread.h
 * @description : TODO
 *******************************************************/

#ifndef DEMO_TEST_CALLBACK_QUEUE_THREAD_H
#define DEMO_TEST_CALLBACK_QUEUE_THREAD_H

#include "spdlog/spdlog.h"
#include "utils/copy_on_write_buffer.h"
#include "utils/time_handler.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace RTC {
		static bool IsLargeSeq(uint16_t small, uint16_t big, bool& has_routing) {
				if (big > small) {
						if (big - small > 65535 / 2) {
								return false; // 异常情况，翻转跳跃了一个很大的数
						} else {
								return true; // 正常情况
						}
				} else {
						if (small - big > 65535 / 2) {
								has_routing = true;
								return true; // 正常情况 small > big, 数据翻转到从 0 开始，因此也是大数
						} else {
								return false; // 异常情况 small > big,
								              // 小于65535/2那么这个数可能跳跃了很大一个数
						}
				}
		}

		class ThreadRtpPacketQueue {
		private:
				std::map<uint32_t, RTCUtils::CopyOnWriteBuffer> packet_map_;
				mutable std::mutex mutex_;
				std::condition_variable is_empty_;
				std::condition_variable is_disorder_;
				bool is_start_{ true };

				uint32_t cycle_{ 0 };
				uint16_t max_seq_{ 0 };
				uint32_t waite_pop_seq_{ 0 };

		public:
				ThreadRtpPacketQueue() = default;

				// 禁止复制和赋值
				ThreadRtpPacketQueue(const ThreadRtpPacketQueue&)            = delete;
				ThreadRtpPacketQueue& operator=(const ThreadRtpPacketQueue&) = delete;

				void ResetPopSeq() {
						this->waite_pop_seq_ = 0;
				}

				// 入队操作
				void Push(uint16_t seq, const RTCUtils::CopyOnWriteBuffer& packet_buffer) {
						std::lock_guard<std::mutex> lock(mutex_);
						if (this->is_start_) {
								this->max_seq_  = seq;
								this->is_start_ = false;
						}

						bool has_routing = false;
						if (IsLargeSeq(max_seq_, seq, has_routing)) {
								max_seq_ = seq;
						}
						if (has_routing) {
								this->cycle_++;
						}
						uint32_t large_seq = (cycle_ << 16) + seq;
						SPDLOG_INFO("push seq:{}", large_seq);
						packet_map_.emplace(large_seq, packet_buffer);
						is_empty_.notify_one(); // 通知一个等待的出队线程

						this->FindContinuousPacket();
				}

				// 出队操作
				bool Pop(std::vector<RTCUtils::CopyOnWriteBuffer>& packet_buffers) {
						std::unique_lock<std::mutex> lock(mutex_);
						is_empty_.wait(lock, [this] {
								return !packet_map_.empty();
						}); // 等待直到队列不为空
						if (packet_map_.empty()) {
								return false;
						}

						// 只有第一个弹出后，才需要确定有序情况
						is_disorder_.wait(lock);

						auto begin = this->packet_map_.begin();
						while (begin != this->packet_map_.end()
						       && waite_pop_seq_ == begin->first)
						{
								packet_buffers.push_back(std::move(begin->second));
								waite_pop_seq_ = waite_pop_seq_ + 1;
								SPDLOG_INFO("pop seq:{}", begin->first);
								begin = this->packet_map_.erase(begin);
								usleep(200);
						}

						return true;
				}

				bool FindContinuousPacket() {
						if (packet_map_.empty()) {
								return false;
						}

						auto begin = this->packet_map_.begin();
						if (begin->first == waite_pop_seq_) {
								is_disorder_.notify_one();
								return true;
						}
						return false;
				}

				// 检查队列是否为空
				bool Empty() const {
						std::lock_guard<std::mutex> lock(mutex_);
						return packet_map_.empty();
				}
		};

		class CallBackQueueThread : public RTCUtils::TimerHandle::Listener {
		public:
				class Listener {
				public:
						virtual void HasContinuousPacketCall(RTCUtils::CopyOnWriteBuffer buffer)
						    = 0;
						virtual void ReceiveDataFinishCall() = 0;
				};

		public:
				CallBackQueueThread(
				    Listener* listener, std::shared_ptr<CoreIO::NetworkThread> thread);
				~CallBackQueueThread();

				void ResetFrameSequence() {
						this->thread_queue_->ResetPopSeq();
				}

				void ReceiveRtpPacket(uint16_t seq,
				                      const RTCUtils::CopyOnWriteBuffer& buffer);

				void OnTimer(RTCUtils::TimerHandle* timer) override;

		private:
				void TryCallData();

		private:
				volatile u_int16_t wait_pop_frames_{ 0 };
				std::unique_ptr<std::thread> thread_;
				std::unique_ptr<ThreadRtpPacketQueue> thread_queue_;
				Listener* listener_{ nullptr };
				volatile bool running_{ true };

				RTCUtils::TimerHandle* check_frame_timer_{ nullptr };
		};
} // namespace RTC

#endif // DEMO_TEST_CALLBACK_QUEUE_THREAD_H
