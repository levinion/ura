#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "ura/scene/frame_plan.hpp"
#include "ura/scene/frame_planner.hpp"
#include "ura/util/worker_pool.hpp"

namespace ura {

class UraOutput;
class UraFrameProfiler;

// Explicit frame stages:
//   main: build immutable world snapshot
//   parallel: build one FramePlan per output
//   main: enter/leave, primary-output and frame-done bookkeeping
//   main/GPU: render and commit one output plan
class UraFrameScheduler {
public:
  UraFrameScheduler(UraFramePlanner& planner, UraFrameProfiler& profiler);
  ~UraFrameScheduler();

  FramePlan take(UraOutput* output);
  void commit(UraOutput* output);
  // A transient render failure: keep enter/leave state, force full damage and
  // retry after a short backoff.
  void abort_frame(UraOutput* output);
  void invalidate();
  void mark_output_dirty(UraOutput* output);
  void invalidate_output(UraOutput* output);
  void remove(UraOutput* output);
  // Stops retry timers before the Wayland event loop is destroyed.
  void shutdown();

private:
  struct RetryState;

  UraFramePlanner& planner;
  UraFrameProfiler& profiler;
  UraWorkerPool pool;
  FrameWorldSnapshot world;

  struct CachedPlan {
    FramePlan plan;
    UraFramePlanner::OutputState next_state;
  };

  bool all_dirty = true;
  bool world_dirty = true;
  std::unordered_set<UraOutput*> dirty_outputs;
  std::unordered_set<UraOutput*> active_outputs;
  std::unordered_map<UraOutput*, CachedPlan> plans;
  std::unordered_map<UraOutput*, UraFramePlanner::OutputState> pending_states;
  std::unordered_map<UraOutput*, std::unique_ptr<RetryState>> retries;

  void ensure_world();
  void schedule_retry(UraOutput* output);
  void cancel_retry(UraOutput* output);
  static int handle_retry(void* data);
  void rebuild_all();
  void rebuild_output(UraOutput* output);
  FramePlan build_single(UraOutput* output);
};

} // namespace ura
