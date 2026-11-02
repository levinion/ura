#include "ura/util/worker_pool.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace ura {

struct UraWorkerPool::Shared {
  std::mutex mutex;
  std::condition_variable ready;
  std::condition_variable finished_cv;
  std::function<void(std::size_t)> task;
  std::size_t count = 0;
  std::size_t finished = 0;
  std::size_t generation = 0;
  std::atomic<std::size_t> next { 0 };
  bool stop = false;
};

UraWorkerPool::UraWorkerPool(std::size_t threads) {
  if (threads == 0) {
    auto hardware = std::thread::hardware_concurrency();
    threads = std::min<std::size_t>(4, std::max<std::size_t>(1, hardware));
  }
  if (threads <= 1)
    return;

  this->shared = std::make_unique<Shared>();
  this->workers.reserve(threads);
  for (std::size_t i = 0; i < threads; ++i)
    this->workers.emplace_back([this] { this->run(); });
}

UraWorkerPool::~UraWorkerPool() {
  if (!this->shared)
    return;
  {
    std::lock_guard lock(this->shared->mutex);
    this->shared->stop = true;
  }
  this->shared->ready.notify_all();
  for (auto& worker : this->workers) {
    if (worker.joinable())
      worker.join();
  }
}

std::size_t UraWorkerPool::size() const {
  return this->workers.size();
}

void UraWorkerPool::run() {
  std::size_t seen = 0;
  while (true) {
    std::function<void(std::size_t)> task;
    std::size_t count = 0;
    {
      std::unique_lock lock(this->shared->mutex);
      this->shared->ready.wait(lock, [&] {
        return this->shared->stop || this->shared->generation != seen;
      });
      if (this->shared->stop)
        return;
      seen = this->shared->generation;
      task = this->shared->task;
      count = this->shared->count;
    }

    while (true) {
      auto index = this->shared->next.fetch_add(1);
      if (index >= count)
        break;
      task(index);
    }

    {
      std::lock_guard lock(this->shared->mutex);
      ++this->shared->finished;
      if (this->shared->finished == this->workers.size())
        this->shared->finished_cv.notify_one();
    }
  }
}

void UraWorkerPool::parallel_for(
  std::size_t count,
  const std::function<void(std::size_t)>& fn
) {
  if (count == 0)
    return;
  if (count == 1 || this->workers.empty()) {
    for (std::size_t i = 0; i < count; ++i) fn(i);
    return;
  }

  {
    std::lock_guard lock(this->shared->mutex);
    this->shared->task = fn;
    this->shared->count = count;
    this->shared->finished = 0;
    this->shared->next.store(0);
    ++this->shared->generation;
  }
  this->shared->ready.notify_all();

  std::unique_lock lock(this->shared->mutex);
  this->shared->finished_cv.wait(lock, [&] {
    return this->shared->finished == this->workers.size();
  });
  this->shared->task = nullptr;
  this->shared->count = 0;
}

} // namespace ura
