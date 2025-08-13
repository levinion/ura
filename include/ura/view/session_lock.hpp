#pragma once

#include "ura/view/output.hpp"

namespace ura {

class UraSessionLockSurface {
public:
  wlr_session_lock_surface_v1* surface;
  wlr_scene_tree* scene_tree;
  UraOutput* output;
  void focus();
  static UraSessionLockSurface* from(wlr_surface* surface);
};

class UraSessionLock {
public:
  wlr_session_lock_v1* lock;
  wlr_scene_tree* scene_tree;
};

} // namespace ura
