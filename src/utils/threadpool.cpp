#include "threadpool.h"

#include <cassert>

ThreadPool::ThreadPool(size_t n_threads) : stop_(false)
{
    for (size_t i = 0; i < n_threads; ++i)
    {
        threads_.emplace_back([this] { this->worker_thread(); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();

    for (auto& t : threads_)
    {
        t.join();
    }
}

void ThreadPool::enqueue(std::function<void()> f)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace(f);
    }
    condition_.notify_one();
}

void ThreadPool::enqueue_batch_and_wait(std::vector<std::function<void()>>& tasks)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        assert(tasks_.empty());
        done_count_ = 0;
        for (auto& task : tasks)
        {
            tasks_.emplace(task);
        }
    }
    condition_.notify_all();

    std::unique_lock<std::mutex> lock(queue_mutex_);
    worker_cv_.wait(lock, [this, &tasks] { return done_count_ == tasks.size(); });
}

void ThreadPool::worker_thread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty())
            {
                return;
            }

            task = tasks_.front();
            tasks_.pop();
        }

        task();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            done_count_.fetch_add(1);
            worker_cv_.notify_one();
        }
    }
}