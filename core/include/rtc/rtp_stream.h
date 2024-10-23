/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-18 下午6:08
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtp_stream.h
 * @description : TODO
 *******************************************************/

#ifndef RTP_STREAM_H
#define RTP_STREAM_H
#include "io/network_thread.h"
#include "rtcp/receiver_report_packet.h"
#include "rtcp/sender_report_packet.h"
#include "rtp_packet.h"
#include "seq_manager.h"

namespace RTC {
		class RtpStream {
		public:
		protected:
				class Listener {
				public:
						virtual ~Listener() = default;

				public:
						virtual void OnRtpStreamScore(RtpStream* rtp_stream,
						                              uint8_t score,
						                              uint8_t previous_score)
						    = 0;
				};

		public:
				struct Params {
						Params(const uint32_t ssrc, const uint8_t payload_type,
						       const uint32_t clock_rate, const bool use_nack)
						    : ssrc(ssrc), payload_type(payload_type),
						      clock_rate(clock_rate), use_nack(use_nack) {
						}
						uint32_t ssrc{ 0u };
						uint8_t payload_type{ 0u };
						uint32_t clock_rate{ 0u };
						bool use_nack{ false };
				};

		public:
				explicit RtpStream(Listener* listener, Params& params,
				                   const std::shared_ptr<CoreIO::NetworkThread>& thread);
				virtual ~RtpStream();

		public:
				// 获取ssrc
				[[nodiscard]] uint32_t GetSsrc() const {
						return params_.ssrc;
				}
				[[nodiscard]] uint8_t GetPayloadType() const {
						return params_.payload_type;
				}

				bool ReceiveStreamPacket(RtpPacketPtr& rtp_packet);

		protected:
				// 更新每次序号，验证序号正确性
				bool UpdateSequence(RtpPacketPtr& rtp_packet);
				void InitSequence(uint16_t seq);
				void PacketRetransmitted(RtpPacketPtr& rtp_packet);

		protected:
				// 回调函数
				RTC::RtpStream::Listener* listener_{ nullptr };
				// 参数
				Params params_;
				// 网络线程
				std::shared_ptr<CoreIO::NetworkThread> thread_;

				// 当前最新序号
				uint16_t max_seq_{ 0u };
				// 序号循环次数
				uint32_t cycles_{ 0u };
				// 对比的基础序号
				uint32_t base_seq_{ 0u };
				// 最新出错序号+1
				uint32_t bad_seq_{ 0u };
				// 最大包时间戳
				uint32_t max_packet_ts_{ 0u };
				// 最新包接收时间
				uint64_t max_packet_ms_{ 0u };
				// 包距离记录
				size_t packets_discarded_{ 0u };
				// 对应链路的rtt
				float rtt_{ 0.0f };

		private:
				bool started_{ false };
	  };
}


#endif //RTP_STREAM_H