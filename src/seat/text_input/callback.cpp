#include "ura/callback.hpp"
#include "ura/client.hpp"
#include "ura/layer_shell.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/server.hpp"
#include "ura/runtime.hpp"
#include "ura/text_input.hpp"
#include "ura/ura.hpp"
#include "ura/seat.hpp"

namespace ura {
/* Text Input V3 Callbacks */

void on_new_text_input(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto text_input = static_cast<wlr_text_input_v3*>(data);
  server->seat->text_input->text_inputs.push_back(text_input);
  server->runtime->register_callback(
    &text_input->events.enable,
    on_text_input_enable,
    text_input
  );
  server->runtime->register_callback(
    &text_input->events.disable,
    on_text_input_disable,
    text_input
  );
  server->runtime->register_callback(
    &text_input->events.commit,
    on_text_input_commit,
    text_input
  );
  server->runtime->register_callback(
    &text_input->events.destroy,
    on_text_input_destroy,
    text_input
  );
}

void on_text_input_enable(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto text_input = static_cast<wlr_text_input_v3*>(data);
  auto input_method = server->seat->text_input->input_method;
  if (input_method) {
    wlr_input_method_v2_send_activate(input_method);
    server->seat->text_input->send_state(text_input);
  }
}

void on_text_input_disable(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto text_input = static_cast<wlr_text_input_v3*>(data);
  auto input_method = server->seat->text_input->input_method;
  if (input_method) {
    wlr_input_method_v2_send_deactivate(input_method);
    server->seat->text_input->send_state(text_input);
  }
}

void on_text_input_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto text_input = static_cast<wlr_text_input_v3*>(data);
  server->seat->text_input->send_state(text_input);
}

void on_text_input_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto text_input = static_cast<wlr_text_input_v3*>(data);
  if (server->seat->text_input->input_method
      && text_input == server->seat->text_input->get_active_text_input())
    wlr_input_method_v2_send_deactivate(server->seat->text_input->input_method);
  server->seat->text_input->text_inputs.remove(text_input);
  server->runtime->remove(text_input);
}

/* Input Method V2 Callbacks */

void on_new_input_method(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_method = static_cast<wlr_input_method_v2*>(data);
  server->seat->text_input->input_method = input_method;
  server->runtime->register_callback(
    &input_method->events.commit,
    on_input_method_commit,
    input_method
  );
  server->runtime->register_callback(
    &input_method->events.destroy,
    on_input_method_destroy,
    input_method
  );
  server->runtime->register_callback(
    &input_method->events.grab_keyboard,
    on_input_method_grab_keyboard,
    input_method
  );
  server->runtime->register_callback(
    &input_method->events.new_popup_surface,
    on_input_method_new_popup_surface,
    input_method
  );
  if (server->seat->text_input->get_active_text_input())
    wlr_input_method_v2_send_activate(input_method);
}

void on_input_method_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_method = static_cast<wlr_input_method_v2*>(data);
  server->seat->text_input->input_method = nullptr;
  server->runtime->remove(input_method);
  if (server->seat->text_input->get_active_text_input())
    wlr_text_input_v3_send_leave(
      server->seat->text_input->get_active_text_input()
    );
}

void on_input_method_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_method = static_cast<wlr_input_method_v2*>(data);
  auto active_text_input = server->seat->text_input->get_active_text_input();
  if (!active_text_input)
    return;
  auto& current = input_method->current;
  wlr_text_input_v3_send_preedit_string(
    active_text_input,
    current.preedit.text,
    current.preedit.cursor_begin,
    current.preedit.cursor_end
  );
  wlr_text_input_v3_send_commit_string(active_text_input, current.commit_text);
  wlr_text_input_v3_send_delete_surrounding_text(
    active_text_input,
    current.delete_.before_length,
    current.delete_.after_length
  );
  wlr_text_input_v3_send_done(active_text_input);
}

void on_input_method_grab_keyboard(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard_grab = static_cast<wlr_input_method_keyboard_grab_v2*>(data);
  auto keyboard = server->seat->seat->keyboard_state.keyboard;
  wlr_input_method_keyboard_grab_v2_set_keyboard(keyboard_grab, keyboard);
  server->runtime->register_callback(
    &keyboard_grab->events.destroy,
    on_input_method_grab_keyboard_destroy,
    keyboard_grab
  );
}

void on_input_method_grab_keyboard_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard_grab = static_cast<wlr_input_method_keyboard_grab_v2*>(data);
  auto seat = server->seat->seat;
  if (keyboard_grab->keyboard) {
    wlr_seat_set_keyboard(seat, keyboard_grab->keyboard);
    wlr_seat_keyboard_notify_modifiers(
      seat,
      &keyboard_grab->keyboard->modifiers
    );
  }
  server->runtime->remove(keyboard_grab);
}

void on_input_method_new_popup_surface(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup_surface = static_cast<wlr_input_popup_surface_v2*>(data);
  auto popup = new UraInputPopup {};

  popup->popup_surface = popup_surface;
  popup->popup_surface->data = popup;
  server->seat->text_input->popups.push_back(popup);
  popup->scene_tree = wlr_scene_subsurface_tree_create(
    server->current_output()->popup,
    popup->popup_surface->surface
  );

  server->runtime->register_callback(
    &popup_surface->events.destroy,
    on_input_method_popup_surface_destroy,
    popup
  );
  server->runtime->register_callback(
    &popup_surface->surface->events.map,
    on_input_method_popup_surface_map,
    popup
  );
  server->runtime->register_callback(
    &popup_surface->surface->events.unmap,
    on_input_method_popup_surface_unmap,
    popup
  );
  server->runtime->register_callback(
    &popup_surface->surface->events.commit,
    on_input_method_popup_surface_commit,
    popup
  );
}

void on_input_method_popup_surface_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_popup = server->runtime->fetch<UraInputPopup*>(listener);
  server->runtime->remove(input_popup);
  wlr_scene_node_destroy(&input_popup->scene_tree->node);
  delete input_popup;
}

void on_input_method_popup_surface_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_popup = server->runtime->fetch<UraInputPopup*>(listener);
  auto toplevel = server->current_output()->get_focused_toplevel();
  if (!toplevel)
    return;
  auto parent_scene_tree = toplevel.value()->scene_tree;
  if (!parent_scene_tree)
    return;
  auto active_text_input = server->seat->text_input->get_active_text_input();
  if (!active_text_input)
    return;
  wlr_scene_node_reparent(&input_popup->scene_tree->node, parent_scene_tree);
  auto cursor_rect = active_text_input->current.cursor_rectangle;
  wlr_input_popup_surface_v2_send_text_input_rectangle(
    input_popup->popup_surface,
    &cursor_rect
  );
  auto popup_x = cursor_rect.x;
  auto popup_y = cursor_rect.y + cursor_rect.height;
  wlr_scene_node_set_position(&input_popup->scene_tree->node, popup_x, popup_y);
  wlr_scene_node_set_enabled(&input_popup->scene_tree->node, true);
}

void on_input_method_popup_surface_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_popup = server->runtime->fetch<UraInputPopup*>(listener);
  if (input_popup->scene_tree) {
    wlr_scene_node_set_enabled(&input_popup->scene_tree->node, false);
  }
}

void on_input_method_popup_surface_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto input_popup = server->runtime->fetch<UraInputPopup*>(listener);
  if (!input_popup->scene_tree || !input_popup->scene_tree->node.enabled)
    return;
  auto active_text_input = server->seat->text_input->get_active_text_input();
  if (!active_text_input)
    return;
  auto cursor_rect = active_text_input->current.cursor_rectangle;
  wlr_input_popup_surface_v2_send_text_input_rectangle(
    input_popup->popup_surface,
    &cursor_rect
  );
}

} // namespace ura
