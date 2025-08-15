#pragma once

#include "ura/ura.hpp"

namespace ura {

// output/callback.cpp
void on_new_output(wl_listener* listener, void* data);
void on_output_frame(wl_listener* listener, void* data);
void on_output_request_state(wl_listener* listener, void* data);
void on_output_destroy(wl_listener* listener, void* data);
void on_output_manager_apply(wl_listener* listener, void* data);
void on_output_power_manager_set_mode(wl_listener* listener, void* data);

// seat/callback.cpp
void on_new_input(wl_listener* listener, void* data);

// seat/idle.cpp
void on_new_idle_inhibitor(wl_listener* listener, void* data);
void on_idle_inhibitor_destroy(wl_listener* listener, void* data);

// seat/cursor/callback.cpp
void on_cursor_motion(wl_listener* listener, void* data);
void on_cursor_motion_absolute(wl_listener* listener, void* data);
void on_cursor_button(wl_listener* listener, void* data);
void on_cursor_axis(wl_listener* listener, void* data);
void on_cursor_frame(wl_listener* listener, void* data);
void on_seat_request_cursor(wl_listener* listener, void* data);
void on_seat_request_set_selection(wl_listener* listener, void* data);
void on_cursor_request_set_shape(wl_listener* listener, void* data);
void on_seat_request_set_primary_selection(wl_listener* listener, void* data);
void on_seat_request_start_drag(wl_listener* listener, void* data);
void on_seat_start_drag(wl_listener* listener, void* data);
void on_pointer_constraints_new_constraint(wl_listener* listener, void* data);
void on_pointer_constraints_constraint_set_region(
  wl_listener* listener,
  void* data
);
void on_pointer_constraints_constraint_destroy(
  wl_listener* listener,
  void* data
);

// seat/keyboard/callback.cpp
void on_keyboard_modifiers(wl_listener* listener, void* data);
void on_keyboard_key(wl_listener* listener, void* data);
void on_keyboard_destroy(wl_listener* listener, void* data);
void on_new_virtual_keyboard(wl_listener* listener, void* data);
void on_new_keyboard_shortcuts_inhibitor(wl_listener* listener, void* data);
void on_keyboard_shortcuts_inhibitor_destroy(wl_listener* listener, void* data);

// view/toplevel/callback.cpp
void on_new_toplevel(wl_listener* listener, void* data);
void on_toplevel_map(wl_listener* listener, void* data);
void on_toplevel_unmap(wl_listener* listener, void* data);
void on_toplevel_commit(wl_listener* listener, void* data);
void on_toplevel_destroy(wl_listener* listener, void* data);
// TODO:
// void on_toplevel_request_move(wl_listener* listener, void* data);
// void on_toplevel_request_resize(wl_listener* listener, void* data);
// void on_toplevel_request_maximize(wl_listener* listener, void* data);
void on_toplevel_request_fullscreen(wl_listener* listener, void* data);

// view/toplevel/decoration.cpp
void on_new_toplevel_decoration(wl_listener* listener, void* data);
void on_toplevel_decoration_destroy(wl_listener* listener, void* data);
void on_toplevel_decoration_request_mode(wl_listener* listener, void* data);
void on_new_server_decoration(wl_listener* listener, void* data);
void on_server_decoration_mode(wl_listener* listener, void* data);

// view/toplevel/activation.cpp
void on_activation_request_activate(wl_listener* listener, void* data);

// view/toplevel/foreign.cpp
void on_foreign_toplevel_handle_request_activate(
  wl_listener* listener,
  void* data
);
void on_foreign_toplevel_handle_request_fullscreen(
  wl_listener* listener,
  void* data
);

// view/popup/callback.cpp
void on_new_popup(wl_listener* listener, void* data);
void on_popup_commit(wl_listener* listener, void* data);
void on_popup_destroy(wl_listener* listener, void* data);

// view/layer_shell/callback.cpp
void on_layer_shell_new_surface(wl_listener* listener, void* data);
void on_layer_shell_surface_commit(wl_listener* listener, void* data);
void on_layer_shell_surface_destroy(wl_listener* listener, void* data);
void on_layer_shell_surface_map(wl_listener* listener, void* data);
void on_layer_shell_surface_unmap(wl_listener* listener, void* data);

// view/session_lock/callback.cpp
void on_new_session_lock(wl_listener* listener, void* data);
void on_session_lock_unlock(wl_listener* listener, void* data);
void on_session_lock_destroy(wl_listener* listener, void* data);
void on_session_lock_new_surface(wl_listener* listener, void* data);
void on_session_lock_surface_destroy(wl_listener* listener, void* data);

// seat/text_input/callback.cpp
void on_new_text_input(wl_listener* listener, void* data);
void on_text_input_enable(wl_listener* listener, void* data);
void on_text_input_disable(wl_listener* listener, void* data);
void on_text_input_commit(wl_listener* listener, void* data);
void on_text_input_destroy(wl_listener* listener, void* data);

void on_new_input_method(wl_listener* listener, void* data);
void on_input_method_destroy(wl_listener* listener, void* data);
void on_input_method_commit(wl_listener* listener, void* data);
void on_input_method_grab_keyboard(wl_listener* listener, void* data);
void on_input_method_grab_keyboard_destroy(wl_listener* listener, void* data);
void on_input_method_new_popup_surface(wl_listener* listener, void* data);
void on_input_method_popup_surface_destroy(wl_listener* listener, void* data);
void on_input_method_popup_surface_map(wl_listener* listener, void* data);
void on_input_method_popup_surface_unmap(wl_listener* listener, void* data);
void on_input_method_popup_surface_commit(wl_listener* listener, void* data);

// view/surface.cpp
void on_new_surface(wl_listener* listener, void* data);
void on_surface_commit(wl_listener* listener, void* data);
void on_surface_destroy(wl_listener* listener, void* data);

} // namespace ura
