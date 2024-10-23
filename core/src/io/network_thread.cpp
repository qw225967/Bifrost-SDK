/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-15 11:14
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : network_thread.cpp
 * @description : TODO
 *******************************************************/

#include "io/network_thread.h"

#include <functional>
#include <future>

#include "spdlog/spdlog.h"
#include "utils/utils.h"

namespace CoreIO {
		inline static void OnAsync(uv_async_t* uv_async) {
				SPDLOG_TRACE();

				if (auto* network_thread = static_cast<NetworkThread*>(uv_async->data)) {
						network_thread->OnAsyncTask(uv_async);
				}
		}

		static void OnAsyncClose(uv_handle_t* uv_handle) {
				SPDLOG_TRACE();

				if (auto* network_thread = static_cast<NetworkThread*>(uv_handle->data))
				{
						network_thread->StopLoop();
				}

				delete reinterpret_cast<uv_async_t*>(uv_handle);
		}

		NetworkThread::NetworkThread()
		    : thread_(nullptr), loop_(nullptr), async_task_(nullptr) {
				SPDLOG_TRACE();
		}

		bool NetworkThread::Start() {
				SPDLOG_TRACE();

				loop_ = new uv_loop_t;
				if (const int err = uv_loop_init(loop_); err != 0) {
						SPDLOG_ERROR("libuv Init failed ...");
						return false;
				}

				async_task_ = new uv_async_t;
				uv_async_init(loop_, async_task_, OnAsync);
				async_task_->data = static_cast<void*>(this);

				std::promise<void> latch_promise;
				std::future<void> latch_future = latch_promise.get_future();

				thread_ = std::make_unique<std::thread>(
				    [this, &latch_promise]() { this->Run(std::ref(latch_promise)); });

				latch_future.get();

				SPDLOG_INFO("network success to Start");
				return true;
		}

		void NetworkThread::Run(std::promise<void>& promise) const {
				SPDLOG_TRACE();

				SPDLOG_INFO("network thread uv loop running");

				promise.set_value();

				uv_run(loop_, UV_RUN_DEFAULT);
				uv_loop_close(loop_);

				SPDLOG_INFO("network thread uv loop exit");
		}

		void NetworkThread::StopLoop() const {
				SPDLOG_TRACE();

				uv_stop(loop_);
		}

		void NetworkThread::OnAsyncTask(uv_async_t* uv_async) {
				SPDLOG_TRACE();

				std::vector<std::unique_ptr<TaskBase>> tasks;
				{
						std::unique_lock<std::mutex> lock(task_lists_mutex_);
						tasks.swap(task_lists_);
				}

				for (auto& task : tasks) {
						task->Execute();
				}
		}

		NetworkThread::~NetworkThread() {
				SPDLOG_TRACE();

				Post<void>([&]() {
						uv_close(reinterpret_cast<uv_handle_t*>(async_task_), OnAsyncClose);
				}).get();

				if (thread_) {
						thread_->join();
				}

				delete loop_;

				SPDLOG_INFO("network thread exit ...");
		}
} // namespace CoreIO