#include "ura/scene/frame_scheduler.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "ura/core/server.hpp"
#include "ura/scene/frame_profile.hpp"
#include "ura/scene/frame_world.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"

namespace ura {

struct UraFrameScheduler::RetryState {
  UraFrameScheduler* scheduler = nullptr;
  UraOutput* output = nullptr;
  wl_event_source* source = nullptr;
  uint32_t failures = 0;
};

namespace {

  bool has_software_cursor(wlr_output* output) {
    wlr_output_cursor* cursor;
    wl_list_for_each(cursor, &output->cursors, link) {
      if (
        cursor->enabled && cursor->visible && cursor != output->hardware_cursor
      )
        return true;
    }
    return false;
  }

  std::optional<OutputFrameState> make_output_state(UraOutput* output) {
    if (!output->output)
      return {};
    auto geometry = output->logical_geometry();
    if (geometry.width <= 0 || geometry.height <= 0)
      return {};
    return OutputFrameState {
      .output = output,
      .native = output->output,
      .enabled = output->output->enabled,
      .name = output->name,
      .logical_box = geometry.to_wlr_box(),
      .scale = output->output->scale,
      .transform = output->output->transform,
      .software_cursor = has_software_cursor(output->output),
      .damage_ring = output->damage_ring(),
      .background_color = output->background_color,
    };
  }

} // namespace

UraFrameScheduler::UraFrameScheduler(
  UraFramePlanner& planner,
  UraFrameProfiler& profiler
) :
  planner(planner),
  profiler(profiler) {}

UraFrameScheduler::~UraFrameScheduler() {
  this->shutdown();
}

void UraFrameScheduler::shutdown() {
  for (auto& [output, retry] : this->retries) {
    if (retry->source)
      wl_event_source_remove(retry->source);
    retry->source = nullptr;
  }
  this->retries.clear();
}

int UraFrameScheduler::handle_retry(void* data) {
  auto* retry = static_cast<RetryState*>(data);
  auto* scheduler = retry->scheduler;
  auto* output = retry->output;

  // Keep the timer source around for the next backoff interval. It is removed
  // on a successful commit, output teardown or scheduler shutdown.
  scheduler->dirty_outputs.insert(output);
  if (output && output->output && output->output->enabled)
    wlr_output_schedule_frame(output->output);
  return 0;
}

void UraFrameScheduler::schedule_retry(UraOutput* output) {
  if (!output || !output->output || !output->output->enabled)
    return;

  auto it = this->retries.find(output);
  if (it == this->retries.end()) {
    auto retry = std::make_unique<RetryState>();
    retry->scheduler = this;
    retry->output = output;
    it = this->retries.emplace(output, std::move(retry)).first;
  }

  auto& retry = *it->second;
  retry.failures = std::min<uint32_t>(retry.failures + 1, 5);
  auto delay = std::min<uint32_t>(16u << (retry.failures - 1), 250u);

  if (!retry.source) {
    auto* server = UraServer::get_instance();
    if (!server || !server->display) {
      wlr_output_schedule_frame(output->output);
      return;
    }
    auto* loop = wl_display_get_event_loop(server->display);
    retry.source = wl_event_loop_add_timer(loop, handle_retry, &retry);
    if (!retry.source) {
      wlr_output_schedule_frame(output->output);
      return;
    }
  }
  wl_event_source_timer_update(retry.source, static_cast<int>(delay));
}

void UraFrameScheduler::cancel_retry(UraOutput* output) {
  auto it = this->retries.find(output);
  if (it == this->retries.end())
    return;
  if (it->second->source)
    wl_event_source_remove(it->second->source);
  this->retries.erase(it);
}

void UraFrameScheduler::invalidate() {
  this->all_dirty = true;
  this->world_dirty = true;
  this->dirty_outputs.clear();
}

void UraFrameScheduler::mark_output_dirty(UraOutput* output) {
  this->plans.erase(output);
  this->dirty_outputs.insert(output);
  this->world_dirty = true;
}

void UraFrameScheduler::invalidate_output(UraOutput* output) {
  this->cancel_retry(output);
  this->plans.erase(output);
  this->pending_states.erase(output);
  this->active_outputs.erase(output);
  this->dirty_outputs.erase(output);
  this->planner.reset_output(output);
  this->all_dirty = true;
  this->world_dirty = true;
}

void UraFrameScheduler::ensure_world() {
  if (!this->world_dirty)
    return;
  this->world = build_frame_world();
  this->world_dirty = false;
}

void UraFrameScheduler::rebuild_all() {
  auto start = UraFrameProfiler::Clock::now();
  this->ensure_world();
  auto after_world = UraFrameProfiler::Clock::now();

  auto* server = UraServer::get_instance();
  auto* view = server->view.get();

  std::vector<OutputFrameState> outputs;
  std::vector<UraFramePlanner::OutputState*> previous_states;
  std::vector<UraFramePlanner::OutputState> next_states;
  std::unordered_set<UraOutput*> next_active;
  outputs.reserve(view->outputs.size());
  previous_states.reserve(view->outputs.size());
  next_states.resize(view->outputs.size());

  for (auto& [name, output] : view->outputs) {
    auto state = make_output_state(output);
    if (!state || !state->enabled)
      continue;
    previous_states.push_back(this->planner.prepare_output(output));
    outputs.push_back(*state);
    next_active.insert(output);
  }

  for (auto* output : this->active_outputs) {
    if (!next_active.contains(output))
      this->planner.reset_output(output);
  }
  this->active_outputs = std::move(next_active);

  std::vector<FramePlan> results(outputs.size());
  auto estimated_work = outputs.size() * this->world.operations.size();
  this->pool
    .parallel_for(outputs.size(), estimated_work, [&](std::size_t index) {
      results[index] = this->planner.build(
        outputs[index],
        *previous_states[index],
        next_states[index],
        this->world
      );
    });
  auto after_plan = UraFrameProfiler::Clock::now();

  this->plans.clear();
  for (std::size_t i = 0; i < outputs.size(); ++i) {
    this->planner.finish_plan(outputs[i].output, results[i]);
    this->plans.emplace(
      outputs[i].output,
      CachedPlan {
        .plan = std::move(results[i]),
        .next_state = std::move(next_states[i]),
      }
    );
  }
  auto after_merge = UraFrameProfiler::Clock::now();

  this->profiler.record_frame(
    after_world - start,
    after_plan - after_world,
    after_merge - after_plan,
    outputs.size(),
    this->world.tasks,
    this->world.surfaces.size(),
    this->world.rects.size(),
    this->world.operations.size()
  );
  this->dirty_outputs.clear();
  this->all_dirty = false;
}

void UraFrameScheduler::rebuild_output(UraOutput* output) {
  auto start = UraFrameProfiler::Clock::now();
  this->ensure_world();
  auto after_world = UraFrameProfiler::Clock::now();

  auto state = make_output_state(output);
  if (!state || !state->enabled) {
    this->planner.reset_output(output);
    this->plans.erase(output);
    this->dirty_outputs.erase(output);
    this->active_outputs.erase(output);
    return;
  }

  auto* previous = this->planner.prepare_output(output);
  UraFramePlanner::OutputState next;
  auto plan = this->planner.build(*state, *previous, next, this->world);
  auto after_plan = UraFrameProfiler::Clock::now();
  this->planner.finish_plan(output, plan);
  auto after_merge = UraFrameProfiler::Clock::now();

  this->plans.erase(output);
  this->plans.emplace(
    output,
    CachedPlan {
      .plan = std::move(plan),
      .next_state = std::move(next),
    }
  );
  this->dirty_outputs.erase(output);
  this->active_outputs.insert(output);

  this->profiler.record_frame(
    after_world - start,
    after_plan - after_world,
    after_merge - after_plan,
    1,
    this->world.tasks,
    this->world.surfaces.size(),
    this->world.rects.size(),
    this->world.operations.size()
  );
}

FramePlan UraFrameScheduler::build_single(UraOutput* output) {
  this->ensure_world();
  auto state = make_output_state(output);
  if (!state) {
    this->planner.reset_output(output);
    return {};
  }
  auto* previous = this->planner.prepare_output(output);
  UraFramePlanner::OutputState next;
  auto plan = this->planner.build(*state, *previous, next, this->world);
  this->planner.finish_plan(output, plan);
  this->pending_states[output] = std::move(next);
  return plan;
}

FramePlan UraFrameScheduler::take(UraOutput* output) {
  if (this->all_dirty)
    this->rebuild_all();
  else if (this->dirty_outputs.contains(output))
    this->rebuild_output(output);

  auto it = this->plans.find(output);
  if (it == this->plans.end())
    return this->build_single(output);

  auto plan = std::move(it->second.plan);
  this->pending_states[output] = std::move(it->second.next_state);
  this->plans.erase(it);
  return plan;
}

void UraFrameScheduler::commit(UraOutput* output) {
  this->cancel_retry(output);
  auto it = this->pending_states.find(output);
  if (it == this->pending_states.end())
    return;
  this->planner.commit_state(output, std::move(it->second));
  this->pending_states.erase(it);
}

void UraFrameScheduler::abort_frame(UraOutput* output) {
  if (!output || !output->output || !output->output->enabled) {
    this->invalidate_output(output);
    return;
  }

  this->plans.erase(output);
  this->pending_states.erase(output);
  this->planner.abort_frame(output);
  this->dirty_outputs.insert(output);
  this->schedule_retry(output);
}

void UraFrameScheduler::remove(UraOutput* output) {
  this->cancel_retry(output);
  this->plans.erase(output);
  this->pending_states.erase(output);
  this->active_outputs.erase(output);
  this->dirty_outputs.erase(output);
  this->planner.remove(output);
  this->all_dirty = true;
  this->world_dirty = true;
}

} // namespace ura
