/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 16:10
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : feedback_rtp_item.h
 * @description : TODO
 *******************************************************/

#ifndef FEEDBACK_RTP_ITEM_H
#define FEEDBACK_RTP_ITEM_H
#include <vector>

#include "rtc/rtcp/feedback_packet.h"

namespace RTC {
		namespace RTCP {
				template<typename Item>
				class FeedbackRtpItemsPacket : public FeedbackRtpPacket {
				public:
						using Iterator =
						    typename std::vector<std::shared_ptr<Item>>::iterator;

				public:
						static std::unique_ptr<FeedbackRtpItemsPacket<Item>> Parse(
						    const uint8_t* data, size_t len);

				public:
						// Parsed Report. Points to an external data.
						explicit FeedbackRtpItemsPacket(CommonHeader* common_header)
						    : FeedbackRtpPacket(common_header) {
						}
						explicit FeedbackRtpItemsPacket(uint32_t sender_ssrc,
						                                uint32_t media_ssrc = 0)
						    : FeedbackRtpPacket(Item::message_type_, sender_ssrc, media_ssrc) {
						}
						~FeedbackRtpItemsPacket() {
								for (auto item : this->items_) {
										item.reset();
								}
						}

						void AddItem(std::shared_ptr<Item> item) {
								this->items_.push_back(item);
						}
						Iterator Begin() {
								return this->items_.begin();
						}
						Iterator End() {
								return this->items_.end();
						}

						/* Virtual methods inherited from FeedbackItem. */
				public:
						void Dump() const override;
						size_t Serialize(uint8_t* buffer) override;
						size_t GetSize() const override {
								size_t size = FeedbackRtpPacket::GetSize();

								for (auto item : this->items_) {
										size += item->GetSize();
								}

								return size;
						}

				private:
						std::vector<std::shared_ptr<Item>> items_;
				};
		} // namespace RTCP
} // namespace RTC

#endif // FEEDBACK_RTP_ITEM_H
