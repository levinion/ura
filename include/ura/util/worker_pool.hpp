#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace ura {

// Small persistent pool for coarse-grained frame work. The main thread blocks
// in parallel_for(), so tasks never outlive the snapshot they reference.
class UraWorkerPool {
public:
  explicit UraWorkerPool(std::size_t threads = 0);
  ~UraWorkerPool();

  UraWorkerPool(const UraWorkerPool&) = delete;
  UraWorkerPool& operator=(const UraWorkerPool&) = delete;

  std::size_t size() const;
  void
  parallel_for(std::size_t count, const std::function<void(std::size_t)>& fn);

private:
  struct Shared;
  std::unique_ptr<Shared> shared;
  std::vector<std::thread> workers;

  void run();
};

} // namespace ura
