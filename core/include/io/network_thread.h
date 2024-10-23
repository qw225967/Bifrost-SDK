/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-15 11:05
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : network_thread.h
 * @description : 该类使用libuv进行io操作，使用最常用的事件循环方式进行读写
 *                libuv的循环运行需要单独的线程运行，因此此处设计了一个线程操作
 *******************************************************/

#ifndef NETWORK_THREAD_H
#define NETWORK_THREAD_H
#include "utils/cpp11_adaptor.h"
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "uv.h"

namespace CoreIO {
		class NetworkThread {
		public:
				NetworkThread();

				~NetworkThread();

				NetworkThread(const NetworkThread&) = delete;

				NetworkThread& operator=(const NetworkThread&) = delete;

				bool Start();

				// 需要异步调用，在run开始后会阻塞
				void Run(std::promise<void>& promise) const;

				// 内部libuv的loop_指针，在libuv存活过程中一直存在
				[[nodiscard]] uv_loop_t* GetLoop() const {
						return loop_;
				}

				void OnAsyncTask(uv_async_t* handle);

				void StopLoop() const;

		public:
				template<class T>
				std::future<T> Post(std::function<T()> f) {
						auto task = Cpp11Adaptor::make_unique<Task<T>>(std::move(f));
						std::future<T> future = task->promise->get_future();

						{
								std::unique_lock<std::mutex> lock(task_lists_mutex_);
								task_lists_.push_back(std::move(task));
						}

						uv_async_send(async_task_);

						return future;
				};

		private:
				struct TaskBase {
						virtual ~TaskBase() = default;

						virtual void Execute() = 0;
				};

				template<typename T>
				struct Task : public TaskBase {
						std::shared_ptr<std::promise<T>> promise;
						std::function<T()> func;

						Task(std::function<T()> f)
						    : promise(Cpp11Adaptor::make_shared<std::promise<T>>()),
						      func(std::move(f)) {
						}

						void Execute() override {
								promise->set_value(func());
						}
				};

		private:
				std::unique_ptr<std::thread> thread_;
				uv_loop_t* loop_;
				uv_async_t* async_task_;

				std::mutex task_lists_mutex_;
				std::vector<std::unique_ptr<TaskBase>> task_lists_;
		};

		template<>
		struct NetworkThread::Task<void> : public NetworkThread::TaskBase {
				std::shared_ptr<std::promise<void>> promise;
				std::function<void()> func;

				Task(std::function<void()> f)
				    : promise(Cpp11Adaptor::make_shared<std::promise<void>>()),
				      func(std::move(f)) {
				}

				void Execute() override {
						func();
						promise->set_value();
				}
		};
} // namespace CoreIO
#endif // NETWORK_THREAD_H
