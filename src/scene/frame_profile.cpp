#include "ura/scene/frame_profile.hpp"
#include "ura/core/log.hpp"

#include <cstdlib>
#include <cstring>

namespace ura {

UraFrameProfiler::UraFrameProfiler() :
  tracing(false),
  last_report(Clock::now()) {
  const char* value = getenv("URA_FRAME_PROFILE");
  this->tracing = value && std::strcmp(value, "0") != 0;
}

bool UraFrameProfiler::enabled() const {
  return this->tracing;
}

void UraFrameProfiler::add(Stat& stat, Clock::duration duration) {
  auto ns = static_cast<std::uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()
  );
  ++stat.count;
  stat.total_ns += ns;
  if (ns > stat.max_ns)
    stat.max_ns = ns;
}

double UraFrameProfiler::avg_ms(const Stat& stat) {
  if (stat.count == 0)
    return 0.0;
  return static_cast<double>(stat.total_ns) / stat.count / 1e6;
}

double UraFrameProfiler::max_ms(const Stat& stat) {
  return static_cast<double>(stat.max_ns) / 1e6;
}

void UraFrameProfiler::record_frame(
  Clock::duration world,
  Clock::duration plan,
  Clock::duration merge,
  std::size_t outputs,
  std::size_t tasks,
  std::size_t surfaces,
  std::size_t rects,
  std::size_t operations
) {
  if (!this->tracing)
    return;
  add(this->world, world);
  add(this->plan, plan);
  add(this->merge, merge);
  ++this->frames;
  this->outputs += outputs;
  this->tasks += tasks;
  this->surfaces += surfaces;
  this->rects += rects;
  this->operations += operations;
}

void UraFrameProfiler::record_render(
  Clock::duration render,
  std::size_t surfaces,
  std::size_t damage_boxes,
  std::size_t operations,
  bool full_redraw,
  bool ok
) {
  if (!this->tracing)
    return;
  add(this->render, render);
  ++this->renders;
  if (full_redraw)
    ++this->full_redraws;
  this->damaged_surfaces += surfaces;
  this->damage_boxes += damage_boxes;
  this->draw_operations += operations;
  if (!ok)
    ++this->render_failures;
}

void UraFrameProfiler::record_skip() {
  if (this->tracing)
    ++this->skipped;
}

void UraFrameProfiler::maybe_report() {
  if (!this->tracing)
    return;
  auto now = Clock::now();
  if (now - this->last_report < std::chrono::seconds(5))
    return;
  if (this->frames == 0)
    return;

  auto frames = static_cast<double>(this->frames);
  log::info(
    "frame profile: frames={} skipped={} outputs/frame={:.2f} "
    "tasks/frame={:.2f} surfaces/frame={:.2f} rects/frame={:.2f} ops/frame={:.2f}",
    static_cast<unsigned long long>(this->frames),
    static_cast<unsigned long long>(this->skipped),
    this->outputs / frames,
    this->tasks / frames,
    this->surfaces / frames,
    this->rects / frames,
    this->operations / frames
  );
  auto renders = static_cast<double>(this->renders);
  log::info(
    "frame profile: world avg={:.3f}ms max={:.3f}ms | plan avg={:.3f}ms max={:.3f}ms "
    "| merge avg={:.3f}ms max={:.3f}ms | render avg={:.3f}ms max={:.3f}ms "
    "failures={}",
    avg_ms(this->world),
    max_ms(this->world),
    avg_ms(this->plan),
    max_ms(this->plan),
    avg_ms(this->merge),
    max_ms(this->merge),
    avg_ms(this->render),
    max_ms(this->render),
    static_cast<unsigned long long>(this->render_failures)
  );
  log::info(
    "frame profile: renders={} full={} surfaces/render={:.2f} "
    "ops/render={:.2f} damage/render={:.2f}",
    static_cast<unsigned long long>(this->renders),
    static_cast<unsigned long long>(this->full_redraws),
    renders > 0 ? this->damaged_surfaces / renders : 0.0,
    renders > 0 ? this->draw_operations / renders : 0.0,
    renders > 0 ? this->damage_boxes / renders : 0.0
  );

  this->last_report = now;
  this->reset();
}

void UraFrameProfiler::reset() {
  this->world = {};
  this->plan = {};
  this->merge = {};
  this->render = {};
  this->frames = 0;
  this->skipped = 0;
  this->outputs = 0;
  this->tasks = 0;
  this->surfaces = 0;
  this->rects = 0;
  this->operations = 0;
  this->renders = 0;
  this->full_redraws = 0;
  this->damaged_surfaces = 0;
  this->damage_boxes = 0;
  this->draw_operations = 0;
  this->render_failures = 0;
}

} // namespace ura
