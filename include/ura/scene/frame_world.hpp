#pragma once

#include "ura/scene/frame_plan.hpp"

namespace ura {

// Builds the immutable scene snapshot consumed by the frame-planning stage.
// This must run on the main thread: it walks wlroots surface trees, reads
// surface textures and timelines, and is not safe to parallelize with wlroots.
FrameWorldSnapshot build_frame_world();

} // namespace ura
