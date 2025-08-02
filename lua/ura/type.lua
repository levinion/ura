---@meta

--- @class UraWindow: table
local UraWindow = {
  --- @type integer
  index = 0,
  --- @type integer
  workspace_index = 0,
  --- @type string
  app_id = "",
  --- @type string
  title = "",
  --- @type boolean
  floating = false,
  --- @type boolean
  fullscreen = false,
}

--- @class UraWorkspace: table
local UraWorkspace = {
  --- @type integer
  index = 0,
  --- @type UraWindow[]
  windows = {}
}

--- @class ura: table
ura = {
  --- @class ura.api: table
  api = {
    terminate = function() end,
    reload = function() end,
  },

  --- @class ura.win: table
  win = {
    --- @param index integer
    --- @return boolean
    focus = function(index) end,
    --- @param index integer
    close = function(index) end,
    --- @param index integer
    --- @param flag boolean
    set_floating = function(index, flag) end,
    --- @param index integer
    --- @param flag boolean
    set_fullscreen = function(index, flag) end,
    --- @param window_index integer
    --- @param workspace_index integer
    move_to_workspace = function(window_index, workspace_index) end,
    --- @return integer
    size = function() end,
    --- @return UraWindow|nil
    get_current = function() end,
    --- @return UraWindow|nil
    get = function(index) end,
    --- @param workspace_index integer
    --- @param window_index integer
    activate = function(workspace_index, window_index) end
  },

  --- @class ura.input: table
  input = {
    --- @class ura.input.keyboard: table
    keyboard = {
      --- @param rate integer The repeat rate (keys per second).
      --- @param delay number The initial delay before repeating (in milliseconds).
      set_repeat = function(rate, delay) end,
    },
    --- @class ura.input.cursor: table
    cursor = {
      --- @param theme string The name of the cursor theme.
      --- @param size integer The size of the cursor.
      set_theme = function(theme, size) end,
      --- @param visible boolean True to show, false to hide.
      set_visible = function(visible) end,
      --- @return boolean
      is_visible = function(visible) end,
      --- @param x number The absolute X coordinate.
      --- @param y number The absolute Y coordinate.
      absolute_move = function(x, y) end,
      --- @param x number The amount to move horizontally.
      --- @param y number The amount to move vertically.
      relative_move = function(x, y) end,
      --- @param name string The name of the cursor shape (e.g., "arrow", "hand").
      set_shape = function(name) end,
    },
  },

  --- @class ura.ws: table
  ws = {
    --- @param index integer The index of the workspace to switch to (0-based).
    switch = function(index) end,
    --- @return integer The size of workspaces in current output.
    size = function() end,
    --- @param index integer The index of the workspace to destroy (0-based).
    destroy = function(index) end,
    --- @return UraWorkspace
    get_current = function() end,
    --- @return UraWorkspace|nil
    get = function(index) end,
  },

  --- @class ura.layout: table
  layout = {
    --- @class ura.layout.tilling: table
    tilling = {
      --- @class ura.layout.tilling.gap: table
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
    --- @class ura.layout.floating: table
    floating = {
      --- @class ura.layout.floating.default: table
      default = {
        --- @type integer
        width = 800,
        --- @type integer
        height = 600,
      },
    },
  },

  --- @class ura.keymap: table
  keymap = {
    --- @param pattern string A string representing key (e.g. "super+shift+p")
    --- @param f fun() The function to execute when the keybinding is pressed.
    set = function(pattern, f) end,
  },

  --- @class ura.hook: table
  hook = {
    --- @param name string
    --- @param f fun()
    set = function(name, f) end,
  },

  --- @class ura.fn: table
  fn = {
    --- @param name string The name of the environment variable.
    --- @param value string The value to set for the environment variable.
    set_env = function(name, value) end,
  },

  --- @class ura.opt: table
  opt = {
    --- @type string
    active_border_color = "#89b4fa",
    --- @type string
    inactive_border_color = "#00000000",
    --- @type integer
    border_width = 1,
    --- @type boolean
    focus_follow_mouse = true,
  },

  --- @class ura.g: table
  g = {
    --- @type table<string, fun()> A table for storing user-defined hooks.
    hooks = {},
    --- @type table<integer, fun()> A table for storing user-defined keymaps by ID.
    keymaps = {},
  },
}
