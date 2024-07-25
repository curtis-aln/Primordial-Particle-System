#pragma once
#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace tp
{

    struct TaskQueue
    {
        std::queue<std::function<void()>> m_tasks;
        std::mutex                        m_mutex;
        std::condition_variable           m_cv;
        std::atomic<uint32_t>             m_remaining_tasks = 0;
        bool                              m_stop = false;

        template<typename TCallback>
        void addTask(TCallback&& callback)
        {
            {
                std::lock_guard<std::mutex> lock_guard{ m_mutex };
                m_tasks.push(std::forward<TCallback>(callback));
                m_remaining_tasks++;
            }
            m_cv.notify_one();
        }

        bool getTask(std::function<void()>& target_callback)
        {
            std::unique_lock<std::mutex> lock_guard{ m_mutex };
            m_cv.wait(lock_guard, [this]() { return !m_tasks.empty() || m_stop; });
            if (m_tasks.empty()) {
                return false;
            }
            target_callback = std::move(m_tasks.front());
            m_tasks.pop();
            return true;
        }

        void waitForCompletion() const
        {
            while (m_remaining_tasks > 0) {
                std::this_thread::yield();
            }
        }

        void workDone()
        {
            m_remaining_tasks--;
        }

        void stop()
        {
            {
                std::lock_guard<std::mutex> lock_guard{ m_mutex };
                m_stop = true;
            }
            m_cv.notify_all();
        }
    };

    struct Worker
    {
        uint32_t              m_id = 0;
        std::thread           m_thread;
        std::function<void()> m_task = nullptr;
        TaskQueue* m_queue = nullptr;

        Worker() = default;

        Worker(TaskQueue& queue, uint32_t id)
            : m_id{ id }
            , m_queue{ &queue }
        {
            m_thread = std::thread([this]() {
                run();
                });
        }

        void run()
        {
            while (true) {
                if (!m_queue->getTask(m_task)) {
                    break;
                }
                if (m_task) {
                    try {
                        m_task();
                    }
                    catch (...) {
                        // Handle task exceptions here if needed
                    }
                    m_queue->workDone();
                    m_task = nullptr;
                }
            }
        }

        void stop()
        {
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }
    };

    struct ThreadPool
    {
        uint32_t            m_thread_count = 0;
        TaskQueue           m_queue;
        std::vector<Worker> m_workers;

        explicit ThreadPool(uint32_t thread_count)
            : m_thread_count{ thread_count }
        {
            m_workers.reserve(thread_count);
            for (uint32_t i{ thread_count }; i--;) {
                m_workers.emplace_back(m_queue, static_cast<uint32_t>(m_workers.size()));
            }
        }

        ~ThreadPool()
        {
            m_queue.stop();
            for (Worker& worker : m_workers) {
                worker.stop();
            }
        }

        template<typename TCallback>
        void addTask(TCallback&& callback)
        {
            m_queue.addTask(std::forward<TCallback>(callback));
        }

        void waitForCompletion() const
        {
            m_queue.waitForCompletion();
        }

        template<typename TCallback>
        void dispatch(uint32_t element_count, TCallback&& callback)
        {
            const uint32_t batch_size = element_count / m_thread_count;
            for (uint32_t i{ 0 }; i < m_thread_count; ++i) {
                const uint32_t start = batch_size * i;
                const uint32_t end = start + batch_size;
                addTask([start, end, &callback]() { callback(start, end); });
            }

            if (batch_size * m_thread_count < element_count) {
                const uint32_t start = batch_size * m_thread_count;
                callback(start, element_count);
            }

            waitForCompletion();
        }
    };

}
