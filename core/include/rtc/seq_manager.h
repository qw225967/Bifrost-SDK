/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午2:45
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : seq_manager.h
 * @description : TODO
 *******************************************************/

#ifndef SEQ_MANAGER_H
#define SEQ_MANAGER_H

#include "rtc_common.h"
#include <limits> // std::numeric_limits
#include <set>

namespace RTC {
		// T is the base type (uint16_t, uint32_t, ...).
		// N is the max number of bits used in T.
		template<typename T, uint8_t N = 0>
		class SeqManager {
		public:
				static constexpr T MaxValue
				    = (N == 0) ? std::numeric_limits<T>::max() : ((1 << N) - 1);

		public:
				struct SeqLowerThan {
						bool operator()(T lhs, T rhs) const;
				};

				struct SeqHigherThan {
						bool operator()(T lhs, T rhs) const;
				};

		public:
				static bool IsSeqLowerThan(T lhs, T rhs);
				static bool IsSeqHigherThan(T lhs, T rhs);

		private:
				static const SeqLowerThan isSeqLowerThan; // NOLINT(readability-identifier-naming)
				static const SeqHigherThan isSeqHigherThan; // NOLINT(readability-identifier-naming)

		public:
				SeqManager() = default;
				explicit SeqManager(T initialOutput);

		public:
				void Sync(T input);
				void Drop(T input);
				bool Input(T input, T& output);
				T GetMaxInput() const;
				T GetMaxOutput() const;

		private:
				void ClearDropped();

		private:
				// Whether at least a sequence number has been inserted.
				bool started_{ false };
				T initial_output_{ 0 };
				T base_{ 0 };
				T max_output_{ 0 };
				T max_input_{ 0 };
				std::set<T, SeqLowerThan> dropped_;
		};
} // namespace RTC

#endif // SEQ_MANAGER_H
