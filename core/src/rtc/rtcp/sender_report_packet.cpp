/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 15:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : sender_report_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/sender_report_packet.h"

#include "spdlog/spdlog.h"

namespace RTC {
		namespace RTCP {

				std::shared_ptr<SenderReport> SenderReport::Parse(const uint8_t* data,
				                                                  size_t len) {
						// Get the header.
						auto* header
						    = const_cast<Header*>(reinterpret_cast<const Header*>(data));

						// Packet size must be >= header size.
						if (len < sizeof(Header)) {
								// SPDLOG_ERROR(
								//    "[send report] not enough space for sender report, packet "
								//  "discarded");

								return nullptr;
						}

						return std::make_shared<SenderReport>(header);
				}

				size_t SenderReport::Serialize(uint8_t* buffer) {
						// Copy the header.
						std::memcpy(buffer, this->header_, sizeof(Header));

						return sizeof(Header);
				}

				/* Class methods. */

				std::unique_ptr<SenderReportPacket> SenderReportPacket::Parse(
				    const uint8_t* data, size_t len) {
						// Get the header.
						auto* header = const_cast<CommonHeader*>(
						    reinterpret_cast<const CommonHeader*>(data));

						auto packet   = Cpp11Adaptor::make_unique<SenderReportPacket>(header);
						size_t offset = sizeof(RtcpPacket::CommonHeader);

						if (auto report = SenderReport::Parse(data + offset, len - offset)) {
								packet->AddReport(std::move(report));
						}

						return packet;
				}

				/* Instance methods. */

				size_t SenderReportPacket::Serialize(uint8_t* buffer) {
						size_t offset = RtcpPacket::Serialize(buffer);

						// Serialize reports.
						for (auto report : this->reports_) {
								offset += report->Serialize(buffer + offset);
						}

						return offset;
				}

		} // namespace RTCP
} // namespace RTC