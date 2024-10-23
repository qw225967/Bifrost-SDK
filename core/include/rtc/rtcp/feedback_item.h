/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 16:01
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : feedback_item.h
 * @description : TODO
 *******************************************************/

#ifndef FEEDBACK_ITEM_H
#define FEEDBACK_ITEM_H

#include "rtc/rtc_common.h"

namespace RTC {
		namespace RTCP {
				class FeedbackItem {
				public:
						template<typename Item>
						static std::shared_ptr<Item> Parse(const uint8_t* data, size_t len) {
								// data size must be >= header.
								if (sizeof(typename Item::Header) > len) {
										return nullptr;
								}

								auto* header = const_cast<typename Item::Header*>(
								    reinterpret_cast<const typename Item::Header*>(data));

								return std::make_shared<Item>(header);
						}

				public:
						bool IsCorrect() const {
								return this->is_correct_;
						}

				protected:
						virtual ~FeedbackItem() {
								delete[] this->raw_;
						}

				public:
						virtual void Dump() const = 0;
						virtual void Serialize() {
								delete[] this->raw_;

								this->raw_ = new uint8_t[this->GetSize()];
								this->Serialize(this->raw_);
						}
						virtual size_t Serialize(uint8_t* buffer) = 0;
						virtual size_t GetSize() const            = 0;

				protected:
						uint8_t* raw_{ nullptr };
						bool is_correct_{ true };
				};
		} // namespace RTCP
} // namespace RTC

#endif // FEEDBACK_ITEM_H
