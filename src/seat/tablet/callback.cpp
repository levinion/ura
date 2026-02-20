#include "ura/core/callback.hpp"
#include <utility>
#include "ura/core/runtime.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/server.hpp"
#include "ura/view/client.hpp"
#include "ura/view/view.hpp"
#include "ura/ura.hpp"

namespace ura {

void on_tablet_tip(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_tablet_tool_tip_event*>(data);
  if (!event->tool->data)
    return;
  auto tool = static_cast<wlr_tablet_v2_tablet_tool*>(event->tool->data);
  switch (event->state) {
    case WLR_TABLET_TOOL_TIP_DOWN: {
      wlr_tablet_v2_tablet_tool_notify_down(tool);
      break;
    }
    case WLR_TABLET_TOOL_TIP_UP: {
      wlr_tablet_v2_tablet_tool_notify_up(tool);
      break;
    }
  };
}

void on_tablet_button(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_tablet_tool_button_event*>(data);
  if (!event->tool->data)
    return;
  auto tool = static_cast<wlr_tablet_v2_tablet_tool*>(event->tool->data);

  zwp_tablet_pad_v2_button_state state;
  switch (event->state) {
    case WLR_BUTTON_PRESSED: {
      state = ZWP_TABLET_PAD_V2_BUTTON_STATE_PRESSED;
      break;
    }
    case WLR_BUTTON_RELEASED: {
      state = ZWP_TABLET_PAD_V2_BUTTON_STATE_RELEASED;
      break;
    }
    default:
      std::unreachable();
  };

  wlr_tablet_v2_tablet_tool_notify_button(tool, event->button, state);
}

void on_tablet_axis(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_tablet_tool_axis_event*>(data);
  if (!event->tool->data)
    return;
  auto tool = static_cast<wlr_tablet_v2_tablet_tool*>(event->tool->data);
  wlr_tablet_v2_tablet_tool_notify_distance(tool, event->distance);
  wlr_tablet_v2_tablet_tool_notify_motion(tool, event->dx, event->dy);
  wlr_tablet_v2_tablet_tool_notify_pressure(tool, event->pressure);
  wlr_tablet_v2_tablet_tool_notify_rotation(tool, event->rotation);
  wlr_tablet_v2_tablet_tool_notify_slider(tool, event->slider);
  wlr_tablet_v2_tablet_tool_notify_tilt(tool, event->tilt_x, event->tilt_y);
}

void on_tablet_proximity(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_tablet_tool_proximity_event*>(data);

  if (!event->tablet->data)
    return;
  auto tablet = static_cast<wlr_tablet_v2_tablet*>(event->tablet->data);

  auto server = UraServer::get_instance();

  if (!event->tool->data) {
    event->tool->data = wlr_tablet_tool_create(
      server->tablet_manager,
      server->seat->seat,
      event->tool
    );
  }

  auto tool = static_cast<wlr_tablet_v2_tablet_tool*>(event->tool->data);

  switch (event->state) {
    case WLR_TABLET_TOOL_PROXIMITY_IN: {
      auto client = server->view->foreground_client();
      if (!client)
        return;
      wlr_tablet_v2_tablet_tool_notify_proximity_in(
        tool,
        tablet,
        client->surface
      );
      break;
    }
    case WLR_TABLET_TOOL_PROXIMITY_OUT: {
      wlr_tablet_v2_tablet_tool_notify_proximity_out(tool);
      break;
    }
  }
}

void on_tablet_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto tablet = server->runtime->fetch<UraTablet*>(listener);
  server->runtime->remove(tablet);
  server->seat->tablets.remove(tablet);
  delete tablet;
}
} // namespace ura
