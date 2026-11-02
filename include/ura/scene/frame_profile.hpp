#pragma once

#include <chrono>
#include <cstdint>

namespace ura {

class UraFrameProfiler {
public:
  using Clock = std::chrono::steady_clock;

  UraFrameProfiler();

  bool enabled() const;
  void record_frame(
    Clock::duration world,
    Clock::duration plan,
    Clock::duration merge,
    std::size_t outputs,
    std::size_t tasks,
    std::size_t surfaces,
    std::size_t rects,
    std::size_t operations
  );
  void record_render(
    Clock::duration render,
    std::size_t surfaces,
    std::size_t damage_boxes,
    std::size_t operations,
    bool full_redraw,
    bool ok
  );
  void record_skip();
  void maybe_report();

private:
  struct Stat {
    std::uint64_t count = 0;
    std::uint64_t total_ns = 0;
    std::uint64_t max_ns = 0;
  };

  bool tracing;
  Stat world;
  Stat plan;
  Stat merge;
  Stat render;
  std::uint64_t frames = 0;
  std::uint64_t skipped = 0;
  std::uint64_t outputs = 0;
  std::uint64_t tasks = 0;
  std::uint64_t surfaces = 0;
  std::uint64_t rects = 0;
  std::uint64_t operations = 0;
  std::uint64_t renders = 0;
  std::uint64_t full_redraws = 0;
  std::uint64_t damaged_surfaces = 0;
  std::uint64_t damage_boxes = 0;
  std::uint64_t draw_operations = 0;
  std::uint64_t render_failures = 0;
  Clock::time_point last_report;

  static void add(Stat& stat, Clock::duration duration);
  static double avg_ms(const Stat& stat);
  static double max_ms(const Stat& stat);
  void reset();
};

} // namespace ura
