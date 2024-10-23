/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 14:17
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtcp_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/rtcp_packet.h"

#include <iostream>

// #include "rtcp_feedback.h"
#include "rtc/rtcp/feedback_packet.h"
#include "rtc/rtcp/receiver_report_packet.h"
#include "rtc/rtcp/sender_report_packet.h"

#include "spdlog/spdlog.h"
#include "utils/utils.h"

namespace RTC {
		namespace RTCP {
				// clang-format off
std::map<Type, std::string> RtcpPacket::type_to_string_ =
    {
    { Type::SR,    "SR"    },
    { Type::RR,    "RR"    },
    { Type::SDES,  "SDES"  },
    { Type::BYE,   "BYE"   },
    { Type::APP,   "APP"   },
    { Type::RTPFB, "RTPFB" },
    { Type::PSFB,  "PSFB"  },
    { Type::XR,    "XR"    },
    { Type::NAT,   "NAT"   }
    };
				// clang-format on

				std::shared_ptr<RtcpPacket> RtcpPacket::Parse(const uint8_t* data,
				                                              size_t len) {
						// 注意:仅解析组合包中的最后一个包.
						std::shared_ptr<RtcpPacket> current{ nullptr };

						// 长度不对直接返回
						while (len > 0u) {
								// 判断是否为rtcp包
								if (!IsRtcp(data, len)) {
										SPDLOG_ERROR(
										    "[rtcp packet] data is not a RTCP packet no rtcp:%s",
										    RTCUtils::Byte::BytesToHex(data, len));
										return nullptr;
								}

								// 解析rtcp通用头部
								auto* header = const_cast<CommonHeader*>(
								    reinterpret_cast<const CommonHeader*>(data));
								size_t packet_len
								    = static_cast<size_t>(ntohs(header->length) + 1) * 4;

								// 长度低于包解析记录的长则为包异常
								if (len < packet_len) {
										SPDLOG_ERROR(
										    "[rtcp packet] packet length exceeds remaining data [len:%d, "
										    "packet len:%d]",
										    len,
										    packet_len);

										return nullptr;
								}

								// 根据rtcp通用头识别具体rtcp类型
								switch (Type(header->packet_type)) {
								case Type::SR:
								{
										current = SenderReportPacket::Parse(data, packet_len);
										break;
								}

								case Type::RR:
								{
										current = ReceiverReportPacket::Parse(data, packet_len);
										break;
								}

								case Type::SDES:
								{
										break;
								}

								case Type::BYE:
								{
										break;
								}

								case Type::NAT:
								{
										break;
								}

								case Type::APP:
								{
										break;
								}

								case Type::RTPFB:
								{
										current = FeedbackRtpPacket::Parse(data, packet_len);
										break;
								}

								case Type::PSFB:
								{
										break;
								}

								case Type::XR:
								{
										break;
								}

								default:
								{
										SPDLOG_ERROR(
										    "[rtcp packet] unknown RTCP packet type [packetType:{}]",
										    static_cast<uint8_t>(header->packet_type));
								}
								}

								if (!current) {
										std::string packet_type
										    = Type2String(Type(header->packet_type));
										SPDLOG_ERROR("[rtcp packet] error parsing {} RtcpPacket err!",
										             packet_type.c_str());

										return nullptr;
								}

								// 包可能是组合包，因此需要多次循环
								data += packet_len;
								len -= packet_len;
						}

						return std::move(current);
				}

				const std::string& RtcpPacket::Type2String(Type type) {
						static const std::string Unknown("UNKNOWN");

						auto it = type_to_string_.find(type);

						if (it == type_to_string_.end()) {
								return Unknown;
						}

						return it->second;
				}

				// 发送时，序列化数据
				size_t RtcpPacket::Serialize(uint8_t* buffer) {
						this->header_ = reinterpret_cast<CommonHeader*>(buffer);

						size_t length = (GetSize() / 4) - 1;

						// 写入包头
						this->header_->version     = 2;
						this->header_->padding     = 0;
						this->header_->count       = static_cast<uint8_t>(GetCount());
    this->header_->packet_type = static_cast<uint8_t>(GetType());
    this->header_->length = uint16_t{htons(length)};

    return sizeof(CommonHeader);
}
}
}