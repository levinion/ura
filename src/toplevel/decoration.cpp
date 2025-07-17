#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/toplevel.hpp"
#include <cassert>

namespace ura {

void on_new_toplevel_decoration(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel_decoration = static_cast<wlr_xdg_toplevel_decoration_v1*>(data);

  auto toplevel = UraToplevel::from(toplevel_decoration->toplevel);
  toplevel->decoration = toplevel_decoration;
  if (toplevel->initialized()) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
      toplevel_decoration,
      WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
  }
}

} // namespace ura
