#pragma once
#include <atomic>
#include <functional>
#include <semaphore>
#include <thread>
#include <vector>

class BarrierThreadPool
{
	struct Worker
	{
		std::counting_semaphore<1> start_sem{ 0 };
		int job_index = 0;          // which job this worker runs
		std::thread thread;
	};

	std::vector<std::unique_ptr<Worker>> workers_;
	const std::vector<std::function<void()>>* jobs_ = nullptr;  // non-owning ptr
	std::atomic<int>  pending_{ 0 };
	std::binary_semaphore all_done_sem_{ 0 };
	int  job_count_ = 0;

	std::atomic<bool> stop_{ false };

public:
	BarrierThreadPool() = default;

	explicit BarrierThreadPool(int n)
	{
		workers_.reserve(n);
		for (int i = 0; i < n; ++i)
		{
			auto& w = workers_.emplace_back(std::make_unique<Worker>());
			w->job_index = i;
			w->thread = std::thread([this, &worker = *w]
				{
					while (true)
					{
						worker.start_sem.acquire();
						if (stop_) return;

						(*jobs_)[worker.job_index]();  // index directly into stored ptr

						if (pending_.fetch_sub(1, std::memory_order_acq_rel) == 1)
							all_done_sem_.release();
					}
				});
		}
	}

	~BarrierThreadPool()
	{
		stop_.store(true, std::memory_order_release);

		// Wake every worker so it can see stop_ and exit.
		for (auto& worker : workers_)
			worker->start_sem.release();

		// Wait for all threads to finish.
		for (auto& worker : workers_)
		{
			if (worker->thread.joinable())
				worker->thread.join();
		}
	}

	// Call once at init, not every frame
	void set_jobs(const std::vector<std::function<void()>>& jobs)
	{
		assert((int)jobs.size() <= (int)workers_.size());
		jobs_ = &jobs;
		job_count_ = (int)jobs.size();
	}

	// Just signal and wait — no copies, no allocation
	void run_and_wait()
	{
		pending_.store(job_count_, std::memory_order_release);
		for (int i = 0; i < job_count_; ++i)
			workers_[i]->start_sem.release();
		all_done_sem_.acquire();
	}
};