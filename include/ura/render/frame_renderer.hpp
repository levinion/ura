#pragma once

#include "ura/scene/frame_plan.hpp"

namespace ura {

bool render_frame(wlr_renderer* renderer, const FramePlan& plan);

} // namespace ura
