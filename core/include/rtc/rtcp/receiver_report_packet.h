/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 14:36
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : receiver_report_packet.h
 * @description : TODO
 *******************************************************/

/*
 *        0                   1                   2                   3
 *        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * header |V=2|P|    RC   |   PT=RR=201   |             length            |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                     SSRC of packet sender                     |
 *        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * report |                 SSRC_1 (SSRC of first source)                 |
 * block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   1    | fraction lost |       cumulative number of packets lost       |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |           extended highest sequence number received           |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                      interarrival jitter                      |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                         last SR (LSR)                         |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                   delay since last SR (DLSR)                  |
 *        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * report |                 SSRC_2 (SSRC of second source)                |
 * block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   2    :                               ...                             :
 *        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *        |                  profile-specific extensions                  |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#ifndef RECEIVER_REPORT_PACKET_H
#define RECEIVER_REPORT_PACKET_H

#include "rtcp_packet.h"
#include "utils/utils.h"
#include <vector>

namespace RTC {
		namespace RTCP {
				class ReceiverReport {
				public:
						// 头信息
						struct Header {
								uint32_t ssrc;
								uint32_t fractionLost : 8;
								uint32_t totalLost : 24;
								uint32_t lastSeq;
								uint32_t jitter;
								uint32_t lsr;
								uint32_t dlsr;
						};

				public:
						static std::shared_ptr<ReceiverReport> Parse(const uint8_t* data,
						                                             size_t len);

				public:
						// Locally generated Report. Holds the data internally.
						ReceiverReport() {
								this->header_ = reinterpret_cast<Header*>(this->raw_);
						}
						// Parsed Report. Points to an external data.
						explicit ReceiverReport(Header* header) : header_(header) {
						}
						explicit ReceiverReport(ReceiverReport* report)
						    : header_(report->header_) {
						}

						void Dump() const;
						size_t Serialize(uint8_t* buffer);
						size_t GetSize() const {
								return sizeof(Header);
						}
						uint32_t GetSsrc() const {
								return uint32_t{ ntohl(this->header_->ssrc) };
						}
						void SetSsrc(uint32_t ssrc) {
								this->header_->ssrc = uint32_t{ htonl(ssrc) };
						}
						uint8_t GetFractionLost() const {
								return uint8_t{ RTCUtils::Byte::Get1Byte(
										(uint8_t*)this->header_, 4) };
						}
						void SetFractionLost(uint8_t fraction_lost) {
								RTCUtils::Byte::Set1Byte((uint8_t*)this->header_, 4,
								                         fraction_lost);
						}
						int32_t GetTotalLost() const {
								auto value = uint32_t{ RTCUtils::Byte::Get3Bytes(
										(uint8_t*)this->header_, 5) };

								// Possitive value.
								if (((value >> 23) & 1) == 0) {
										return value;
								}

								// Negative value.
								if (value != 0x0800000) {
										value &= ~(1 << 23);
								}

								return -value;
						}
						void SetTotalLost(int32_t total_lost) {
								// Get the limit value for possitive and negative totalLost.
								int32_t clamp
								    = (total_lost >= 0)
								          ? total_lost > 0x07FFFFF ? 0x07FFFFF : total_lost
								      : -total_lost > 0x0800000 ? 0x0800000
								                                : -total_lost;

								uint32_t value = (total_lost >= 0) ? (clamp & 0x07FFFFF)
								                                   : (clamp | 0x0800000);

								RTCUtils::Byte::Set3Bytes(
								    reinterpret_cast<uint8_t*>(this->header_), 5, value);
						}
						uint32_t GetLastSeq() const {
								return uint32_t{ ntohl(this->header_->lastSeq) };
						}
						void SetLastSeq(uint32_t lastSeq) {
								this->header_->lastSeq = uint32_t{ htonl(lastSeq) };
						}
						uint32_t GetJitter() const {
								return uint32_t{ ntohl(this->header_->jitter) };
						}
						void SetJitter(uint32_t jitter) {
								this->header_->jitter = uint32_t{ htonl(jitter) };
						}
						uint32_t GetLastSenderReport() const {
								return uint32_t{ ntohl(this->header_->lsr) };
						}
						void SetLastSenderReport(uint32_t lsr) {
								this->header_->lsr = uint32_t{ htonl(lsr) };
						}
						uint32_t GetDelaySinceLastSenderReport() const {
								return uint32_t{ ntohl(this->header_->dlsr) };
						}
						void SetDelaySinceLastSenderReport(uint32_t dlsr) {
								this->header_->dlsr = uint32_t{ htonl(dlsr) };
						}

				private:
						Header* header_{ nullptr };
						uint8_t raw_[sizeof(Header)]{ 0u };
				};

				class ReceiverReportPacket : public RtcpPacket {
				public:
						using Iterator
						    = std::vector<std::shared_ptr<ReceiverReport>>::iterator;

				public:
						static std::unique_ptr<ReceiverReportPacket> Parse(
						    const uint8_t* data, size_t len, size_t offset = 0);

				public:
						ReceiverReportPacket() : RtcpPacket(Type::RR) {
						}
						explicit ReceiverReportPacket(CommonHeader* common_header)
						    : RtcpPacket(common_header) {
						}
						~ReceiverReportPacket() override {
								for (auto report : this->reports_) {
										report.reset();
								}
						}

						uint32_t GetSsrc() const {
								return this->ssrc_;
						}
						void SetSsrc(uint32_t ssrc) {
								this->ssrc_ = ssrc;
						}
						void AddReport(std::shared_ptr<ReceiverReport> report) {
								this->reports_.push_back(report);
						}
						void RemoveReport(std::shared_ptr<ReceiverReport> report) {
								auto it = std::find(this->reports_.begin(),
								                    this->reports_.end(), report);

								if (it != this->reports_.end()) {
										this->reports_.erase(it);
								}
						}
						Iterator Begin() {
								return this->reports_.begin();
						}
						Iterator End() {
								return this->reports_.end();
						}

						/* Pure virtual methods inherited from Packet. */
				public:
						void Dump() const override {
						}
						size_t Serialize(uint8_t* buffer) override;
						// NOTE: We need to force this since when we parse a
						// SenderReportPacket that contains receive report blocks we also
						// generate a second ReceiverReportPacket from same data and len, so
						// parent Packet::GetType() would return this->type which would be
						// SR instead of RR.
						Type GetType() const override {
								return Type::RR;
						}
						size_t GetCount() const override {
								return this->reports_.size();
						}
						size_t GetSize() const override {
								size_t size = sizeof(CommonHeader) + 4u /* this->ssrc */;

								for (auto report : reports_) {
										size += report->GetSize();
								}

								return size;
						}

				private:
						// SSRC of packet sender.
						uint32_t ssrc_{ 0u };
						std::vector<std::shared_ptr<ReceiverReport>> reports_;
				};
		} // namespace RTCP
} // namespace RTC

#endif // RECEIVER_REPORT_PACKET_H
