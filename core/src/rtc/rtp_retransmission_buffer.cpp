/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午5:19
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_retransmission_buffer.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtp_retransmission_buffer.h"
#include "rtc/seq_manager.h"

#include "spdlog/spdlog.h"

namespace RTC {
		/* Instance methods. */

		RtpRetransmissionBuffer::RtpRetransmissionBuffer(
		    uint16_t max_items,
		    uint32_t max_retransmission_delay_ms,
		    uint32_t clock_rate)
		    : max_items_(max_items),
		      max_retransmission_delay_ms_(max_retransmission_delay_ms),
		      clock_rate_(clock_rate) {
				SPDLOG_TRACE();
		}

		RtpRetransmissionBuffer::~RtpRetransmissionBuffer() {
				SPDLOG_TRACE();

				Clear();
		}

		RtpRetransmissionBuffer::Item* RtpRetransmissionBuffer::Get(uint16_t seq) const {
				SPDLOG_TRACE();

				const auto oldest_item = GetOldest();

				if (!oldest_item) {
						return nullptr;
				}

				if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(
				        seq, oldest_item->sequence_number))
				{
						return nullptr;
				}

				const uint16_t idx = seq - oldest_item->sequence_number;

				if (static_cast<size_t>(idx) > this->buffer_.size() - 1) {
						return nullptr;
				}

				return this->buffer_.at(idx);
		}

		/**
		 * This method tries to insert given packet into the buffer. Here we assume
		 * that packet seq number is legitimate according to the content of the buffer.
		 * We discard the packet if too old and also discard it if its timestamp does
		 * not properly fit (by ensuring that elements in the buffer are not only
		 * ordered by increasing seq but also that their timestamp are incremental).
		 */
		void RtpRetransmissionBuffer::Insert(RtpPacketPtr& rtp_packet) {
				SPDLOG_TRACE();

				const auto ssrc      = rtp_packet->GetSsrc();
				const auto seq       = rtp_packet->GetSequenceNumber();
				const auto timestamp = rtp_packet->GetTimestamp();

				SPDLOG_DEBUG(
				    "packet [seq:%" PRIu16 ", timestamp:%" PRIu32 "]", seq, timestamp);

				// 1.当前缓存是空的直接插入新内容
				if (this->buffer_.empty()) {
						SPDLOG_DEBUG("buffer empty [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
						             seq,
						             timestamp);

						auto* item = new Item(rtp_packet, ssrc, seq, timestamp, 0, 0);

						this->buffer_.push_back(item);

						return;
				}

				auto* oldest_item = GetOldest();
				auto* newest_item = GetNewest();

				// 2.处理特殊case
				// 特殊case：接收到的数据包的序列号比缓冲区中最新的数据包低，但其时间戳却更高。如果是这样，清空整个缓冲区。
				// 证明发送端可能重置了时间戳。
				if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(
				        seq, newest_item->sequence_number)
				    && RTC::SeqManager<uint32_t>::IsSeqHigherThan(
				        timestamp, newest_item->timestamp))
				{
						SPDLOG_WARN(
						    "packet has lower seq but higher timestamp than newest "
						    "packet in the buffer, emptying the buffer [ssrc:%" PRIu32
						    ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
						    ssrc,
						    seq,
						    timestamp);

						Clear();

						auto* item = new Item(rtp_packet, ssrc, seq, timestamp, 0, 0);

						this->buffer_.push_back(item);

						return;
				}

				// 3.清除buffer中的旧数据。
				// 注意：在这种情况下，我们必须考虑由于大量数据包丢失，
				// 接收到的数据包具有更高的时间戳但序列号比缓冲区中最新的数据包“更旧”，
				// 如果是这样，使用它来清除过时的数据包，而不是缓冲区中最新的数据包。
				auto newest_timestamp = RTC::SeqManager<uint32_t>::IsSeqHigherThan(
				                            timestamp, newest_item->timestamp)
				                            ? timestamp
				                            : newest_item->timestamp;

				// ClearTooOldByTimestamp() returns true if at least one packet has been
				// removed from the front.
				if (ClearTooOldByTimestamp(newest_timestamp)) {
						// Buffer content has been modified so we must check it again.
						if (this->buffer_.empty()) {
								SPDLOG_WARN(
								    "buffer empty after clearing too old packets"
								    " [seq:{}, timestamp:{}]",
								    seq,
								    timestamp);

								auto* item = new Item(rtp_packet, ssrc, seq, timestamp, 0, 0);

								this->buffer_.push_back(item);

								return;
						}

						oldest_item = GetOldest();
						newest_item = GetNewest();
				}

				// 4.包有序到达，数据则正常放入
				if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(
				        seq, newest_item->sequence_number))
				{
						SPDLOG_DEBUG("packet in order [seq:%" PRIu16 ", timestamp:%" PRIu32
						             "]",
						             seq,
						             timestamp);

						// Ensure that the timestamp of the packet is equal or higher than
						// the timestamp of the newest stored packet.
						if (RTC::SeqManager<uint32_t>::IsSeqLowerThan(
						        timestamp, newest_item->timestamp))
						{
								SPDLOG_WARN(
								    "packet has higher seq but lower timestamp than newest packet "
								    "in the buffer, discarding it [ssrc:{}, seq:{}, timestamp:{}]",
								    ssrc,
								    seq,
								    timestamp);

								return;
						}

						// 计算空白槽位，主要是应对乱序到达
						uint16_t num_blank_slots = seq - newest_item->sequence_number - 1;

						// 超过大小需要删除旧数据
						if (this->buffer_.size() + num_blank_slots + 1 > this->max_items_) {
								const uint16_t num_items_to_remove = this->buffer_.size()
								                                     + num_blank_slots + 1
								                                     - this->max_items_;

								// 如果待删除数量过大直接清空
								if (num_items_to_remove > this->buffer_.size() - 1) {
										SPDLOG_WARN(
										    "packet has too high seq and forces buffer emptying "
										    "[ssrc:{}, seq:{}, timestamp:{}]",
										    ssrc,
										    seq,
										    timestamp);

										num_blank_slots = 0u;

										Clear();
								} else {
										SPDLOG_DEBUG("calling RemoveOldest(%" PRIu16
										             ") [bufferSize:%zu, numBlankSlots:%" PRIu16
										             ", maxItems:%" PRIu16 "]",
										             num_items_to_remove,
										             this->buffer.size(),
										             numBlankSlots,
										             this->maxItems);

										RemoveOldest(num_items_to_remove);
								}
						}

						// 塞入空白槽
						for (uint16_t i{ 0u }; i < num_blank_slots; ++i) {
								this->buffer_.push_back(nullptr);
						}

						auto* item = new Item(rtp_packet, ssrc, seq, timestamp, 0, 0);

						this->buffer_.push_back(item);
				}
				// 5.每次接到更旧的包，则把它设置为最旧包
				else if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(
				             seq, oldest_item->sequence_number))
				{
						SPDLOG_DEBUG(
						    "packet out of order and older than oldest packet in the buffer "
						    "[seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
						    seq,
						    timestamp);

						// 包太旧，超过时间限制了也不存储
						if (IsTooOldTimestamp(timestamp, newest_item->timestamp)) {
								SPDLOG_WARN("packet's timestamp too old, discarding it [seq:%" PRIu16
								            ", timestamp:%" PRIu32 "]",
								            seq,
								            timestamp);

								return;
						}

						// 确认数据包不低于最旧的包，低于则返回
						if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(
						        timestamp, oldest_item->timestamp))
						{
								SPDLOG_WARN(
								    "packet has lower seq but higher timestamp than oldest "
								    "packet in the buffer,discarding it [ssrc:%" PRIu32
								    ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
								    ssrc,
								    seq,
								    timestamp);

								return;
						}

						// 当旧的数据包到达，计算旧数据的槽位
						const uint16_t num_blank_slots
						    = oldest_item->sequence_number - seq - 1;

						// 确认数据没有超过最大限制
						if (this->buffer_.size() + num_blank_slots + 1 > this->max_items_) {
								SPDLOG_WARN(

								    "discarding received old packet to not exceed max buffer size"
								    " [ssrc:%" PRIu32 ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
								    ssrc,
								    seq,
								    timestamp);

								return;
						}

						// 放到旧数据前面
						for (uint16_t i{ 0u }; i < num_blank_slots; ++i) {
								this->buffer_.push_front(nullptr);
						}

						auto* item = new Item(rtp_packet, ssrc, seq, timestamp, 0, 0);

						this->buffer_.push_front(item);
				}
				// 6.当前后数据都存在，来的数据在中间，则需要先获取数据，再设置
				else
				{
						SPDLOG_DEBUG(
						    "packet out of order and in between oldest and newest packets"
						    " in the buffer [seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
						    seq,
						    timestamp);

						// Let's check if an item already exist in same position. If so,
						// assume it's duplicated.
						auto* item = Get(seq);

						if (item) {
								SPDLOG_DEBUG("packet already in the buffer, discarding [seq:%" PRIu16
								             ", timestamp:%" PRIu32 "]",
								             seq,
								             timestamp);

								return;
						}

						// 获取到buffer的下标
						const uint16_t idx = seq - oldest_item->sequence_number;

						// 根据下标检查是旧数据还是之前设置的空槽
						for (int32_t idx2 = idx - 1; idx2 >= 0; --idx2) {
								const auto* older_item = this->buffer_.at(idx2);

								// 设置的空槽则跳过
								if (!older_item) {
										continue;
								}

								// 存在旧数据
								if (timestamp >= older_item->timestamp) {
										break;
								} else {
										SPDLOG_WARN(
										    "packet timestamp is lower than timestamp of immediate "
										    "older packet in the buffer,discarding it [ssrc:%" PRIu32
										    ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
										    ssrc,
										    seq,
										    timestamp);

										return;
								}
						}

						// 根据下标检查是新数据还是之前设置的空槽
						for (size_t idx2 = idx + 1; idx2 < this->buffer_.size(); ++idx2) {
								const auto* newer_item = this->buffer_.at(idx2);

								// 设置的空槽则跳过
								if (!newer_item) {
										continue;
								}

								// We are done.
								if (timestamp <= newer_item->timestamp) {
										break;
								} else {
										SPDLOG_WARN(

										    "packet timestamp is higher than timestamp of immediate "
										    "newer packet in the buffer,discarding it [ssrc:%" PRIu32
										    ", seq:%" PRIu16 ", timestamp:%" PRIu32 "]",
										    ssrc,
										    seq,
										    timestamp);

										return;
								}
						}

						// 否则直接在槽位存储数据
						item = new Item(rtp_packet, ssrc, seq, timestamp, 0, 0);

						this->buffer_[idx] = item;
				}
		}

		void RtpRetransmissionBuffer::Clear() {
				SPDLOG_TRACE();

				for (auto* item : this->buffer_) {
						if (!item) {
								continue;
						}

						// Reset the stored item (decrease RTP packet shared pointer counter).
						item->Reset();

						delete item;
				}

				this->buffer_.clear();
		}

		RtpRetransmissionBuffer::Item* RtpRetransmissionBuffer::GetOldest() const {
				SPDLOG_TRACE();

				if (this->buffer_.empty()) {
						return nullptr;
				}

				return this->buffer_.front();
		}

		RtpRetransmissionBuffer::Item* RtpRetransmissionBuffer::GetNewest() const {
				SPDLOG_TRACE();

				if (this->buffer_.empty()) {
						return nullptr;
				}

				return this->buffer_.back();
		}

		void RtpRetransmissionBuffer::RemoveOldest() {
				SPDLOG_TRACE();

				if (this->buffer_.empty()) {
						return;
				}

				auto* item = this->buffer_.front();

				// Reset the stored item (decrease RTP packet shared pointer counter).
				item->Reset();

				delete item;

				this->buffer_.pop_front();

				SPDLOG_DEBUG("removed 1 item from the front");

				// Remove all nullptr elements from the beginning of the buffer.
				// NOTE: Calling front on an empty container is undefined.
				size_t num_items_removed{ 0u };

				while (!this->buffer_.empty() && this->buffer_.front() == nullptr) {
						this->buffer_.pop_front();

						++num_items_removed;
				}

				if (num_items_removed) {
						SPDLOG_DEBUG("removed %zu blank slots from the front",
						             numItemsRemoved);
				}
		}

		void RtpRetransmissionBuffer::RemoveOldest(uint16_t numItems) {
				SPDLOG_TRACE();

				const size_t intended_buffer_size = this->buffer_.size() - numItems;

				if (this->buffer_.size() > intended_buffer_size) {
						RemoveOldest();
				}
		}

		bool RtpRetransmissionBuffer::ClearTooOldByTimestamp(uint32_t newestTimestamp) {
				SPDLOG_TRACE();

				RtpRetransmissionBuffer::Item* oldest_item{ nullptr };
				bool items_removed{ false };

				// Go through all buffer items starting with the first and free all
				// items that contain too old packets.
				while ((oldest_item = GetOldest())) {
						if (IsTooOldTimestamp(oldest_item->timestamp, newestTimestamp)) {
								RemoveOldest();

								items_removed = true;
						}
						// If current oldest stored packet is not too old, exit the loop
						// since we know that packets stored after it are guaranteed to be
						// newer.
						else
						{
								break;
						}
				}

				return items_removed;
		}

		bool RtpRetransmissionBuffer::IsTooOldTimestamp(uint32_t timestamp,
		                                                uint32_t newestTimestamp) const {
				SPDLOG_TRACE();

				if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(timestamp, newestTimestamp))
				{
						return false;
				}

				const int64_t diff_ts = newestTimestamp - timestamp;

				return static_cast<uint32_t>(diff_ts * 1000 / this->clock_rate_)
				       > this->max_retransmission_delay_ms_;
		}

		void RtpRetransmissionBuffer::Item::Reset() {
				SPDLOG_TRACE();

				this->rtp_packet.reset();
				this->ssrc            = 0u;
				this->sequence_number = 0u;
				this->timestamp       = 0u;
				this->resent_at_ms    = 0u;
				this->sent_times      = 0u;
		}
} // namespace RTC
