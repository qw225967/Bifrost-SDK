/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 15:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : sender_report_packet.h
 * @description : TODO
 *******************************************************/

/*
 *        0                   1                   2                   3
 *        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * header |V=2|P|    RC   |   PT=SR=200   |             length            |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                         SSRC of sender                        |
 *        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * sender |              NTP timestamp, most significant word             |
 * info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |             NTP timestamp, least significant word             |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                         RTP timestamp                         |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                     sender's packet count                     |
 *        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *        |                      sender's octet count                     |
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
 *
 */

#ifndef SENDER_REPORT_PACKET_H
#define SENDER_REPORT_PACKET_H

#include <vector>

#include "rtcp_packet.h"
#include "utils/utils.h"

namespace RTC {
		namespace RTCP {
				class SenderReport {
				public:
						// 头信息
						struct Header {
								uint32_t ssrc;
								uint32_t ntpSec;
								uint32_t ntpFrac;
								uint32_t rtpTs;
								uint32_t packetCount;
								uint32_t octetCount;
						};

				public:
						static std::shared_ptr<SenderReport> Parse(const uint8_t* data,
						                                           size_t len);

				public:
						// Locally generated Report. Holds the data internally.
						SenderReport() {
								this->header_ = reinterpret_cast<Header*>(this->raw_);
						}
						// Parsed Report. Points to an external data.
						explicit SenderReport(Header* header) : header_(header) {
						}
						explicit SenderReport(std::shared_ptr<SenderReport> report)
						    : header_(report->header_) {
						}

						void Dump() const;

						[[nodiscard]] size_t GetSize() const {
								return sizeof(Header);
						}
						[[nodiscard]] uint32_t GetSsrc() const {
								return uint32_t{ ntohl(this->header_->ssrc) };
						}

						[[nodiscard]] uint32_t GetNtpSec() const {
								return uint32_t{ ntohl(this->header_->ntpSec) };
						}

						[[nodiscard]] uint32_t GetNtpFrac() const {
								return uint32_t{ ntohl(this->header_->ntpFrac) };
						}

						[[nodiscard]] uint32_t GetRtpTs() const {
								return uint32_t{ ntohl(this->header_->rtpTs) };
						}

						[[nodiscard]] uint32_t GetPacketCount() const {
								return uint32_t{ ntohl(this->header_->packetCount) };
						}

						[[nodiscard]] uint32_t GetOctetCount() const {
								return uint32_t{ ntohl(this->header_->octetCount) };
						}

						size_t Serialize(uint8_t* buffer);

						void SetSsrc(uint32_t ssrc) const {
								this->header_->ssrc = uint32_t{ htonl(ssrc) };
						}

						void SetNtpSec(uint32_t ntp_sec) {
								this->header_->ntpSec = uint32_t{ htonl(ntp_sec) };
						}

						void SetNtpFrac(uint32_t ntpFrac) {
								this->header_->ntpFrac = uint32_t{ htonl(ntpFrac) };
						}

						void SetRtpTs(uint32_t rtpTs) {
								this->header_->rtpTs = uint32_t{ htonl(rtpTs) };
						}

						void SetPacketCount(uint32_t packetCount) {
								this->header_->packetCount = uint32_t{ htonl(packetCount) };
						}

						void SetOctetCount(uint32_t octetCount) {
								this->header_->octetCount = uint32_t{ htonl(octetCount) };
						}

				private:
						Header* header_{ nullptr };
						uint8_t raw_[sizeof(Header)]{ 0 };
				};

				class SenderReportPacket : public RtcpPacket {
				public:
						using Iterator = std::vector<std::shared_ptr<SenderReport>>::iterator;

				public:
						static std::unique_ptr<SenderReportPacket> Parse(const uint8_t* data,
						                                                 size_t len);

				public:
						SenderReportPacket() : RtcpPacket(Type::SR) {
						}
						explicit SenderReportPacket(CommonHeader* common_header)
						    : RtcpPacket(common_header) {
						}
						~SenderReportPacket() override {
								for (auto report : this->reports_) {
										report.reset();
								}
						}

						void AddReport(std::shared_ptr<SenderReport> report) {
								this->reports_.push_back(report);
						}

						void RemoveReport(std::shared_ptr<SenderReport> report) {
								auto it = std::find(
								    this->reports_.begin(), this->reports_.end(), report);

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
						size_t GetCount() const override {
								return this->reports_.size();
						}
						size_t GetSize() const override {
								size_t size = sizeof(CommonHeader);

								for (const auto& report : this->reports_) {
										size += report->GetSize();
								}

								return size;
						}

				private:
						std::vector<std::shared_ptr<SenderReport>> reports_;
				};
		} // namespace RTCP
} // namespace RTC

#endif // SENDER_REPORT_PACKET_H
