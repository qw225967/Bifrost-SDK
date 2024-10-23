/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午3:33
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : data_calculator.h
 * @description : TODO
 *******************************************************/

#ifndef DATA_CALCULATOR_H
#define DATA_CALCULATOR_H

#include "rtc_common.h"
#include "rtc/rtp_packet.h"
#include <vector>

namespace RTC {
		// It is considered that the time source increases monotonically.
		// ie: the current timestamp can never be minor than a timestamp in the past.
		class RateCalculator {
		public:
				static constexpr size_t kDefaultWindowSize{ 1000u };
				static constexpr float kDefaultBpsScale{ 8000.0f };
				static constexpr uint16_t kDefaultWindowItems{ 100u };

		public:
				explicit RateCalculator(size_t window_size_ms = kDefaultWindowSize,
				                        float scale           = kDefaultBpsScale,
				                        uint16_t window_items = kDefaultWindowItems)
				    : window_size_ms_(window_size_ms), scale_(scale),
				      window_items_(window_items) {
						this->item_size_ms_ = std::max(window_size_ms / window_items,
						                               static_cast<size_t>(1));
						this->buffer_.resize(window_items);
				}
				void Update(size_t size, uint64_t now_ms);
				uint32_t GetRate(uint64_t now_ms);
				[[nodiscard]] size_t GetBytes() const {
						return this->bytes_;
				}

		private:
				void RemoveOldData(uint64_t now_ms);
				void Reset() {
						std::memset(static_cast<void*>(&this->buffer_.front()),
						            0,
						            sizeof(BufferItem) * this->buffer_.size());

						this->newest_item_start_time_ = 0u;
						this->newest_item_index_      = -1;
						this->oldest_item_start_time_ = 0u;
						this->oldest_item_index_      = -1;
						this->total_count_            = 0u;
						this->last_rate_              = 0u;
						this->last_time_              = 0u;
				}

		private:
				struct BufferItem {
						size_t count{ 0u };
						uint64_t time{ 0u };
				};

		private:
				// Window Size (in milliseconds).
				size_t window_size_ms_{ kDefaultWindowSize };
				// Scale in which the rate is represented.
				float scale_{ kDefaultBpsScale };
				// Window Size (number of items).
				uint16_t window_items_{ kDefaultWindowItems };
				// Item Size (in milliseconds), calculated as: windowSizeMs / windowItems.
				size_t item_size_ms_{ 0u };
				// Buffer to keep data.
				std::vector<BufferItem> buffer_;
				// Time (in milliseconds) for last item in the time window.
				uint64_t newest_item_start_time_{ 0u };
				// Index for the last item in the time window.
				int32_t newest_item_index_{ -1 };
				// Time (in milliseconds) for oldest item in the time window.
				uint64_t oldest_item_start_time_{ 0u };
				// Index for the oldest item in the time window.
				int32_t oldest_item_index_{ -1 };
				// Total count in the time window.
				size_t total_count_{ 0u };
				// Total bytes transmitted.
				size_t bytes_{ 0u };
				// Last value calculated by GetRate().
				uint32_t last_rate_{ 0u };
				// Last time GetRate() was called.
				uint64_t last_time_{ 0u };
		};

		class RtpDataCounter {
		public:
				explicit RtpDataCounter(size_t window_size_ms = 2500)
				    : rate(window_size_ms) {
				}

		public:
				void Update(const RtpPacketPtr& rtp_packet);
				uint32_t GetBitrate(uint64_t now_ms) {
						return this->rate.GetRate(now_ms);
				}
				[[nodiscard]] size_t GetPacketCount() const {
						return this->packets;
				}
				[[nodiscard]] size_t GetBytes() const {
						return this->rate.GetBytes();
				}

		private:
				RateCalculator rate;
				size_t packets{ 0u };
		};
} // namespace RTC

#endif // DATA_CALCULATOR_H
