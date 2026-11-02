#include "ura/util/worker_pool.hpp"

#include <atomic>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

int main() {
  ura::UraWorkerPool pool(4);

  for (int round = 0; round < 4; ++round) {
    std::vector<int> values(128, -1);
    std::atomic<int> completed { 0 };
    std::mutex mutex;
    std::set<std::thread::id> threads;

    pool.parallel_for(values.size(), [&](std::size_t index) {
      values[index] = static_cast<int>(index) + round;
      completed.fetch_add(1, std::memory_order_relaxed);
      std::lock_guard lock(mutex);
      threads.insert(std::this_thread::get_id());
    });

    if (completed.load() != static_cast<int>(values.size())) {
      std::cerr << "not all tasks completed\n";
      return 1;
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (values[i] != static_cast<int>(i) + round) {
        std::cerr << "wrong task result\n";
        return 1;
      }
    }
    if (threads.empty() || threads.contains(std::this_thread::get_id())) {
      std::cerr << "tasks did not run on a worker thread\n";
      return 1;
    }
  }

  auto main_thread = std::this_thread::get_id();
  bool ran_inline = false;
  pool.parallel_for(1, [&](std::size_t) {
    ran_inline = std::this_thread::get_id() == main_thread;
  });
  if (!ran_inline) {
    std::cerr << "single task should stay on the main thread\n";
    return 1;
  }

  return 0;
}
