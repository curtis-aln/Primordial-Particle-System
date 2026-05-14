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
    // - responsible for managing the tasks to be executed by the thread pool.
    // - uses a std::queue to store the tasks.
    // - Uses synchronization primitives (coordinates the execution of multiple threads & prevent race conditions) 
    //   to ensure thread-safety.
    struct TaskQueue
    {
        // container adapter that gives the functionality of a queue - a FIFO (first-in, first-out) data structure.
        std::queue<std::function<void()>> m_tasks; // queue of tasks to be executed

        // - (mutual exclusion) is a synchronization primitive which provides thread-safe access to the queue by preventing 
        //   multiple threads from simultaneously accessing a shared resource. 
        // - When a thread wants to access a shared resource, it attempts to "lock" the mutex.
        // - If the mutex is already locked by another thread, the requesting thread is blocked until the mutex becomes available.
        // - After finishing with the shared resource, the thread "unlocks" the mutex, allowing other threads to acquire it.
        std::mutex                        m_mutex;

        // - A synchronization primitive that enables threads to wait for a specific condition to occur.
        // - A thread locks a mutex and checks if a condition is met.
        // - If the condition is not met, it calls wait() on the condition variable, which atomically releases the mutex and puts the thread to sleep.
        // - When another thread changes the condition, it calls notify_one() or notify_all() on the condition variable.
        // - The waiting thread(s) wake up, reacquire the mutex, and check the condition again.
        std::condition_variable           m_condition_variable; // Used to notify worker threads when new tasks are available.

        // - std::atomic provides atomic operations on an enclosed value, ensuring that these operations are indivisible 
        //   and free from race conditions without needing explicit locking.
        // - used to keep track of the number of remaining tasks without risking data races when multiple threads increment or decrement it.
        std::atomic<uint32_t>             m_remaining_tasks = 0;

        bool                              m_stop = false; // flag for when the queue should stop operating

        // Adds a new task to the queue and notifies waiting threads
        template<typename TCallback>
        void addTask(TCallback&& callback)
        {
            {
                std::lock_guard<std::mutex> lock_guard{ m_mutex };

                // forward used for perfect forwarding in template functions & preserves the value category (lvalue or rvalue) of the argument when passing it to another function
                // - lvalue is an expression that refers to a memory location.
                // - rvalue is an expression that doesn't have a persistent memory address.
                m_tasks.push(std::forward<TCallback>(callback));
                m_remaining_tasks++;
            }

            // Wakes up one waiting thread
            // Used when a shared state protected by a mutex is modified and only one waiting thread needs to be notified of this change
            // used to wake up a worker thread when a new task is added to the queue.
            m_condition_variable.notify_one();
        }

        // Retrieves a task from the queue, waiting if necessary.
        bool getTask(std::function<void()>& target_callback)
        {
            // unique_lock is used here to protect access to shared data (the task queue) and work with the condition variable.
            std::unique_lock<std::mutex> lock_guard{ m_mutex };
            m_condition_variable.wait(lock_guard, [this]() { return !m_tasks.empty() || m_stop; });
            
            if (m_tasks.empty()) 
            {
                return false;
            }

            // std::move converts the expression to an rvalue, enabling move semantics. 
            // Move semantics allow the efficient transfer of resources from one object to another. Instead of copying, which can be expensive for large objects
            target_callback = std::move(m_tasks.front());
            m_tasks.pop();
            return true;
        }

        // waits until all tasks are completed
        void waitForCompletion() const
        {
            while (m_remaining_tasks > 0) 
            {
                // allowing other threads to run while waiting for tasks to complete.
                std::this_thread::yield();
            }
        }

        // Decrements the remaining tasks counter
        void workDone()
        {
            m_remaining_tasks--;
        }

        // Signals the queue to stop operating
        void stop()
        {
            {
                std::lock_guard<std::mutex> lock_guard{ m_mutex };
                m_stop = true;
            }

            // Wakes up all waiting threads. It's used when multiple threads might be waiting and all need to check a condition.
            // Wake up all worker threads when the thread pool is being shut down.
            m_condition_variable.notify_all();
        }
    };

    // A thread that executes tasks from the TaskQueue.
    struct Worker
    {
        uint32_t              m_id = 0;
        std::thread           m_thread;

        // - general-purpose polymorphic function wrapper (declares a member variable of type `std::function<void()>` initialized to `nullptr`)
        // - void() specifies that it wraps a function taking no arguments and returning nothing.
        std::function<void()> m_task = nullptr; // The current task being executed.
        TaskQueue* m_queue = nullptr; // pointer for fetching tasks

        Worker() = default;

        Worker(TaskQueue& queue, uint32_t id)
            : m_id{ id }
            , m_queue{ &queue }
        {
            m_thread = std::thread([this]() {
                run();
                });
        }

        // continuously fetching and executing tasks.
        void run()
        {
            while (true) 
            {
                if (!m_queue->getTask(m_task)) 
                {
                    break;
                }

                if (m_task) 
                {
                    try 
                    {
                        m_task();
                    }
                    catch (...) 
                    {
                        // Handle task exceptions here if needed
                    }
                    m_queue->workDone();
                    m_task = nullptr;
                }
            }
        }

        // Joins the worker thread, effectively stopping it
        void stop()
        {
            if (m_thread.joinable()) 
            {
                m_thread.join();
            }
        }
    };


    // manages a collection of Worker threads and provides the main interface
    struct ThreadPool
    {
        uint32_t            m_thread_count = 0;
        TaskQueue           m_queue; // The Task Queue shared by all workers
        std::vector<Worker> m_workers;

        // Initializes Self, Task Queue, and worker threads
        explicit ThreadPool(uint32_t thread_count)
            : m_thread_count{ thread_count }
        {
            m_workers.reserve(thread_count);
            
            for (uint32_t i{ thread_count }; i--;) 
            {
                m_workers.emplace_back(m_queue, static_cast<uint32_t>(m_workers.size()));
            }
        }

        ~ThreadPool()
        {
            m_queue.stop();

            for (Worker& worker : m_workers) 
            {
                worker.stop();
            }
        }

        // adds a task the the queue
        template<typename TCallback>
        void addTask(TCallback&& callback)
        {
            m_queue.addTask(std::forward<TCallback>(callback));
        }

        // waits for all tasks to complete
        void waitForCompletion() const
        {
            m_queue.waitForCompletion();
        }

        // Distributes work across the thread pool
        template<typename TCallback>
        void dispatch(uint32_t element_count, TCallback&& callback)
        {
            const uint32_t batch_size = element_count / m_thread_count;
            for (uint32_t i{ 0 }; i < m_thread_count; ++i) 
            {
                const uint32_t start = batch_size * i;
                const uint32_t end = start + batch_size;
                addTask([start, end, &callback]() { callback(start, end); });
            }

            if (batch_size * m_thread_count < element_count) 
            {
                const uint32_t start = batch_size * m_thread_count;
                callback(start, element_count);
            }

            waitForCompletion();
        }
    };

}
