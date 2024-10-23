/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 13:53
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtcp_packet.h
 * @description : 所有rtcp类包的公共l类
 *******************************************************/

#ifndef RTCP_PACKET_H
#define RTCP_PACKET_H

#include <map>

#include "rtc/rtc_common.h"
#define MS_LITTLE_ENDIAN 1
// #define MS_BIG_ENDIAN 1
namespace RTC {
		namespace RTCP {
				// 音频、视频 RTCP 发送间隔
				constexpr uint16_t MaxAudioIntervalMs{ 5000 };
				constexpr uint16_t MaxVideoIntervalMs{ 1000 };

				// RTCP类型标识
				enum class Type : uint8_t {
						SR    = 200,
						RR    = 201,
						SDES  = 202,
						BYE   = 203,
						APP   = 204,
						RTPFB = 205,
						PSFB  = 206,
						XR    = 207,
						NAT   = 211
				};

				class RtcpPacket {
				public:
						/* Struct for RTCP common header. */
						struct CommonHeader {
#if defined(MS_LITTLE_ENDIAN)
								uint8_t count : 5;
								uint8_t padding : 1;
								uint8_t version : 2;
#elif defined(MS_BIG_ENDIAN)
								uint8_t version : 2;
								uint8_t padding : 1;
								uint8_t count : 5;
#endif
								uint8_t packet_type : 8;
								uint16_t length : 16;
						};

				public:
						static bool IsRtcp(const uint8_t* data, size_t len) {
								auto header = const_cast<CommonHeader*>(
								    reinterpret_cast<const CommonHeader*>(data));

								// clang-format off
        return (
            (len >= sizeof(CommonHeader)) &&
            // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
            (data[0] > 127 && data[0] < 192) &&
            // RTP Version must be 2.
            (header->version == 2) &&
            // RTCP packet types defined by IANA:
            // http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
            // RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
            (header->packet_type >= 192 && header->packet_type <= 223)
            );
								// clang-format on
						}
						static std::shared_ptr<RtcpPacket> Parse(const uint8_t* data,
						                                         size_t len);
						static const std::string& Type2String(Type type);

				private:
						static std::map<Type, std::string> type_to_string_;

				public:
						explicit RtcpPacket(Type type) : type_(type) {
						}
						explicit RtcpPacket(CommonHeader* common_header) {
								this->type_   = Type(common_header->packet_type);
								this->header_ = common_header;
						}
						virtual ~RtcpPacket() = default;

						// 获取包数据指针
						const uint8_t* GetData() const {
								return reinterpret_cast<uint8_t*>(this->header_);
						}

				public:
						/*
						 * 数据可视化
						 */
						virtual void Dump() const = 0;

						/*
						 * 发送包前，必须调用序列化数据。
						 * 不同子类型的rtcp包有自有格式，必须继承并封装序列化方式
						 */
						virtual size_t Serialize(uint8_t* buffer) = 0;
						virtual Type GetType() const {
								return this->type_;
						}
						virtual size_t GetCount() const {
								return 0u;
						}
						virtual size_t GetSize() const = 0;

				private:
    Type type_;
    CommonHeader* header_{nullptr};

};

	typedef std::shared_ptr<RtcpPacket> RtcpPacketPtr;
}
}


#endif //RTCP_PACKET_H
