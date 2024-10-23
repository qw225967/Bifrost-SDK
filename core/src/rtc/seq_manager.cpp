/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午2:45
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : seq_manager.cpp
 * @description : TODO
 *******************************************************/

#include "rtc/seq_manager.h"
#include "spdlog/spdlog.h"
#include <iterator>

namespace RTC {
		template<typename T, uint8_t N>
		bool SeqManager<T, N>::SeqLowerThan::operator()(T lhs, T rhs) const {
				return ((rhs > lhs) && (rhs - lhs <= MaxValue / 2))
				       || ((lhs > rhs) && (lhs - rhs > MaxValue / 2));
		}

		template<typename T, uint8_t N>
		bool SeqManager<T, N>::SeqHigherThan::operator()(T lhs, T rhs) const {
				return ((lhs > rhs) && (lhs - rhs <= MaxValue / 2))
				       || ((rhs > lhs) && (rhs - lhs > MaxValue / 2));
		}

		template<typename T, uint8_t N>
		const typename SeqManager<T, N>::SeqLowerThan SeqManager<T, N>::isSeqLowerThan{};

		template<typename T, uint8_t N>
		const typename SeqManager<T, N>::SeqHigherThan SeqManager<T, N>::isSeqHigherThan{};

		template<typename T, uint8_t N>
		bool SeqManager<T, N>::IsSeqLowerThan(T lhs, T rhs) {
				return isSeqLowerThan(lhs, rhs);
		}

		template<typename T, uint8_t N>
		bool SeqManager<T, N>::IsSeqHigherThan(T lhs, T rhs) {
				return isSeqHigherThan(lhs, rhs);
		}

		template<typename T, uint8_t N>
		SeqManager<T, N>::SeqManager(T initialOutput)
		    : initial_output_(initialOutput) {
				// SPDLOG_TRACE("SeqManager constructor");
		}

		template<typename T, uint8_t N>
		void SeqManager<T, N>::Sync(T input) {
				// SPDLOG_TRACE();

				// Update base.
				this->base_ = (this->max_output_ - input) & MaxValue;

				// Update maxInput.
				this->max_input_ = input;

				// Clear dropped set.
				this->dropped_.clear();
		}

		template<typename T, uint8_t N>
		void SeqManager<T, N>::Drop(T input) {
				// SPDLOG_TRACE();

				// Mark as dropped if 'input' is higher than anyone already processed.
				if (SeqManager<T, N>::IsSeqHigherThan(input, this->max_input_)) {
						this->max_input_ = input;
						// Insert input in the last position.
						// Explicitly indicate insert() to add the input at the end, which
						// is more performant.
						this->dropped_.insert(this->dropped_.end(), input);

						ClearDropped();
				}
		}

		template<typename T, uint8_t N>
		bool SeqManager<T, N>::Input(T input, T& output) {
				// SPDLOG_TRACE();

				auto base = this->base_;

				// No dropped inputs to consider.
				if (this->dropped_.empty()) {
						goto done;
				}
				// Dropped inputs present, cleanup and update base.
				else
				{
						// Set 'maxInput' here if needed before calling ClearDropped().
						if (this->started_ && IsSeqHigherThan(input, this->max_input_)) {
								this->max_input_ = input;
						}

						ClearDropped();

						base = this->base_;
				}

				// No dropped inputs to consider after cleanup.
				if (this->dropped_.empty()) {
						goto done;
				}
				// This input was dropped.
				else if (this->dropped_.find(input) != this->dropped_.end())
				{
						// SPDLOG_DEBUG("trying to send a dropped input");

						return false;
				}
				// There are dropped inputs, calculate 'base' for this input.
				else
				{
						auto droppedCount = this->dropped_.size();

						// Get the first dropped input which is higher than or equal 'input'.
						auto it = this->dropped_.lower_bound(input);

						droppedCount -= std::distance(it, this->dropped_.end());
						base = (this->base_ - droppedCount) & MaxValue;
				}

		done:
				output = (input + base) & MaxValue;

				if (!this->started_) {
						this->started_    = true;
						this->max_input_  = input;
						this->max_output_ = output;
				} else {
						// New input is higher than the maximum seen.
						if (IsSeqHigherThan(input, this->max_input_)) {
								this->max_input_ = input;
						}

						// New output is higher than the maximum seen.
						if (IsSeqHigherThan(output, this->max_output_)) {
								this->max_output_ = output;
						}
				}

				output = (output + this->initial_output_) & MaxValue;

				return true;
		}

		template<typename T, uint8_t N>
		T SeqManager<T, N>::GetMaxInput() const {
				return this->max_input_;
		}

		template<typename T, uint8_t N>
		T SeqManager<T, N>::GetMaxOutput() const {
				return this->max_output_;
		}

		/*
		 * Delete droped inputs greater than maxInput, which belong to a previous
		 * cycle.
		 */
		template<typename T, uint8_t N>
		void SeqManager<T, N>::ClearDropped() {
				// SPDLOG_TRACE();

				// Cleanup dropped values.
				if (this->dropped_.empty()) {
						return;
				}

				const size_t previousDroppedSize = this->dropped_.size();

				for (auto it = this->dropped_.begin(); it != this->dropped_.end();) {
						auto value = *it;

						if (isSeqHigherThan(value, this->max_input_)) {
								it = this->dropped_.erase(it);
						} else {
								break;
						}
				}

				// Adapt base.
				this->base_
				    = (this->base_ - (previousDroppedSize - this->dropped_.size()))
				      & MaxValue;
		}

		// Explicit instantiation to have all SeqManager definitions in this file.
		template class SeqManager<uint8_t>;
		template class SeqManager<uint8_t, 3>; // For testing.
		template class SeqManager<uint16_t>;
		template class SeqManager<uint16_t, 15>; // For PictureID (15 bits).
		template class SeqManager<uint32_t>;

} // namespace RTC