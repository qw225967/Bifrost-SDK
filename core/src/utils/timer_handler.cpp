/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午2:37
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : timer_handler.cpp
 * @description : TODO
 *******************************************************/

#include <utility>

#include "spdlog/spdlog.h"
#include "utils/time_handler.h"

namespace RTCUtils {
		inline static void OnTimer(const uv_timer_t* handle) {
				if (auto* timer = static_cast<TimerHandle*>(handle->data)) {
						timer->OnUvTimer();
				}
		}

		inline static void OnCloseTimer(uv_handle_t* handle) {
				delete reinterpret_cast<uv_timer_t*>(handle);
		}

		TimerHandle::TimerHandle(TimerHandle::Listener* listener,
		                         std::shared_ptr<CoreIO::NetworkThread> thread)
		    : listener_(listener), thread_(std::move(thread)) {
				// SPDLOG_TRACE();
		}

		TimerHandle::~TimerHandle() {
				if (!is_close_.load()) {
						CloseInvoke();
				}
		}

		bool TimerHandle::Init() {
				// SPDLOG_TRACE();

				uv_handle_       = new uv_timer_t;
				uv_handle_->data = static_cast<void*>(this);
				is_close_.store(false);

				int err = uv_timer_init(thread_->GetLoop(), uv_handle_);
				if (err != 0) {
						delete uv_handle_;
						uv_handle_ = nullptr;
						// SPDLOG_INFO("uv_timer_init() failed: {}", uv_strerror(err));
						return false;
				}

				return true;
		}

		bool TimerHandle::InitInvoke() {
				// SPDLOG_TRACE();

				return thread_->Post<bool>([&]() -> bool { return Init(); }).get();
		}

		bool TimerHandle::Start(uint64_t timeout, uint64_t repeat) {
				// SPDLOG_TRACE();

				if (is_close_.load()) {
						// SPDLOG_INFO("timer handle closed");
						return false;
				}

				timeout_ = timeout;
				repeat_  = repeat;

				if (uv_is_active(reinterpret_cast<uv_handle_t*>(uv_handle_)) != 0) {
						Stop();
				}

				int err = uv_timer_start(
				    uv_handle_, reinterpret_cast<uv_timer_cb>(OnTimer), timeout, repeat);
				if (err != 0) {
						// SPDLOG_ERROR("uv_timer_start() failed: {}", uv_strerror(err));
						return false;
				}

				return true;
		}

		bool TimerHandle::StartInvoke(uint64_t timeout, uint64_t repeat) {
				// SPDLOG_TRACE();

				return thread_
				    ->Post<bool>([&, timeout, repeat]() -> bool {
						    return Start(timeout, repeat);
				    })
				    .get();
		}

		void TimerHandle::Stop() {
				// SPDLOG_TRACE();

				if (is_close_.load()) {
						// SPDLOG_ERROR("timer handle closed");
				}

				int err = uv_timer_stop(uv_handle_);
				if (err != 0) {
						// SPDLOG_ERROR("uv_timer_stop() failed: {}", uv_strerror(err));
				}
		}

		void TimerHandle::StopInvoke() {
				// SPDLOG_TRACE();

				thread_->Post<void>([&]() { Stop(); }).get();
		}

		void TimerHandle::Reset() {
				// SPDLOG_TRACE();

				if (is_close_.load()) {
						// SPDLOG_ERROR("timer handle closed");
				}

				if (uv_is_active(reinterpret_cast<uv_handle_t*>(uv_handle_)) == 0) {
						return;
				}

				if (repeat_ == 0) {
						return;
				}

				int err = uv_timer_start(
				    uv_handle_, reinterpret_cast<uv_timer_cb>(OnTimer), repeat_, repeat_);
				if (err != 0) {
						// SPDLOG_ERROR("uv_timer_start() failed: {}", uv_strerror(err));
				}
		}

		void TimerHandle::ResetInvoke() {
				// SPDLOG_TRACE();

				thread_->Post<void>([&]() { Reset(); }).get();
		}

		void TimerHandle::Restart() {
				// SPDLOG_TRACE();

				if (is_close_.load()) {
						// SPDLOG_ERROR("timer handle closed");
				}

				if (uv_is_active(reinterpret_cast<uv_handle_t*>(uv_handle_)) != 0) {
						Stop();
				}

				int err
				    = uv_timer_start(uv_handle_, reinterpret_cast<uv_timer_cb>(OnTimer),
				                     timeout_, repeat_);
				if (err != 0) {
						// SPDLOG_ERROR("uv_timer_start() failed: {}", uv_strerror(err));
				}
		}

		void TimerHandle::RestartInvoke() {
				// SPDLOG_TRACE();

				thread_->Post<void>([&]() { Restart(); }).get();
		}

		bool TimerHandle::IsActive() const {
				// SPDLOG_TRACE();

				return uv_is_active(reinterpret_cast<uv_handle_t*>(uv_handle_)) != 0;
		}

		bool TimerHandle::IsActiveInvoke() const {
				// SPDLOG_TRACE();

				return thread_->Post<bool>([&]() -> bool { return IsActive(); }).get();
		}

		void TimerHandle::Close() {
				// SPDLOG_TRACE();

				if (is_close_.load()) {
						return;
				}

				is_close_.store(true);
				uv_close(reinterpret_cast<uv_handle_t*>(uv_handle_),
				         static_cast<uv_close_cb>(OnCloseTimer));
		}

		void TimerHandle::CloseInvoke() {
				// SPDLOG_TRACE();

				if (is_close_.load()) {
						return;
				}

				thread_->Post<void>([&]() { Close(); }).get();
		}

		inline void TimerHandle::OnUvTimer() {
				// SPDLOG_TRACE();

				listener_->OnTimer(this);
		}
}