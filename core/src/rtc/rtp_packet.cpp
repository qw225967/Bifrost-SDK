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
#include "utils/utils.h"
namespace RTC {

		std::shared_ptr<RtpPacket> RtpPacket::Parse(const uint8_t* data, size_t len) {
				// SPDLOG_TRACE();

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

				// 2. 解析payload
				size_t csrc_list_size{0u};
				if (this->header_->csrc_count != 0u) {
						csrc_list_size = this->header_->csrc_count * sizeof(this->header_->ssrc);

						// Packet size must be >= header size + CSRC list.
						if (size < (ptr - data_) + csrc_list_size) {
								return;
						}
						ptr += csrc_list_size;
				}

				// Check header extension.
				if (this->header_->extension == 1u) {
						// The header extension is at least 4 bytes.
						if (size < static_cast<size_t>(ptr - data_) + 4) {
								return;
						}

						this->header_extension_ = reinterpret_cast<HeaderExtension*>(ptr);

						// The header extension contains a 16-bit length field that counts the
						// number of 32-bit words in the extension, excluding the four-octet header
						// extension.
						size_t extension_value_size =
						    static_cast<size_t>(this->header_extension_->length);

						// Packet size must be >= header size + CSRC list + header extension size.
						if (size < (ptr - data_) + 4 + extension_value_size) {
								return;
						}
						ptr += 4 + extension_value_size;
				}

				// 3.解析payload
				payload_        = ptr;
				payload_length_ = size - (ptr - data_);

				// 4.padding 解析
				if (this->header_->padding != 0u) {
						// Must be at least a single payload byte.
						if (this->payload_length_ == 0) {
								return;
						}

						this->payload_padding_ = data[size - 1];
						if (this->payload_padding_ == 0) {
								return;
						}

						if (this->payload_length_ < size_t{this->payload_padding_}) {
								return;
						}
						this->payload_length_ -= size_t{this->payload_padding_};
				}
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