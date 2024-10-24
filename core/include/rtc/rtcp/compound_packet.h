/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-23 下午5:01
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : compound_packet.h
 * @description : TODO
 *******************************************************/

#ifndef COMPOUND_PACKET_H
#define COMPOUND_PACKET_H

#include "rtc/rtc_common.h"
#include "rtc/rtcp/receiver_report_packet.h"
#include "rtc/rtcp/sender_report_packet.h"
#include "rtc/rtp_packet.h" // MtuSize.
#include <vector>

namespace RTC {
		namespace RTCP {
				class CompoundPacket {
				public:
						// Maximum size for a CompundPacket.
						// * IPv4|Ipv6 header size: 20|40 bytes. IPv6 considered.
						// * UDP|TCP header size:   8|20  bytes. TCP considered.
						// * SRTP Encryption: 148 bytes.
						//    SRTP_MAX_TRAILER_LEN+4 is the maximum number of octects that
						//    will be added to an RTCP packet by srtp_protect_rtcp(). srtp.h:
						//    SRTP_MAX_TRAILER_LEN (SRTP_MAX_TAG_LEN + SRTP_MAX_MKI_LEN)
						constexpr static size_t kMaxSize{ RTC::kMtuSize - 40u - 20u - 148u };

				public:
						CompoundPacket() = default;

				public:
						[[nodiscard]] const uint8_t* GetData() const {
								return this->header_;
						}
						size_t GetSize();
						[[nodiscard]] size_t GetSenderReportCount() const {
								return this->sender_report_packet_.GetCount();
						}
						[[nodiscard]] size_t GetReceiverReportCount() const {
								return this->receiver_report_packet_.GetCount();
						}
						bool AddSenderReport(const std::shared_ptr<SenderReport>& report);
						bool AddReceiverReport(const std::shared_ptr<ReceiverReport>& report);
						bool HasSenderReport() {
								return this->sender_report_packet_.Begin()
								       != this->sender_report_packet_.End();
						}
						void Serialize(uint8_t* data);

				private:
						uint8_t* header_{ nullptr };
						SenderReportPacket sender_report_packet_;
						ReceiverReportPacket receiver_report_packet_;
				};
		} // namespace RTCP
} // namespace RTC

#endif // COMPOUND_PACKET_H
