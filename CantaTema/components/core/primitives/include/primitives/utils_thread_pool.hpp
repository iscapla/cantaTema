#ifndef UTILS_THREAD_POOL_HPP
#define UTILS_THREAD_POOL_HPP

#include <iostream>
#include <vector>
#include <future>
#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(size_t numThreads)
        : numThreads(numThreads), stopFlag(false), unfinishedTasks(0)
    {}

    ~ThreadPool() {
        threads_destroy();
    }

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using returnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<returnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<returnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (stopFlag)
                throw std::runtime_error("submit on stopped ThreadPool");

            tasks.emplace([task]() { (*task)(); });
            ++unfinishedTasks;
        }
        condition.notify_one();
        return result;
    }

    void waitForCompletion() {
        std::unique_lock<std::mutex> lock(queueMutex);
        taskCompleted.wait(lock, [this] { return unfinishedTasks == 0; });
    }

    void threads_initialize() {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stopFlag || !tasks.empty(); });
                        if (stopFlag && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        --unfinishedTasks;
                    }
                    taskCompleted.notify_all();
                }
            });
        }
    }

    void threads_destroy() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    size_t numThreads;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable taskCompleted;
    std::atomic<int> unfinishedTasks;
    std::atomic<bool> stopFlag;
};

#endif // UTILS_THREAD_POOL_HPP