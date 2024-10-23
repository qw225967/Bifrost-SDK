/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-21 下午2:36
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : time_handler.h
 * @description : TODO
 *******************************************************/

#ifndef TIME_HANDLER_H
#define TIME_HANDLER_H

#include "io/network_thread.h"
#include "uv.h"

#include <atomic>

namespace RTCUtils {
		class TimerHandle {
		public:
				class Listener {
				public:
						virtual ~Listener() = default;

				public:
						virtual void OnTimer(TimerHandle* timer) = 0;
				};

		public:
				explicit TimerHandle(Listener* listener,
				                     std::shared_ptr<CoreIO::NetworkThread> thread);

				~TimerHandle();

				TimerHandle(const TimerHandle&) = delete;

				TimerHandle& operator=(const TimerHandle&) = delete;

				bool Init();

				bool InitInvoke();

				bool Start(uint64_t timeout, uint64_t repeat = 0);

				bool StartInvoke(uint64_t timeout, uint64_t repeat = 0);

				void Stop();

				void StopInvoke();

				void Reset();

				void ResetInvoke();

				void Restart();

				void RestartInvoke();

				void Close();

				void CloseInvoke();

				bool IsActive() const;

				bool IsActiveInvoke() const;

		public:
				void OnUvTimer();

		private:
				Listener* listener_{ nullptr };
				std::shared_ptr<CoreIO::NetworkThread> thread_{ nullptr };
				uv_timer_t* uv_handle_{ nullptr };

				std::atomic<bool> close_{ false };
				uint64_t timeout_{ 0u };
				uint64_t repeat_{ 0u };
		};
} // namespace RTCUtils
#endif // TIME_HANDLER_H
