#pragma once

#include "ura/view/output.hpp"

namespace ura {

class UraSessionLockSurface {
public:
  wlr_session_lock_surface_v1* surface;
  UraOutput* output;
  void focus();
  static UraSessionLockSurface* from(wlr_surface* surface);
};

class UraSessionLock {
public:
  wlr_session_lock_v1* lock;
};

} // namespace ura
