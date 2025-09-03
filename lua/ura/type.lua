---@meta

--- @class UraWindow
local UraWindow = {
  --- @type integer
  index = 0,
  --- @type integer
  workspace_index = 0,
  --- @type string
  app_id = "",
  --- @type string
  title = "",
  --- @type integer
  x = 0,
  --- @type integer
  y = 0,
  --- @type integer
  width = 0,
  --- @type integer
  height = 0,
  --- @type string
  layout = "tiling",
  --- @type string|nil
  last_layout = nil,
  --- @type boolean
  first_commit_after_layout_change = true,
  --- @type integer
  z_index = 0
}

--- @class UraWorkspace
local UraWorkspace = {
  --- @type integer
  index = 0,
  --- @type string|nil
  name = "",
  --- @type UraWindow[]
  windows = {}
}

--- @class UraOutput
local UraOutput = {
  --- @type string
  name = "",
  --- @class UraOutput.size
  size = {
    --- @type number
    x = 0,
    --- @type number
    y = 0,
    --- @type number
    width = 0,
    --- @type number
    height = 0,
    --- @type UraWorkspace[]
    workspaces = {}
  },
  --- @class UraOutput.usable
  usable = {
    --- @type number
    x = 0,
    --- @type number
    y = 0,
    --- @type number
    width = 0,
    --- @type number
    height = 0
  },
  --- @type number
  scale = 0,
  --- @type number
  refresh = 0,
  --- @type boolean
  dpms = true,
}

--- @class UraOutputMode
local UraOutputMode = {
  --- @type integer|nil
  width = 0,
  --- @type integer|nil
  height = 0,
  --- @type number|nil
  refresh = 0,
  --- @type number|nil
  scale = 0
}

--- @class UraCursorTheme
local UraCursorTheme = {
  --- @type string|nil
  theme = "",
  --- @type integer
  size = 0
}

--- @class UraPointerProperties
local UraPointerProfile = {
  ---@type "adaptive"|"flat"
  accel_profile = "adaptive",
  ---@type number
  move_speed = 1,
  ---@type number
  scroll_speed = 1,
}

--- @class ura
ura = {
  --- @class ura.api
  api = {
    terminate = function() end,
    reload = function() end,
    --- @param cmd string
    spawn = function(cmd) end,
    --- @param f function
    --- @param milliseconds integer
    schedule = function(f, milliseconds) end,
    --- @param summary string
    --- @param body string
    notify = function(summary, body) end,
    notify_idle_activity = function() end,
    --- @param flag boolean
    set_idle_inhibitor = function(flag) end
  },

  --- @class ura.win
  win = {
    --- @param index integer
    focus = function(index) end,
    --- @param index integer
    close = function(index) end,
    --- @param window_index integer
    --- @param workspace_id integer|string
    move_to_workspace = function(window_index, workspace_id) end,
    --- @param window_index integer
    --- @param workspace_index integer The 0-based index of the workspace to move to. If the index is greater than the current number of workspaces, new ones will be created until this index is available.
    move_to_workspace_or_create = function(window_index, workspace_index) end,
    --- @return integer
    size = function() end,
    --- @return UraWindow|nil
    get_current = function() end,
    --- @return UraWindow|nil
    get = function(index) end,
    --- @param workspace_id integer|string
    --- @param window_index integer
    activate = function(workspace_id, window_index) end,
    ---@param index integer
    ---@param x integer
    ---@param y integer
    move = function(index, x, y) end,
    ---@param index integer
    ---@param width integer
    ---@param height integer
    resize = function(index, width, height) end,
    ---@param index integer
    center = function(index) end,
    ---@param index integer
    ---@param layout string
    set_layout = function(index, layout) end,
    ---@param index integer
    ---@param z_index integer
    set_z_index = function(index, z_index) end,
    ---@param index integer
    ---@param flag boolean
    set_draggable = function(index, flag) end,
    ---@param src integer
    ---@param dst integer
    swap = function(src, dst) end,
    ---@param index integer
    redraw = function(index) end
  },

  --- @class ura.input
  input = {
    --- @class ura.input.keyboard
    keyboard = {
      --- @param rate integer The repeat rate (keys per second).
      --- @param delay number The initial delay before repeating (in milliseconds).
      set_repeat = function(rate, delay) end,
    },
    --- @class ura.input.cursor
    cursor = {
      --- @param theme UraCursorTheme
      set_theme = function(theme) end,
      --- @param visible boolean True to show, false to hide.
      set_visible = function(visible) end,
      --- @return boolean
      is_visible = function(visible) end,
      --- @param name string The name of the cursor shape (e.g., "arrow", "hand").
      set_shape = function(name) end,
    },
    --- @class ura.input.pointer
    pointer = {
      ---@param pattern string
      ---@param properties UraPointerProperties
      set_properties = function(pattern, properties) end,
    }
  },

  --- @class ura.ws
  ws = {
    create = function() end,
    --- @param index integer The index of the workspace to switch to (0-based).
    switch = function(index) end,
    --- @param index integer The 0-based index of the workspace to switch to. If the index is greater than the current number of workspaces, new ones will be created until this index is available.
    switch_or_create = function(index) end,
    --- @return integer The size of workspaces in current output.
    size = function() end,
    --- @param index integer The index of the workspace to destroy (0-based).
    destroy = function(index) end,
    --- @return UraWorkspace
    get_current = function() end,
    --- @param id integer|string
    --- @return UraWorkspace|nil
    get = function(id) end,
    --- @return UraWorkspace[]
    list = function() end,
    redraw = function() end,
  },

  --- @class ura.output
  output = {
    --- @return UraOutput|nil
    get_current = function() end,
    --- @return UraOutput|nil
    --- @param name string
    get = function(name) end,
    --- @param name string
    --- @param flag boolean
    set_dpms = function(name, flag) end,
    --- @param name string
    --- @param mode UraOutputMode
    set_mode = function(name, mode) end
  },

  --- @class ura.layout
  layout = {
    --- @param name string
    --- @param f fun(integer)
    set = function(name, f) end,
    --- @param name string
    unset = function(name) end,
  },

  --- @class ura.keymap
  keymap = {
    --- wrapper of ura.keymap.set_mode, equals to ura.keymap.set_mode("normal",...)
    --- @param pattern string A string representing key (e.g. "super+shift+p")
    --- @param f fun() The function to execute when the keybinding is pressed.
    set = function(pattern, f) end,
    --- @param mode string
    --- @param pattern string
    --- @param f fun()
    set_mode = function(mode, pattern, f) end,
    --- wrapper of ura.keymap.unset_mode, equals to ura.keymap.unset_mode("normal",...)
    --- @param pattern string
    unset = function(pattern) end,
    --- @param mode string
    --- @param pattern string
    unset_mode = function(mode, pattern) end,
    --- @param mode string
    enter_mode = function(mode) end,
    --- @return string
    get_current_mode = function() end
  },

  --- @class ura.hook
  hook = {
    --- @param name string
    --- @param f function
    set = function(name, f) end,
  },

  --- @class ura.fn
  fn = {
    --- @param name string The name of the environment variable.
    --- @param value string The value to set for the environment variable.
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
  },

  --- @class ura.opt
  opt = {
    --- @type string
    active_border_color = "#89b4fa",
    --- @type string
    inactive_border_color = "#00000000",
    --- @type integer
    border_width = 1,
    --- @type boolean
    focus_follow_mouse = true,
    --- @class ura.opt.tilling
    tilling = {
      --- @class ura.opt.tilling.gap
      gap = {
        outer = {
          --- @type number
          top = 10,
          --- @type number
          left = 10,
          --- @type number
          bottom = 10,
          --- @type number
          right = 10,
        },
        --- @type number
        inner = 10,
      },
    },
    --- @class ura.opt.device
    device = {
      --- @type table<string,UraOutputMode>
      outputs = {},
      --- @type table<string,UraPointerProperties>
      pointer_rules = {},
    },
  },
  --- @type table
  g = {}
}
