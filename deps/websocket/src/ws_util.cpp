//
// Created by xuhuahai on 2017/4/24.
//

#include "ws_util.h"

#include <assert.h>

#include "spdlog/spdlog.h"
#include "ws_encode.h"
#include "ws_sha1.h"

static uchar mask[4] = { 0x1a, 0x2b, 0x3c, 0x4d };

/**
 * 对输入先进行SHA1，再进行Base64处理
 * @param input   输入内容的首指针
 * @param n       输入内容的长度
 * @note 返回的内容，需要调用free()进行内存回收
 * @return  处理完得到的字符串
 */
char* sha1_and_base64(char* input, size_t n) {
		SHA1_CONTEXT hd;
		hash1_reset(&hd);
		hash1_write(&hd, (uchar*)input, n);
		hash1_final(&hd);
		char* p = base64_encode(hd.buf, strlen((char*)hd.buf));
		return p;
}

/**
 * 产生websocket客户端的Handshake请求报文内容
 * @param ip     WS服务器IP
 * @param port   WS服务器端口
 * @param path   请求URI路径
 * @param key    Sec-WebSocket-Key的值
 * @note  返回的内容需要使用free()进行内容回收
 * @return  报文内容
 */
char* generate_websocket_client_handshake(const char* ip, ushort port,
                                          const char* path, const char* key) {
		char* buf = static_cast<char*>(malloc(HANDSHAKE_SIZE));
		memset(buf, 0, HANDSHAKE_SIZE);

		snprintf(buf, HANDSHAKE_SIZE,
		         "GET %s HTTP/1.1\r\n"
		         "Host: %s:%d\r\n"
		         "Upgrade: websocket\r\n"
		         "Connection: Upgrade\r\n"
		         "Sec-WebSocket-Key: %s\r\n"
		         "Sec-WebSocket-Version: 13\r\n"
		         "\r\n",
		         path, ip, port, key);

		return buf;
}

/**
 * 产生websocket服务器端的Handshake应答报文内容
 * @param key    Sec-WebSocket-Key的值
 * @note  返回的内容需要使用free()进行内容回收
 * @return  报文内容
 */
char* generate_websocket_server_handshake(const char* key) {
		char akey[100] = { 0 };

		strcpy(akey, key);
		strncpy(akey + strlen(key), WS_KEY_SUFFFIX, strlen(WS_KEY_SUFFFIX));

		char* p = sha1_and_base64(akey, strlen(akey));

		char* buf = static_cast<char*>(malloc(HANDSHAKE_SIZE));
		memset(buf, 0, HANDSHAKE_SIZE);

		snprintf(buf, HANDSHAKE_SIZE,
		         "HTTP/1.1 101 Switching Protocols\r\n"
		         "Upgrade: websocket\r\n"
		         "Connection: Upgrade\r\n"
		         "Sec-WebSocket-Accept: %s\r\n"
		         "\r\n",
		         p);
		free(p);

		return buf;
}

/**
 * 对数据进行掩码计算
 * @param data
 * @param len
 * @param mask
 */
void do_mask(uchar* data, size_t len, uchar* mask) {
		size_t i = 0;
		for (i = 0; i < len; i++) {
				data[i] = data[i] ^ mask[i % 4];
		}
}

/**
 * 从缓冲区或数据中读取字节，
 * 如果能从缓冲区得到，则先从缓冲区读取，缓冲区如果不够，则从数据区读，
 * 如果最后也没有读到，则返回0
 * @param buf        缓冲区
 * @param buf_read   缓冲区已读取数量
 * @param data       数据区指针
 * @param data_read  数据区已读取数量
 * @param data_len    数据区总长度
 * @param readlen    需要读取的字节个数
 * @param readchar
 * 读取得到的字节保存区，如果该参数提供NULL，则表示不存储数据，其他逻辑依旧需要处理
 * @param real_read_num_from_data 实际上从数据区读取的数量
 * @return  如果返回1，表示读到了字节，如果返回0，表示没有读完数据
 */
static int read_char_from_buf_or_data(membuf_t* buf, size_t* buf_read, char* data,
                                      size_t* data_read, size_t data_len,
                                      size_t readlen, char* readchar,
                                      size_t* real_read_num_from_data) {
		if (*buf_read + readlen <= buf->size) { /// 缓冲区有足够数据可读
				if (readchar) {
						memcpy(readchar, buf->data + *buf_read, readlen);
				}
				*buf_read += readlen;
				return 1;
		}
		size_t buf_available = buf->size - *buf_read; /// 缓冲区部分可读的个数
		if (buf_available > 0) {
				if (readchar) {
						memcpy(readchar, buf->data + *buf_read, buf_available);
				}
				*buf_read += buf_available;
		}
		if (*data_read + readlen - buf_available <= data_len)
		{ /// 剩下的部分从数据区可全部读出来
				if (readchar) {
						memcpy(readchar + buf_available, data + *data_read,
						       readlen - buf_available);
				}
				// 数据同步拷贝到buf，并且修改缓冲区的已读取数量
				membuf_append_data(buf, data + *data_read, readlen - buf_available);
				*data_read += readlen - buf_available;
				*buf_read += readlen - buf_available;
				*real_read_num_from_data += readlen - buf_available;
				return 1;
		}
		// 此次只能读取部分数据了
		size_t data_available = data_len - *data_read;
		if (data_available > 0) {
				if (readchar) {
						memcpy(readchar + buf_available, data + *data_read, data_available);
				}
				// 数据同步拷贝到buf，并且修改缓冲区的已读取数量
				membuf_append_data(buf, data + *data_read, data_available);
				*data_read += data_available;
				*buf_read += data_available;
				*real_read_num_from_data += data_available;
		}
		return 0;
};

/**
 * 尝试解析一个完整报文，会从缓冲和数据区混合读取数据，这样就统一了读取源的模型，极大简化了逻辑复杂度，
 * 每一次都是从协议头开始读取，这样做可以大幅度简化逻辑。
 * 由于实际上多次读取才能读完整的情况并不常见，所以对性能影响可以忽略，
 * @param handle
 * @param data
 * @param data_len
 * @return
 * 返回1表示解析出了一个完整报文，返回0表示协议解析不完整(可能可以读出0或n个字节)
 */
int try_parse_protocol(websocket_handle* handle, char* data, size_t data_len) {
		assert(handle);
		handle->read_count = 0; /// 将本次的数据区读取计数清空
		if (data_len == 0 || !data) {
				return 0;
		}
		if (handle->parse_state == PARSE_COMPLETE) {
				membuf_clear(&handle->proto_buf, 0); /// 清理原来的缓冲数据
				handle->mask[0] = handle->mask[1] = handle->mask[2] = handle->mask[3] = 0;
		}
		if (data_len > 0) { /// 如果有数据可读，则表示进入未完成状态
				handle->parse_state = PARSE_INCOMPLETE;
		}
		membuf_t* buf     = &handle->proto_buf;
		size_t buf_read   = 0; /// buf的读取数量
		size_t data_read  = 0; /// data的读取数量
		size_t header_len = 0; /// 协议头的长度

		// 处理粘包，当小包数据在头时，小包包括：ping、pong、close
		uchar type = data[0] & 0x0F;
		if (type > 3 & data_len > 2) {
				// 移除ping数据
				printf("1111");
//				data+=2;
//				data_len-=2;
		}

		// 解析首字节
		char temp;
		read_char_from_buf_or_data(buf, &buf_read, data, &data_read, data_len,
		                           sizeof(char), &temp, &handle->read_count);
		handle->is_eof = (char)(temp >> 7); // FIN
		handle->df_ext = (temp & 0x70);     // RSV1,RSV2,RSV3
		handle->type   = temp & 0xF;        // OPCode
		// 解析第二个字节
		if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read, data_len,
		                                sizeof(char), &temp, &handle->read_count))
		{
				return 0;
		}
		handle->has_mask    = (char)(temp >> 7); // MASK
		handle->payload_len = temp & 0x7f;       // Payload len

		header_len += 2;
		// 解析后续字节
		if (handle->payload_len < 126) {
				handle->real_payload_len = handle->payload_len;
				if (handle->real_payload_len == 0) { // 解析到一个最简单的2字节报文
						handle->parse_state = PARSE_COMPLETE;
						return 1;
				}
				if (handle->has_mask) {
						// read mask
						if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read,
						                                data_len, sizeof(handle->mask),
						                                (char*)&handle->mask[0],
						                                &handle->read_count))
						{
								return 0;
						}
						header_len += 4;
				}
		} else if (handle->payload_len == 126) {
				// read extend payload length
				char exten_len[2];
				if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read,
				                                data_len, sizeof(exten_len),
				                                &exten_len[0], &handle->read_count))
				{
						return 0;
				}
				handle->real_payload_len = read_two_byte_length((uchar*)exten_len);
				header_len += 2;
				if (handle->has_mask) {
						// read mask
						if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read,
						                                data_len, sizeof(handle->mask),
						                                (char*)&handle->mask[0],
						                                &handle->read_count))
						{
								return 0;
						}
						header_len += 4;
				}
		} else if (handle->payload_len == 127) {
				// read extend payload length
				char exten_len[4];
				if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read,
				                                data_len, sizeof(exten_len),
				                                &exten_len[0], &handle->read_count))
				{
						return 0;
				}
				if (!(exten_len[0] == 0 && exten_len[1] == 0 && exten_len[2] == 0
				      && exten_len[3] == 0))
				{
						// LOG_ERROR("parse error : found wrong extension length field");
						handle->parse_state = PARSE_FAILED;
						return 0;
				}
				if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read,
				                                data_len, sizeof(exten_len),
				                                &exten_len[0], &handle->read_count))
				{
						return 0;
				}
				handle->real_payload_len = read_four_byte_length((uchar*)exten_len);
				header_len += 8;
				if (handle->has_mask) {
						// read mask
						if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read,
						                                data_len, sizeof(handle->mask),
						                                (char*)&handle->mask[0],
						                                &handle->read_count))
						{
								return 0;
						}
						header_len += 4;
				}
		} else {
				// LOG_ERROR("parse error : found wrong paylod length field");
				handle->parse_state = PARSE_FAILED;
				return 0;
		}
		// read payload
		if (!read_char_from_buf_or_data(buf, &buf_read, data, &data_read, data_len,
		                                handle->real_payload_len, NULL,
		                                &handle->read_count))
		{
				return 0;
		}
		if (handle->has_mask) {
				do_mask(buf->data + header_len, buf->size - header_len, handle->mask);
		}

		// 全部解析完成，则重置解析状态
		handle->parse_state = PARSE_COMPLETE;
		// 去掉头部数据
		membuf_remove(buf, 0, header_len);
		return 1;
}

/**
 * 产生一个websocket帧
 * @param buf     用来存放帧的缓冲区
 * @param data    实际数据的首指针
 * @param data_len 实际数据的长度
 * @param op      控制码/帧类型
 *                 %x0 denotes a continuation frame
 *                 %x1 denotes a text frame
 *                 %x2 denotes a binary frame
 *                 %x3-7 are reserved for further non-control frames
 *                 %x8 denotes a connection close
 *                 %x9 denotes a ping
 *                 %xA denotes a pong
 *                 %xB-F are reserved for further control frames
 * @param fin     是否结束帧
 * @param enable_mask 是否使用掩码
 */
void generate_websocket_frame_without_fragment(membuf_t* buf, const void* data,
                                               size_t data_len, uchar op,
                                               uchar fin, uchar enable_mask) {
		uchar header[14]; /// 定义一个数组用于保存协议头
		memset(header, 0, 14);
		size_t header_size = 0;
		uchar payload_len  = 0L;
		if (data_len <= 125) {
				// 第一byte,10000000, fin = 1, rsv1 rsv2 rsv3均为0, opcode =
				// 0x01,即数据为文本帧
				header[0] |= (uchar)(fin << 7);  // FIN
				header[0] |= (uchar)(op & 0x0F); // opcode
				payload_len = (uchar)data_len;
				header[1] |= (uchar)(enable_mask << 7);   // mask flag
				header[1] |= (uchar)(payload_len & 0x7F); // payload len
				header_size += 2;
				if (enable_mask) {
						header[2] = mask[0];
						header[3] = mask[1];
						header[4] = mask[2];
						header[5] = mask[3];
						header_size += 4;
				}
		} else if (data_len <= 65535) {
				// 第一byte,10000000, fin = 1, rsv1 rsv2 rsv3均为0, opcode =
				// 0x01,即数据为文本帧
				header[0] |= (uchar)(fin << 7);  // FIN
				header[0] |= (uchar)(op & 0x0F); // opcode
				payload_len = 0x7E;
				header[1] |= (uchar)(enable_mask << 7);   // mask flag
				header[1] |= (uchar)(payload_len & 0x7F); // payload len
				header_size += 2;
				write_two_byte_length(data_len, &header[2]);
				header_size += 2;
				if (enable_mask) {
						header[4] = mask[0];
						header[5] = mask[1];
						header[6] = mask[2];
						header[7] = mask[3];
						header_size += 4;
				}
		} else if (data_len <= 0xFFFFFFFFul) {
				// 第一byte,10000000, fin = 1, rsv1 rsv2 rsv3均为0, opcode =
				// 0x01,即数据为文本帧
				header[0] |= (uchar)(fin << 7);  // FIN
				header[0] |= (uchar)(op & 0x0F); // opcode
				payload_len = 0x7F;
				header[1] |= (uchar)(enable_mask << 7);   // mask flag
				header[1] |= (uchar)(payload_len & 0x7F); // payload len
				header_size += 2;
				// 数据长度,前4字节留空,保存32位数据大小
				header[2] = 0;
				header[3] = 0;
				header[4] = 0;
				header[5] = 0;
				write_four_byte_length(data_len, &header[6]);
				header_size += 8;
				if (enable_mask) {
						header[10] = mask[0];
						header[11] = mask[1];
						header[12] = mask[2];
						header[13] = mask[3];
						header_size += 4;
				}
		} else {
				printf("Data to send is too big! Max size is 0xFFFFFFFF.\n");
				return;
		}
		membuf_append_data(buf, header, header_size); /// 加头
		membuf_append_data(buf, data, data_len);      /// 加体
		if (enable_mask) {
				do_mask(buf->data + buf->size - data_len, data_len, mask);
		}
		// LOG_DEBUG("generate one datapacket whthout fragment , size =
		// %u",data_len);
}

/**
 * 产生一个close控制帧
 * @return
 */
uchar* generate_close_control_frame() {
		static uchar buf[2] = { (uchar)(1 << 7) | (uchar)(WS_CLOSE_FRAME & 0x0F), 2 };
		return buf;
}

/**
 * 产生一个ping控制帧
 * @return
 */
uchar* generate_ping_control_frame() {
		static uchar buf[2];
		buf[0] |= (uchar)(1 << 7);               // FIN
		buf[0] |= (uchar)(WS_PING_FRAME & 0x0F); // opcode
		buf[1] |= (uchar)(0 << 7);               // mask flag
		buf[1] |= (uchar)(0 & 0x7F);             // payload len
		return buf;
}

/**
 * 产生一个pong控制帧
 * @return
 */
uchar* generate_pong_control_frame() {
		static uchar buf[6];
		buf[0] |= (uchar)(1 << 7);               // FIN
		buf[0] |= (uchar)(WS_PONG_FRAME & 0x0F); // opcode
		buf[1] |= (uchar)(1 << 7);               // mask flag
		buf[1] |= (uchar)(0 & 0x7F);             // payload len

		// 设置mask
		buf[2] = mask[0];
		buf[3] = mask[1];
		buf[4] = mask[2];
		buf[5] = mask[3];
		return buf;
}

/**
 * 产生websocket帧，支持fragment
 *     A fragmented message consists of a single frame with the FIN bit
 *     clear and an opcode other than 0, followed by zero or more frames
 *     with the FIN bit clear and the opcode set to 0, and terminated by
 *     a single frame with the FIN bit set and an opcode of 0.
 * @param buf      用来存放帧的缓冲区
 * @param data     实际数据的首指针
 * @param data_len  实际数据的长度
 * @param op       控制码/帧类型
 */
void generate_websocket_frame(membuf_t *buf, const void * data, size_t data_len, uchar op){
    if(data_len == 0){
        return;
    }
    // LOG_DEBUG("generate_websocket_frame , input len = %u",data_len);
    //开始分割数据包
    size_t have_wrapped_count = 0L;
    uchar * data_ptr = (uchar*)data;
    size_t a = (size_t)(data_len/DATA_FRAME_MAX_LEN);
    size_t b = (data_len % DATA_FRAME_MAX_LEN == 0)?0:1;
    size_t frag_num = a + b;
    // LOG_DEBUG("fragment count = %u",fragNum);
    uint counter = 0;
    do{
        size_t num = (data_len-have_wrapped_count>DATA_FRAME_MAX_LEN) ?
        					DATA_FRAME_MAX_LEN : (data_len-have_wrapped_count);
        if(frag_num == 1){
            generate_websocket_frame_without_fragment(buf, data_ptr, num, op,
            	FIN_ENABLE, 1);
        }else{
            if(counter == 0){ //first
                generate_websocket_frame_without_fragment(buf, data_ptr, num, op,
                	FIN_DISABLE, 1);
            }else if(counter == frag_num-1){ //last
                generate_websocket_frame_without_fragment(buf, data_ptr, num,
                	WS_CONTINUE_FRAME, FIN_ENABLE, 1);
            }else{
                generate_websocket_frame_without_fragment(buf, data_ptr, num,
                	WS_CONTINUE_FRAME, FIN_DISABLE, 1);
            }
        }
        data_ptr += num;
        have_wrapped_count += num;
        counter++;
    }while(have_wrapped_count<data_len && counter<frag_num);
    // LOG_DEBUG("generate_websocket_frame , output len = %u",buf->size);
}

