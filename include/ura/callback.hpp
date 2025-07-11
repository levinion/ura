#include "ura/ura.hpp"

namespace ura {

void on_new_output(wl_listener* listener, void* data);
void on_new_toplevel(wl_listener* listener, void* data);
void on_new_popup(wl_listener* listener, void* data);
void on_new_input(wl_listener* listener, void* data);

void on_seat_request_cursor(wl_listener* listener, void* data);
void on_seat_request_set_selection(wl_listener* listener, void* data);

void on_cursor_motion(wl_listener* listener, void* data);
void on_cursor_motion_absolute(wl_listener* listener, void* data);
void on_cursor_button(wl_listener* listener, void* data);
void on_cursor_axis(wl_listener* listener, void* data);
void on_cursor_frame(wl_listener* listener, void* data);

void on_toplevel_map(wl_listener* listener, void* data);
void on_toplevel_unmap(wl_listener* listener, void* data);
void on_toplevel_commit(wl_listener* listener, void* data);
void on_toplevel_destroy(wl_listener* listener, void* data);
void on_toplevel_request_move(wl_listener* listener, void* data);
void on_toplevel_request_resize(wl_listener* listener, void* data);
void on_toplevel_request_maximize(wl_listener* listener, void* data);
void on_toplevel_request_fullscreen(wl_listener* listener, void* data);
} // namespace ura
