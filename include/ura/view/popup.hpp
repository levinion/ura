#pragma once

#include "ura/core/entity.hpp"
#include <memory>
#include "ura/ura.hpp"
#include "ura/view/output.hpp"

namespace ura {

class UraClient;

class UraPopup {
public:
  entt::entity entity { entt::null };
  wlr_xdg_popup* xdg_popup;
  std::unique_ptr<UraClient> parent;
  bool init(wlr_xdg_popup* xdg_popup);
  static UraPopup* from(wlr_surface* surface);
  static UraPopup* from(uint64_t id);
  void commit();
  void destroy();
  uint64_t id();
  void unfocus();
  UraOutput* output();

private:
  void unconstrain();
};

} // namespace ura
