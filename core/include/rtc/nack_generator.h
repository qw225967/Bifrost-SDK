/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午2:34
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : nack_generator.h
 * @description : TODO
 *******************************************************/

#ifndef NACK_GENERATOR_H
#define NACK_GENERATOR_H

#include "rtc_common.h"
#include "rtc/rtp_packet.h"
#include "rtc/seq_manager.h"
#include "utils/time_handler.h"
#include <map>
#include <set>
#include <vector>

namespace RTC {
		class NackGenerator final : public RTCUtils::TimerHandle::Listener {
		public:
				class Listener {
				public:
						virtual ~Listener() = default;

				public:
						virtual void OnNackGeneratorNackRequired(
						    const std::vector<uint16_t>& seq_numbers)
						    = 0;
						virtual void OnNackGeneratorKeyFrameRequired() = 0;
				};

		private:
				struct NackInfo {
						NackInfo() = default;
						explicit NackInfo(uint64_t created_at_ms, uint16_t seq,
						                  uint16_t send_at_seq)
						    : created_at_ms_(created_at_ms), seq_(seq),
						      send_at_seq_(send_at_seq) {
						}

						uint64_t created_at_ms_{ 0u };
						uint16_t seq_{ 0u };
						uint16_t send_at_seq_{ 0u };
						uint64_t sent_at_ms_{ 0u };
						uint8_t retries_{ 0u };
				};

				enum class NackFilter { SEQ, TIME };

		public:
				explicit NackGenerator(Listener* listener,
				                       const std::shared_ptr<CoreIO::NetworkThread>& thread,
				                       unsigned int send_nack_delay_ms);
				~NackGenerator() override;

				bool ReceivePacket(const RtpPacketPtr& rtp_packet, bool is_recovered);
				[[nodiscard]] size_t GetNackListLength() const {
						return this->nack_list_.size();
				}
				void UpdateRtt(uint32_t rtt) {
						this->rtt_ = rtt;
				}
				void Reset();

		private:
				void AddPacketsToNackList(uint16_t seq_start, uint16_t seq_end);
				bool RemoveNackItemsUntilKeyFrame();
				std::vector<uint16_t> GetNackBatch(NackFilter filter);
				void MayRunTimer() const;

				/* Pure virtual methods inherited from TimerHandle::Listener. */
		public:
				void OnTimer(RTCUtils::TimerHandle* timer) override;

		private:
				// Passed by argument.
				Listener* listener_{ nullptr };
				unsigned int send_nack_delay_ms_{ 0u };
				// Allocated by this.
				RTCUtils::TimerHandle* timer_{ nullptr };
				// Others.
				std::map<uint16_t, NackInfo, RTC::SeqManager<uint16_t>::SeqLowerThan> nack_list_;
				std::set<uint16_t, RTC::SeqManager<uint16_t>::SeqLowerThan> key_frame_list_;
				std::set<uint16_t, RTC::SeqManager<uint16_t>::SeqLowerThan> recovered_list_;
				bool started_{ false };
				uint16_t last_seq_{ 0u }; // Seq number of last valid packet.
				uint32_t rtt_{ 0u };      // Round trip time (ms).
		};
} // namespace RTC

#endif // NACK_GENERATOR_H
