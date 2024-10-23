/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 上午11:08
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/rtp_packet.h"
#include "spdlog/spdlog.h"

namespace RTC {

		std::shared_ptr<RtpPacket> RtpPacket::Parse(const uint8_t* data, size_t len) {
				SPDLOG_TRACE();

				// 检测rtp格式
				if (!RtpPacket::IsRtp(data, len)) {
						return nullptr;
				}

				// 原逻辑应该把payload存储做静态区处理，demo直接memcpy
				// TODO:后续可以优化拷贝逻辑
				return std::make_shared<RtpPacket>(data, len);
		}

		RtpPacket::RtpPacket(const uint8_t* data, size_t size) : size_(size) {
				data_ = new uint8_t[size];
				memcpy(data_, data, size);

				// 头部空间
				// 1.解析头
				auto* ptr = const_cast<uint8_t*>(data_);
				header_   = reinterpret_cast<Header*>(ptr);

				// 偏移头部空间
				ptr += kHeaderSize;

				// 2. csrc和拓展头暂时不用，直接解析payload
				// TODO:补充拓展类的解析

				// 3.解析payload
				payload_        = ptr;
				payload_length_ = size - (ptr - data_);
		}

		RtpPacket::RtpPacket(Header header) {
				data_ = new uint8_t[1400];
				memcpy(data_, &header, kHeaderSize);
				header_   = reinterpret_cast<Header*>(data_);
		}

		RtpPacket::~RtpPacket() {
				delete[] data_;
		}

}