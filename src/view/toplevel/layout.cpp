#include "ura/layout.hpp"
#include "ura/output.hpp"

namespace ura::layout {

std::optional<sol::table> tiling(int index) {
  auto server = UraServer::get_instance();
  auto toplevel =
    server->current_output()->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return {};
  if (toplevel.value()->initial_commit) {
    toplevel.value()->draggable = false;
    auto layer = toplevel.value()->layer =
      server->view->try_get_scene_tree(UraSceneLayer::Normal);
    toplevel.value()->set_layer(layer);
  }
  auto geo = toplevel.value()->geometry;
  auto usable_area = toplevel.value()->output->usable_area;
  auto width = usable_area.width;
  auto height = usable_area.height;
  auto outer_l =
    server->lua->fetch<int>("opt.tilling.gap.outer.left").value_or(10);
  auto outer_r =
    server->lua->fetch<int>("opt.tilling.gap.outer.right").value_or(10);
  auto outer_t =
    server->lua->fetch<int>("opt.tilling.gap.outer.top").value_or(10);
  auto outer_b =
    server->lua->fetch<int>("opt.tilling.gap.outer.bottom").value_or(10);
  auto inner = server->lua->fetch<int>("opt.tilling.gap.inner").value_or(10);
  auto& toplevels = toplevel.value()->output->current_workspace->toplevels;
  // find mapped toplevel number
  int sum = 0;
  for (auto window : toplevels) {
    if (window->layout == toplevel.value()->layout)
      sum += 1;
  }
  // no toplevel to arrage
  if (sum == 0)
    return {};
  // find this toplevel's index
  int i = 0;
  for (auto window : toplevels) {
    if (window->layout != toplevel.value()->layout)
      continue;
    if (window != toplevel.value())
      i++;
    else
      break;
  }
  auto gaps = sum - 1;
  auto w = (width - (outer_r + outer_l) - inner * gaps) / sum;
  auto h = height - (outer_t + outer_b);
  auto x = usable_area.x + outer_l + (w + inner) * i;
  auto y = usable_area.y + outer_t;

  auto table = server->lua->state.create_table();
  table["x"] = x;
  table["y"] = y;
  table["width"] = w;
  table["height"] = h;
  return table;
}

std::optional<sol::table> fullscreen(int index) {
  auto server = UraServer::get_instance();

  auto toplevel =
    server->current_output()->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return {};
  if (toplevel.value()->initial_commit) {
    toplevel.value()->draggable = false;
    auto layer = server->view->try_get_scene_tree(UraSceneLayer::Fullscreen);
    toplevel.value()->set_layer(layer);
  }

  auto geo = server->current_output()->logical_geometry();
  auto table = server->lua->state.create_table();
  table["x"] = geo.x;
  table["y"] = geo.y;
  table["width"] = geo.width;
  table["height"] = geo.height;
  return table;
}

std::optional<sol::table> floating(int index) {
  auto server = UraServer::get_instance();

  auto toplevel =
    server->current_output()->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return {};

  if (toplevel.value()->initial_commit) {
    toplevel.value()->draggable = true;
    auto layer = server->view->try_get_scene_tree(UraSceneLayer::Floating);
    toplevel.value()->set_layer(layer);
  }

  auto geo = toplevel.value()->geometry;
  auto usable_area = toplevel.value()->output->usable_area;
  int w = geo.width, h = geo.height, x = geo.x, y = geo.y;
  if (toplevel.value()->initial_commit
      && !toplevel.value()->xdg_toplevel->base->initial_commit) {
    w = server->lua->fetch<int>("opt.floating.default.width").value_or(800);
    h = server->lua->fetch<int>("opt.floating.default.height").value_or(600);
    x = usable_area.x + (usable_area.width - w) / 2;
    y = usable_area.y + (usable_area.height - h) / 2;
  }
  auto table = server->lua->state.create_table();
  table["x"] = x;
  table["y"] = y;
  table["width"] = w;
  table["height"] = h;
  return table;
}
} // namespace ura::layout
