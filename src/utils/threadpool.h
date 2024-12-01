#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <thread>
#include <vector>

class ThreadPool
{
  public:
    ThreadPool(size_t n_threads);
    ~ThreadPool();

    void enqueue(std::function<void()> f);

    void enqueue_batch_and_wait(std::vector<std::function<void()>>& tasks);

    size_t get_num_threads() const
    {
        return threads_.size();
    }

  private:
    void worker_thread();

    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable worker_cv_;
    bool stop_;
    std::atomic<uint32_t> done_count_;
};