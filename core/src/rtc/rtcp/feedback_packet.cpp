/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-17 15:28
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : feedback_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtcp/feedback_packet.h"

#include <cstring>

#include "spdlog/spdlog.h"
#include "rtc/rtcp/nack_packet.h"

namespace RTC {
		namespace RTCP {
				/* Class methods. */

				template<typename T>
				const std::string& FeedbackPacket<T>::MessageType2String(
				    typename T::MessageType type) {
						static const std::string Unknown("UNKNOWN");

						auto it = type_to_string_.find(type);

						if (it == type_to_string_.end()) {
								return Unknown;
						}

						return it->second;
				}

				template<typename T>
				FeedbackPacket<T>::FeedbackPacket(CommonHeader* common_header)
				    : RtcpPacket(common_header),
				      message_type_(typename T::MessageType(common_header->count)) {
						this->header_ = reinterpret_cast<Header*>(
						    reinterpret_cast<uint8_t*>(common_header) + sizeof(CommonHeader));
				}

				template<typename T>
				FeedbackPacket<T>::FeedbackPacket(typename T::MessageType message_type,
				                                  uint32_t sender_ssrc,
				                                  uint32_t media_ssrc)
				    : RtcpPacket(rtcp_type_), message_type_(message_type) {
						this->raw_                 = new uint8_t[sizeof(Header)];
						this->header_              = reinterpret_cast<Header*>(this->raw_);
						this->header_->sender_ssrc = uint32_t{ htonl(sender_ssrc) };
						this->header_->media_ssrc  = uint32_t{ htonl(media_ssrc) };
				}

				template<typename T>
				FeedbackPacket<T>::~FeedbackPacket() {
						delete[] this->raw_;
				}

				template<typename T>
				size_t FeedbackPacket<T>::Serialize(uint8_t* buffer) {
						size_t offset = RtcpPacket::Serialize(buffer);

						// Copy the header.
						std::memcpy(buffer + offset, this->header_, sizeof(Header));

						return offset + sizeof(Header);
				}

				template<>
				Type FeedbackPacket<FeedbackPs>::rtcp_type_ = Type::PSFB;

				// clang-format off
template<>
std::map<FeedbackPs::MessageType, std::string> FeedbackPacket<FeedbackPs>::type_to_string_ =
    {
    { FeedbackPs::MessageType::PLI,   "PLI"   },
    { FeedbackPs::MessageType::SLI,   "SLI"   },
    { FeedbackPs::MessageType::RPSI,  "RPSI"  },
    { FeedbackPs::MessageType::FIR,   "FIR"   },
    { FeedbackPs::MessageType::TSTR,  "TSTR"  },
    { FeedbackPs::MessageType::TSTN,  "TSTN"  },
    { FeedbackPs::MessageType::VBCM,  "VBCM"  },
    { FeedbackPs::MessageType::PSLEI, "PSLEI" },
    { FeedbackPs::MessageType::ROI,   "ROI"   },
    { FeedbackPs::MessageType::AFB,   "AFB"   },
    { FeedbackPs::MessageType::EXT,   "EXT"   }
    };
				// clang-format on

				template<>
				std::shared_ptr<FeedbackPacket<FeedbackPs>> FeedbackPacket<FeedbackPs>::Parse(
				    const uint8_t* data, size_t len) {
						if (len < sizeof(CommonHeader) + sizeof(FeedbackPacket::Header)) {
								SPDLOG_ERROR(
								    "[feedback packet] not enough space for Feedback packet, "
								    "discarded");

								return nullptr;
						}

						auto* common_header = const_cast<CommonHeader*>(
						    reinterpret_cast<const CommonHeader*>(data));

						switch (FeedbackPs::MessageType(common_header->count)) {
						case FeedbackPs::MessageType::PLI:
								break;

						case FeedbackPs::MessageType::SLI:
								break;

						case FeedbackPs::MessageType::RPSI:
								break;

						case FeedbackPs::MessageType::FIR:
								break;

						case FeedbackPs::MessageType::TSTR:
								break;

						case FeedbackPs::MessageType::TSTN:
								break;

						case FeedbackPs::MessageType::VBCM:
								break;

						case FeedbackPs::MessageType::PSLEI:
								break;

						case FeedbackPs::MessageType::ROI:
								break;

						case FeedbackPs::MessageType::AFB:
								break;

						case FeedbackPs::MessageType::EXT:
								break;

						default:
								SPDLOG_ERROR(
								    "[feedback packet] unknown RTCP PS Feedback message type "
								    "[packetType: {}]",
								    static_cast<uint8_t>(common_header->count));
						}

						return nullptr;
				}

				/* Specialization for Rtcp class. */

				template<>
				Type FeedbackPacket<FeedbackRtp>::rtcp_type_ = Type::RTPFB;

				// clang-format off
template<>
std::map<FeedbackRtp::MessageType, std::string> FeedbackPacket<FeedbackRtp>::type_to_string_ =
    {
    { FeedbackRtp::MessageType::NACK,   "NACK"   },
    { FeedbackRtp::MessageType::TMMBR,  "TMMBR"  },
    { FeedbackRtp::MessageType::TMMBN,  "TMMBN"  },
    { FeedbackRtp::MessageType::SR_REQ, "SR_REQ" },
    { FeedbackRtp::MessageType::RAMS,   "RAMS"   },
    { FeedbackRtp::MessageType::TLLEI,  "TLLEI"  },
    { FeedbackRtp::MessageType::ECN,    "ECN"    },
    { FeedbackRtp::MessageType::PS,     "PS"     },
    { FeedbackRtp::MessageType::EXT,    "EXT"    },
    { FeedbackRtp::MessageType::TCC,    "TCC"    },
    { FeedbackRtp::MessageType::QUICFB, "QUICFB" }
    };
				// clang-format on

				/* Class methods. */

				template<>
				std::shared_ptr<FeedbackPacket<FeedbackRtp>> FeedbackPacket<FeedbackRtp>::Parse(
				    const uint8_t* data, size_t len) {
						if (len < sizeof(CommonHeader) + sizeof(Header)) {
								SPDLOG_ERROR(
								    "[feedback packet] not enough space for Feedback packet, "
								    "discarded");

								return nullptr;
						}

						auto* common_header
						    = reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(data));
						std::shared_ptr<FeedbackRtpPacket> packet{ nullptr };

						switch (static_cast<FeedbackRtp::MessageType>(common_header->count))
						{
						case FeedbackRtp::MessageType::NACK:
								packet = FeedbackRtpNackPacket::Parse(data, len);
								break;

						case FeedbackRtp::MessageType::TMMBR:
								break;

						case FeedbackRtp::MessageType::TMMBN:
								break;

						case FeedbackRtp::MessageType::SR_REQ:
								break;

						case FeedbackRtp::MessageType::RAMS:
								break;

						case FeedbackRtp::MessageType::TLLEI:
								break;

						case FeedbackRtp::MessageType::ECN:
								break;

						case FeedbackRtp::MessageType::PS:
								break;

						case FeedbackRtp::MessageType::EXT:
								break;

						case FeedbackRtp::MessageType::TCC:
								// packet = FeedbackRtpTransportPacket::Parse(data, len);
								break;

						default:
								SPDLOG_ERROR(
								    "[feedback packet] unknown RTCP RTP Feedback message type "
								    "[packetType: {}]",
								    static_cast<uint8_t>(common_header->count));
						}

						return std::move(packet);
				}

				// Explicit instantiation to have all FeedbackPacket definitions in this file.
				template class FeedbackPacket<FeedbackPs>;
				template class FeedbackPacket<FeedbackRtp>;
		} // namespace RTCP
} // namespace RTC