/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-28 下午4:32
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : tcp_buffer.h
 * @description : TODO
 *******************************************************/

#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>

namespace CoreIO {
		class Buffer {
		public:
				static const size_t kCheapPrepend = 8;
				static const size_t kInitialSize  = 65536 - kCheapPrepend;

				explicit Buffer(size_t initialSize = kInitialSize)
				    : buffer_(kCheapPrepend + initialSize),
				      reader_index_(kCheapPrepend), writer_index_(kCheapPrepend) {
				}

				Buffer(const Buffer&)            = delete;
				Buffer& operator=(const Buffer&) = delete;

				size_t readableBytes() const {
						return writer_index_ - reader_index_;
				}

				size_t writableBytes() const {
						return buffer_.size() - writer_index_;
				}

				const uint8_t* peek() const {
						return begin() + reader_index_;
				}

				void retrieve(size_t len) {
						assert(len <= readableBytes());
						if (len < readableBytes()) {
								reader_index_ += len;
						} else {
								retrieveAll();
						}
				}

				void retrieveAll() {
						reader_index_ = kCheapPrepend;
						writer_index_ = kCheapPrepend;
				}

				void append(const char* /*restrict*/ data, size_t len) {
						ensureWritableBytes(len);
						std::copy(data, data + len, beginWrite());
						hasWritten(len);
				}

				void append(const void* /*restrict*/ data, size_t len) {
						append(static_cast<const char*>(data), len);
				}

				void hasWritten(size_t len) {
						assert(len <= writableBytes());
						writer_index_ += len;
				}

				uint16_t peekUint16() const {
						assert(readableBytes() >= sizeof(uint16_t));
						uint8_t data[2];
						::memcpy(&data, peek(), sizeof data);

						return uint16_t{ data[1] } | uint16_t{ data[0] } << 8;
				}

				size_t prependableBytes() const {
						return reader_index_;
				}

		private:
				void ensureWritableBytes(size_t len) {
						if (writableBytes() < len) {
								makeSpace(len);
						}

						assert(writableBytes() >= len);
				}

				uint8_t* beginWrite() {
						return begin() + writer_index_;
				}

				const uint8_t* beginWrite() const {
						return begin() + writer_index_;
				}

				uint8_t* begin() {
						return &*buffer_.begin();
				}

				const uint8_t* begin() const {
						return &*buffer_.begin();
				}

				void makeSpace(size_t len) {
						if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
								// FIXME: move readable data
								buffer_.resize(writer_index_ + len);
						} else {
								// move readable data to the front, make space inside buffer
								assert(kCheapPrepend < reader_index_);
								size_t readable = readableBytes();
								std::copy(begin() + reader_index_, begin() + writer_index_,
								          begin() + kCheapPrepend);
								reader_index_ = kCheapPrepend;
								writer_index_ = reader_index_ + readable;
								assert(readable == readableBytes());
						}
				}

		private:
				std::vector<uint8_t> buffer_;
				size_t reader_index_;
				size_t writer_index_;
		};
} // namespace CoreIO
#endif // TCP_BUFFER_H
