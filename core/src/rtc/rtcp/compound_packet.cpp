/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-23 下午5:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : compound_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/compound_packet.h"
#include "spdlog/spdlog.h"

namespace RTC {
		namespace RTCP {
				/* Instance methods. */

				size_t CompoundPacket::GetSize() {
						size_t size{ 0 };

						if (this->sender_report_packet_.GetCount() > 0u) {
								size += this->sender_report_packet_.GetSize();
						}

						if (this->receiver_report_packet_.GetCount() > 0u) {
								size += this->receiver_report_packet_.GetSize();
						}

						return size;
				}

				void CompoundPacket::Serialize(uint8_t* data) {
						// SPDLOG_TRACE();

						this->header_ = data;

						// Fill it.
						size_t offset{ 0 };

						if (this->sender_report_packet_.GetCount() > 0u) {
								offset += this->sender_report_packet_.Serialize(this->header_);
						}

						if (this->receiver_report_packet_.GetCount() > 0u) {
								offset += this->receiver_report_packet_.Serialize(this->header_
								                                                  + offset);
						}
				}

				bool CompoundPacket::AddSenderReport(
				    const std::shared_ptr<SenderReport>& report) {
						// SPDLOG_TRACE();

						this->sender_report_packet_.AddReport(report);

						if (GetSize() <= kMaxSize) {
								return true;
						}

						this->sender_report_packet_.RemoveReport(report);

						return false;
				}

				bool CompoundPacket::AddReceiverReport(
				    const std::shared_ptr<ReceiverReport>& report) {
						// SPDLOG_TRACE();

						this->receiver_report_packet_.AddReport(report);

						if (GetSize() <= kMaxSize) {
								return true;
						}

						this->receiver_report_packet_.RemoveReport(report);

						return false;
				}
		} // namespace RTCP
} // namespace RTC