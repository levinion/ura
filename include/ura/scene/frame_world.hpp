#pragma once

#include "ura/scene/frame_plan.hpp"
#include "ura/util/worker_pool.hpp"

namespace ura {

// Builds the immutable scene snapshot consumed by the parallel frame-planning
// stage. Task discovery and ECS access happen on the main thread; workers only
// read disjoint wlroots surface trees and never mutate them.
FrameWorldSnapshot build_frame_world(UraWorkerPool& pool);

} // namespace ura
