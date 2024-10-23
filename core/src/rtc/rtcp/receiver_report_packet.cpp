/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 14:39
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : receiver_report_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/receiver_report_packet.h"

#include "spdlog/spdlog.h"

namespace RTC {
		namespace RTCP {
				std::shared_ptr<ReceiverReport> ReceiverReport::Parse(const uint8_t* data,
				                                                      size_t len) {
						// 获取头部信息
						auto* header
						    = const_cast<Header*>(reinterpret_cast<const Header*>(data));

						// 包大小必须大于头部大小
						if (len < sizeof(Header)) {
								SPDLOG_ERROR(
								    "[receive report] not enough space for receiver report, "
								    "packet discarded");

								return nullptr;
						}

						return std::make_shared<ReceiverReport>(header);
				}

				size_t ReceiverReport::Serialize(uint8_t* buffer) {
						// Copy the header.
						std::memcpy(buffer, this->header_, sizeof(Header));

						return sizeof(Header);
				}

				/**
				 * ReceiverReportPacket::Parse()
				 * @param  data   - 整个包的开始指针
				 * @param  len    - 整个包的大小
				 * @param  offset - 单个receiver report的偏移值
				 */
				std::unique_ptr<ReceiverReportPacket> ReceiverReportPacket::Parse(
				    const uint8_t* data, size_t len, size_t offset) {
						// Get the header.
						auto* header = const_cast<CommonHeader*>(
						    reinterpret_cast<const CommonHeader*>(data));

						// 除了有通用头外，receiver report还会带有自己的ssrc，确认不存在直接返回
						if (len < sizeof(CommonHeader) + 4u /* ssrc */) {
								SPDLOG_ERROR(
								    "[receive report] not enough space for receiver report "
								    "packet, packet discarded");

								return nullptr;
						}

						std::unique_ptr<ReceiverReportPacket> packet
						    = std::make_unique<ReceiverReportPacket>(header);

						uint32_t ssrc = RTCUtils::Byte::Get4Bytes(
						    reinterpret_cast<uint8_t*>(header), sizeof(CommonHeader));

						packet->SetSsrc(ssrc);

						if (offset == 0) {
								offset = sizeof(CommonHeader) + 4u /* ssrc */;
						}

						uint8_t count = header->count;

						while ((count-- != 0u) && (len > offset)) {
								if (auto report
								    = ReceiverReport::Parse(data + offset, len - offset);
								    report != nullptr)
								{
										packet->AddReport(std::move(report));
										offset += report->GetSize();
								} else {
										return packet;
								}
						}

						return std::move(packet);
				}

				size_t ReceiverReportPacket::Serialize(uint8_t* buffer) {
						size_t offset = RtcpPacket::Serialize(buffer);

						// 拷贝ssrc
						RTCUtils::Byte::Set4Bytes(buffer, sizeof(CommonHeader), this->ssrc_);
						offset += 4u;

						// 序列化所有组合的report
						for (auto report : this->reports_) {
        offset += report->Serialize(buffer + offset);
    }

    return offset;
}
}  // namespace RTCP
}  // namespace RTC