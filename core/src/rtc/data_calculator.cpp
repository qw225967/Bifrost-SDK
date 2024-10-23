/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午3:33
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : data_calculator.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/data_calculator.h"
#include "spdlog/spdlog.h"
#include "utils/utils.h"
#include <cmath> // std::trunc()

namespace RTC {
		void RateCalculator::Update(size_t size, uint64_t now_ms) {
				// SPDLOG_TRACE();

				// Ignore too old data. Should never happen.
				if (now_ms < this->oldest_item_start_time_) {
						return;
				}

				// Increase bytes.
				this->bytes_ += size;

				RemoveOldData(now_ms);

				// If the elapsed time from the newest item start time is greater than
				// the item size (in milliseconds), increase the item index.
				if (this->newest_item_index_ < 0
				    || now_ms - this->newest_item_start_time_ >= this->item_size_ms_)
				{
						this->newest_item_index_++;
						this->newest_item_start_time_ = now_ms;

						if (this->newest_item_index_ >= this->window_items_) {
								this->newest_item_index_ = 0;
						}

						// Set the newest item.
						BufferItem& item = this->buffer_[this->newest_item_index_];
						item.count       = size;
						item.time        = now_ms;
				} else {
						// Update the newest item.
						BufferItem& item = this->buffer_[this->newest_item_index_];
						item.count += size;
				}

				// Set the oldest item index and time, if not set.
				if (this->oldest_item_index_ < 0) {
						this->oldest_item_index_      = this->newest_item_index_;
						this->oldest_item_start_time_ = now_ms;
				}

				this->total_count_ += size;

				// Reset lastRate and lastTime so GetRate() will calculate rate again
				// even if called with same now in the same loop iteration.
				this->last_rate_ = 0;
				this->last_time_ = 0;
		}

		uint32_t RateCalculator::GetRate(uint64_t now_ms) {
				// SPDLOG_TRACE();

				if (now_ms == this->last_time_) {
						return this->last_rate_;
				}

				RemoveOldData(now_ms);

				const float scale = this->scale_ / this->window_size_ms_;

				this->last_time_ = now_ms;
				this->last_rate_ = static_cast<uint32_t>(
				    std::trunc(this->total_count_ * scale + 0.5f));

				return this->last_rate_;
		}

		inline void RateCalculator::RemoveOldData(uint64_t now_ms) {
				// SPDLOG_TRACE();

				// No item set.
				if (this->newest_item_index_ < 0 || this->oldest_item_index_ < 0) {
						return;
				}

				const uint64_t newOldestTime = now_ms - this->window_size_ms_;

				// Oldest item already removed.
				if (newOldestTime < this->oldest_item_start_time_) {
						return;
				}

				// A whole window size time has elapsed since last entry. Reset the buffer.
				if (newOldestTime >= this->newest_item_start_time_) {
						Reset();

						return;
				}

				while (newOldestTime >= this->oldest_item_start_time_) {
						BufferItem& oldestItem = this->buffer_[this->oldest_item_index_];
						this->total_count_ -= oldestItem.count;
						oldestItem.count = 0u;
						oldestItem.time  = 0u;

						if (++this->oldest_item_index_ >= this->window_items_) {
								this->oldest_item_index_ = 0;
						}

						const BufferItem& newOldestItem
						    = this->buffer_[this->oldest_item_index_];
						this->oldest_item_start_time_ = newOldestItem.time;
				}
		}

		void RtpDataCounter::Update(const RtpPacketPtr& rtp_packet) {
				const uint64_t nowMs = RTCUtils::Time::GetTimeMs();

				this->packets++;
				this->rate.Update(rtp_packet->GetSize(), nowMs);
		}
} // namespace RTC