---@meta

--- @class ura.api
ura.api = {
  -- base
  terminate = function() end,
  reload = function() end,
  --- @param cmd string
  spawn = function(cmd) end,
  --- @param summary string
  --- @param body string
  notify = function(summary, body) end,
  --- @param f function
  --- @param time integer Milliseconds
  schedule = function(f, time) end,

  -- hook
  --- @param name string
  --- @param f function
  set_hook = function(name, f) end,
  --- @param name string
  unset_hook = function(name) end,

  -- idle
  notify_idle_activity = function() end,
  --- @param flag boolean
  set_idle_inhibitor = function(flag) end,

  -- keymap
  --- @param pattern string
  --- @param mode string
  --- @param f function
  set_keymap = function(pattern, mode, f) end,
  --- @param pattern string
  --- @param mode string
  unset_keymap = function(pattern, mode) end,

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

  --- Clear = -50,
  --- Background = 0,
  --- Bottom = 50,
  --- Normal = 100,
  --- Floating = 150,
  --- Top = 200,
  --- Fullscreen = 250,
  --- Popup = 300,
  --- Overlay = 350,
  --- LockScreen = 400
  --- @param id integer
  --- @param z integer
  set_window_z_index = function(id, z) end,
  --- @param id integer
  --- @param flag boolean
  set_window_draggable = function(id, flag) end,
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
  --- @return boolean|nil
  is_window_draggable = function(id) end,
  --- @param id integer
  activate_window = function(id) end,
  --- @param id integer
  --- @param x integer
  --- @param y integer
  move_window = function(id, x, y) end,
  --- @param id integer
  --- @param width integer
  --- @param height integer
  resize_window = function(id, width, height) end,
  --- @param id integer
  --- @param flag boolean
  set_window_fullscreen = function(id, flag) end,
  --- @param id integer
  --- @return boolean|nil
  is_window_fullscreen = function(id) end,
  --- @param id any
  --- @return table|nil
  get_window_geometry = function(id) end,
  --- @param id integer
  --- @param tags table<string>
  set_window_tags = function(id, tags) end,
  --- @param id integer
  get_window_tags = function(id) end,
  --- @return table<integer>
  get_all_windows = function() end,

  -- input
  --- @param rate integer
  --- @param delay integer
  set_keyboard_repeat = function(rate, delay) end,
  --- @param profile string
  --- @param glob string
  set_pointer_accel_profile = function(profile, glob) end,
  --- @param speed number
  --- @param glob string
  set_pointer_move_speed = function(speed, glob) end,
  --- @param speed number
  --- @param glob string
  set_pointer_scroll_speed = function(speed, glob) end,
  --- @param theme string
  --- @param size integer
  set_cursor_theme = function(theme, size) end,
  --- @return string
  get_cursor_theme = function() end,
  --- @return integer
  get_cursor_size = function() end,
  --- @param flag boolean
  set_cursor_visible = function(flag) end,
  --- @return boolean
  is_cursor_visible = function() end,
  --- @param name string
  set_cursor_shape = function(name) end,
  --- @return string
  get_cursor_shape = function() end,

  -- output
  --- @return integer|nil
  get_current_output = function() end,
  --- @param name string
  --- @return integer|nil
  get_output = function(name) end,
  --- @return string|nil
  get_output_name = function() end,
  --- @param id integer
  --- @param flag boolean
  set_output_dpms = function(id, flag) end,
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
  --- @param tags table< string>
  set_output_tags = function(id, tags) end,
  --- @param id integer
  --- @return table<string>
  get_output_tags = function(id) end,

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

  -- opt
  ---@param key string
  ---@param value any
  set_option = function(key, value) end,
  ---@param key string
  ---@return any
  get_option = function(key) end,
  ---@param id integer
  ---@param value any
  set_userdata = function(id, value) end,
  ---@param id integer
  ---@return any
  get_userdata = function(id) end,
}
