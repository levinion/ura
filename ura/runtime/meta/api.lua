---@meta

--- @class ura.api
ura.api = {
  -- base
  terminate = function() end,
  --- @param cmd string
  spawn = function(cmd) end,
  --- @param summary string
  --- @param body string
  notify = function(summary, body) end,
  --- @param f function
  --- @param timeout number Milliseconds
  --- @return integer|nil
  set_timeout = function(f, timeout) end,
  --- @param fd integer
  clear_timeout = function(fd) end,
  --- @param f function
  --- @param interval number Milliseconds
  --- @return integer|nil
  set_interval = function(f, interval) end,
  --- @param fd integer
  clear_interval = function(fd) end,

  -- idle
  notify_idle_activity = function() end,
  --- @param flag boolean
  set_idle_inhibitor = function(flag) end,
  --- @param name string
  --- @param seconds number
  --- @param on_idle function|nil
  --- @param on_resume function|nil
  add_idle_timeout = function(name, seconds, on_idle, on_resume) end,
  --- @param name string
  remove_idle_timeout = function(name) end,
  clear_idle_timeouts = function() end,
  --- @return boolean
  is_idle = function() end,

  -- win
  --- @param id integer
  close_window = function(id) end,
  --- @return integer|nil
  get_current_window = function() end,
  --- @param id integer
  --- @return integer|nil
  get_window_output = function(id) end,
  --- @param id integer
  focus_window = function(id) end,

  --- @param id integer
  --- @param z integer
  set_window_z_index = function(id, z) end,
  --- @param id integer
  --- @return integer|nil
  get_window_z_index = function(id) end,
  --- @param id integer
  --- @return string|nil
  get_window_app_id = function(id) end,
  --- @param id integer
  --- @return string|nil
  get_window_title = function(id) end,
  --- @param id integer
  --- @return integer|nil
  get_window_pid = function(id) end,
  --- @param id integer
  --- @return string|nil
  get_window_icon_name = function(id) end,
  --- @param id integer
  --- @return table|nil
  get_window_icon = function(id) end,
  --- @param id integer
  activate_window = function(id) end,
  --- @param id integer
  --- @param x integer
  --- @param y integer
  move_window = function(id, x, y) end,
  --- @param id integer
  --- @param x integer
  --- @param y integer
  --- @param duration number Milliseconds
  --- @param easing string|table
  animate_window_move = function(id, x, y, duration, easing) end,
  --- @param id integer
  --- @param width integer
  --- @param height integer
  resize_window = function(id, width, height) end,
  --- @param id integer
  --- @param width integer
  --- @param height integer
  --- @param duration number Milliseconds
  --- @param easing string|table
  animate_window_resize = function(id, width, height, duration, easing) end,
  --- @param id integer
  --- @param degrees number
  --- @param duration number Milliseconds
  --- @param easing string|table
  animate_window_rotation = function(id, degrees, duration, easing) end,
  --- @param id integer
  --- @return number|nil
  get_window_rotation = function(id) end,
  --- @param id integer
  --- @param opacity number
  --- @param duration number Milliseconds
  --- @param easing string|table
  animate_window_opacity = function(id, opacity, duration, easing) end,
  --- @param id integer
  --- @param duration number Milliseconds
  --- @param easing string|table
  --- @param options table|nil
  animate_window_open = function(id, duration, easing, options) end,
  --- @param id integer
  --- @param duration number Milliseconds
  --- @param easing string|table
  --- @param options table|nil
  animate_window_close = function(id, duration, easing, options) end,
  --- @param id integer
  --- @param flag boolean
  set_window_fullscreen = function(id, flag) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_fullscreen = function(id) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_fullscreen_requested = function(id) end,
  --- @param id integer
  --- @param flag boolean
  set_window_resizing = function(id, flag) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_resizing = function(id) end,
  --- @param id integer
  --- @param flag boolean
  set_window_maximized = function(id, flag) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_maximized = function(id) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_maximized_requested = function(id) end,
  --- @param id integer
  --- @return table|nil
  get_window_geometry = function(id) end,
  --- @param id integer
  --- @return table|nil
  get_window_visual_geometry = function(id) end,
  --- @param id integer
  --- @param tags table<string>
  set_window_tags = function(id, tags) end,
  --- @param id integer
  --- @return table<string>|nil
  get_window_tags = function(id) end,
  --- @return table<integer>
  get_all_windows = function() end,
  --- @param id integer
  --- @return boolean|nil
  is_window_mapped = function(id) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_focused = function(id) end,
  --- @param id integer
  --- @return integer|nil
  get_window_lru = function(id) end,
  --- @param id integer
  --- @param opacity number
  set_window_opacity = function(id, opacity) end,
  --- @param id integer
  --- @return number|nil
  get_window_opacity = function(id) end,
  --- @param id integer
  --- @param color string
  set_window_border_color = function(id, color) end,
  --- @param id integer
  --- @return string|nil
  get_window_border_color = function(id) end,
  --- @param spec table
  --- @return { id: integer, generation: integer, name: string }|nil
  define_effect = function(spec) end,
  --- @param id integer
  --- @param generation integer
  --- @return boolean
  find_effect = function(name) end,
  undefine_effect = function(id, generation) end,
  clear_effects = function() end,
  migrate_effects = function() end,
  --- @param id integer
  --- @param effect_id integer
  --- @param generation integer
  --- @param uniforms table
  --- @param options table
  --- @return boolean
  set_window_effect_instance = function(id, effect_id, generation, uniforms, options) end,
  --- @param id integer
  --- @param name string
  --- @param from number|table|boolean
  --- @param to number|table|boolean
  --- @param duration number Milliseconds
  --- @param easing string|table
  animate_window_effect_uniform = function(id, name, from, to, duration, easing) end,
  --- @param id integer
  clear_window_effect = function(id) end,

  -- input
  --- @param rate integer
  --- @param delay integer
  set_keyboard_repeat = function(rate, delay) end,
  --- @param theme string
  --- @param size integer
  set_cursor_theme = function(theme, size) end,
  --- @param flag boolean
  set_cursor_visible = function(flag) end,
  --- @return boolean
  get_cursor_visible = function() end,
  --- @param name string
  set_cursor_shape = function(name) end,
  --- @return {x: number, y: number}
  get_cursor_pos = function() end,

  -- output
  --- @return integer|nil
  get_current_output = function() end,
  --- @param name string
  --- @return integer|nil
  get_output = function(name) end,
  --- @return table<integer>
  get_all_outputs = function() end,
  --- @param id integer
  --- @return string|nil
  get_output_name = function(id) end,
  --- @param id integer
  --- @return table|nil
  get_output_logical_geometry = function(id) end,
  --- @param id integer
  --- @return table|nil
  get_output_usable_geometry = function(id) end,
  --- @param id integer
  --- @return number|nil
  get_output_scale = function(id) end,
  --- @param id integer
  --- @param tags table<string>
  set_output_tags = function(id, tags) end,
  --- @param id integer
  --- @param flag boolean
  set_output_dpms = function(id, flag) end,
  --- @param id integer
  --- @return table<string>|nil
  get_output_tags = function(id) end,
  --- @param name string
  --- @return { name: string, namespace: string, index: integer }|nil
  get_tag = function(name) end,
  --- @return table<{ name: string, namespace: string, index: integer }>
  get_tags = function() end,
  --- @return table<string>
  get_namespaces = function() end,
  --- @param name_space string
  --- @return { name: string, namespace: string, index: integer }|nil
  get_active_tag = function(name_space) end,
  --- @return { name: string, namespace: string, index: integer }|nil
  get_current_tag = function() end,
  --- @param name string
  --- @return boolean
  set_current_tag = function(name) end,
  --- @param delta integer
  --- @return boolean
  shift_tag_index = function(delta) end,
  --- @param index integer
  --- @return boolean
  set_tag_index = function(index) end,
  --- @param delta integer
  --- @param update_output boolean
  --- @return boolean
  shift_window_tag_index = function(delta, update_output) end,
  --- @param delta integer
  --- @return boolean
  switch_tag_namespace = function(delta) end,

  -- fn
  --- @param name string
  --- @param value string
  set_env = function(name, value) end,
  --- @param name string
  unset_env = function(name) end,
  --- @param path string
  append_package_path = function(path) end,
  --- @param path string
  prepend_package_path = function(path) end,
  --- @param path string
  --- @return string
  expanduser = function(path) end,
  --- @param path string
  --- @return string
  expandvars = function(path) end,
  --- @param path string
  --- @return string
  expand = function(path) end,
  --- @param value any
  --- @return string
  to_json = function(value) end,
  --- @param str string
  --- @return any
  parse_json = function(str) end,

  -- userdata
  ---@param id integer
  ---@param value any
  set_userdata = function(id, value) end,
  ---@param id integer
  ---@return any
  get_userdata = function(id) end,

  -- util
  ---@param pattern string
  ---@return integer|nil
  get_keybinding_id = function(pattern) end,
  ---@return integer
  time_since_epoch = function() end,
}
