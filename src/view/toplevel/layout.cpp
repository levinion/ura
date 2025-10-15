#include "ura/view/layout.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include "ura/lua/lua.hpp"

namespace ura::layout {

// void tiling(int index) {
//   auto server = UraServer::get_instance();
//   auto output = server->view->current_output();
//   if (!output)
//     return;
//   auto toplevel = output->current_workspace->get_toplevel_at(index);
//   if (!toplevel)
//     return;
//   if (toplevel->first_commit_after_layout_change) {
//     toplevel->draggable = false;
//     toplevel->set_z_index(UraSceneLayer::Normal);
//   }
//   auto geo = toplevel->geometry;
//   auto usable_area = output->usable_area;
//   auto width = usable_area.width;
//   auto height = usable_area.height;
//   auto outer_l =
//     server->lua->fetch<int>("opt.tilling.gap.outer.left").value_or(10);
//   auto outer_r =
//     server->lua->fetch<int>("opt.tilling.gap.outer.right").value_or(10);
//   auto outer_t =
//     server->lua->fetch<int>("opt.tilling.gap.outer.top").value_or(10);
//   auto outer_b =
//     server->lua->fetch<int>("opt.tilling.gap.outer.bottom").value_or(10);
//   auto inner = server->lua->fetch<int>("opt.tilling.gap.inner").value_or(10);
//   auto& toplevels = output->current_workspace->toplevels;
//   // find mapped toplevel number
//   int sum = 0;
//   for (auto window : toplevels) {
//     if (window->layout == toplevel->layout)
//       sum += 1;
//   }
//   // find this toplevel's index
//   int i = 0;
//   for (auto window : toplevels) {
//     if (window->layout != toplevel->layout)
//       continue;
//     if (window != toplevel)
//       i++;
//     else
//       break;
//   }
//   auto gaps = sum - 1;
//   auto w = (width - (outer_r + outer_l) - inner * gaps) / sum;
//   auto h = height - (outer_t + outer_b);
//   auto x = usable_area.x + outer_l + (w + inner) * i;
//   auto y = usable_area.y + outer_t;
//
//   toplevel->resize(w, h);
//   toplevel->move(x, y);
// }
//
// void fullscreen(int index) {
//   auto server = UraServer::get_instance();
//
//   auto toplevel =
//     server->view->current_output()->current_workspace->get_toplevel_at(index);
//   if (!toplevel)
//     return;
//   if (toplevel->first_commit_after_layout_change) {
//     toplevel->draggable = false;
//     toplevel->set_z_index(UraSceneLayer::Fullscreen);
//   }
//
//   auto geo = server->view->current_output()->logical_geometry();
//   auto table = server->lua->state.create_table();
//
//   toplevel->resize(geo.width, geo.height);
//   toplevel->move(geo.x, geo.y);
// }
//
// void floating(int index) {
//   auto server = UraServer::get_instance();
//   auto toplevel =
//     server->view->current_output()->current_workspace->get_toplevel_at(index);
//   if (!toplevel)
//     return;
//   if (toplevel->first_commit_after_layout_change) {
//     toplevel->draggable = true;
//     toplevel->set_z_index(UraSceneLayer::Floating);
//   }
// }
} // namespace ura::layout
