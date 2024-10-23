/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 16:12
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : feedback_rtp_item.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/feedback_rtp_item.h"
#include "rtc/rtcp/feedback_item.h"
#include "rtc/rtcp/nack_packet.h"

#include "spdlog/spdlog.h"

namespace RTC {
		namespace RTCP {
				/* Class methods. */

				template<typename Item>
				std::unique_ptr<FeedbackRtpItemsPacket<Item>> FeedbackRtpItemsPacket<
				    Item>::Parse(const uint8_t* data, size_t len) {
						if (len < sizeof(CommonHeader) + sizeof(Header)) {
								// SPDLOG_ERROR(
								//   "[feedback rtp items] not enough space for Feedback packet, "
								//  "discarded");

								return nullptr;
						}

						// NOLINTNEXTLINE(llvm-qualified-auto)
						auto* commonHeader = const_cast<CommonHeader*>(
						    reinterpret_cast<const CommonHeader*>(data));

						auto packet
						    = Cpp11Adaptor::make_unique<FeedbackRtpItemsPacket<Item>>(commonHeader);

						size_t offset = sizeof(CommonHeader) + sizeof(Header);

						while (len > offset) {
								if (auto item
								    = FeedbackItem::Parse<Item>(data + offset, len - offset))
								{
										packet->AddItem(item);
										offset += item->GetSize();
								} else {
										break;
								}
						}

						return std::move(packet);
				}

				/* Instance methods. */

				template<typename Item>
				size_t FeedbackRtpItemsPacket<Item>::Serialize(uint8_t* buffer) {
						size_t offset = FeedbackPacket::Serialize(buffer);

						for (auto item : this->items_) {
								offset += item->Serialize(buffer + offset);
						}

						return offset;
				}

				template<typename Item>
				void FeedbackRtpItemsPacket<Item>::Dump() const {
				}

				// Explicit instantiation to have all FeedbackRtpPacket definitions in
				// this file.
				template class FeedbackRtpItemsPacket<FeedbackRtpNackItem>;
		} // namespace RTCP
} // namespace RTC