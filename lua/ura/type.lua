---@meta

--- @class ura: table
--- The top-level Ura application API.
ura = {
  --- Core application API functions.
  --- @class ura.api: table
  api = {
    --- Terminates the application.
    terminate = function() end,
    --- Reloads the application configuration and state.
    reload = function() end,
  },

  --- Window management functions.
  --- @class ura.win: table
  win = {
    --- Focuses on a window.
    --- @param index integer The index of the window to focus.
    --- @return boolean True if the window was successfully focused, false otherwise.
    focus = function(index) end,
    --- Closes the currently focused window.
    close = function() end,
    --- Sets the floating state of the focused window.
    --- @param is_floating boolean True to make the window float, false otherwise.
    set_floating = function(is_floating) end,
    --- @return boolean
    is_floating = function() end,
    --- Sets the fullscreen state of the focused window.
    --- @param is_fullscreen boolean True to make the window fullscreen, false otherwise.
    set_fullscreen = function(is_fullscreen) end,
    --- @return boolean
    is_fullscreen = function() end,
    --- Moves the focused window to a specified workspace.
    --- @param index integer The index of the target workspace (0-based).
    move_to_workspace = function(index) end,
    --- Gets the index of the current top-level window.
    --- @return integer The index of the current top-level window.
    get_index = function() end,
  },

  --- Input device management.
  --- @class ura.input: table
  input = {
    --- @class ura.input.keyboard: table
    keyboard = {
      --- Sets the keyboard repeat rate and delay.
      --- @param rate integer The repeat rate (keys per second).
      --- @param delay number The initial delay before repeating (in milliseconds).
      set_repeat = function(rate, delay) end,
    },
    --- @class ura.input.cursor: table
    cursor = {
      --- Sets whether the focus follows the mouse cursor.
      --- @type boolean
      focus_follow_mouse = true,
      --- Sets the cursor theme and size.
      --- @param theme string The name of the cursor theme.
      --- @param size integer The size of the cursor.
      set_theme = function(theme, size) end,
      --- Sets the visibility of the mouse cursor.
      --- @param visible boolean True to show, false to hide.
      set_visible = function(visible) end,
      --- Moves the cursor to an absolute position on the screen.
      --- @param x number The absolute X coordinate.
      --- @param y number The absolute Y coordinate.
      absolute_move = function(x, y) end,
      --- Moves the cursor by a relative amount.
      --- @param x number The amount to move horizontally.
      --- @param y number The amount to move vertically.
      relative_move = function(x, y) end,
      --- Sets the shape of the mouse cursor.
      --- @param name string The name of the cursor shape (e.g., "arrow", "hand").
      set_shape = function(name) end,
    },
  },

  --- Workspace management functions.
  --- @class ura.ws: table
  ws = {
    --- Switches to a specified workspace.
    --- @param index integer The index of the workspace to switch to (0-based).
    --- @return number The index of the newly switched workspace.
    switch = function(index) end,
    --- Gets the index of the current workspace.
    --- Workspace index starts with 0.
    --- @return integer The index of the current workspace.
    get_index = function() end,
  },

  --- Layout management functions.
  --- @class ura.layout: table
  layout = {
    --- Configuration for tiling layouts.
    --- @class ura.layout.tilling: table
    tilling = {
      --- Gap settings for tiling layouts.
      --- @class ura.layout.tilling.gap: table
      gap = {
        --- The outer gaps.
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
        --- The inner gap between tiled windows.
        --- @type number
        inner = 10,
      },
    },
    --- Configuration for floating layouts.
    --- @class ura.layout.floating: table
    floating = {
      --- Default size for new floating windows.
      --- @class ura.layout.floating.default: table
      default = {
        --- The default width for floating windows.
        --- @type integer
        width = 800,
        --- The default height for floating windows.
        --- @type integer
        height = 600,
      },
    },
  },

  --- Keymap functions.
  --- @class ura.keymap: table
  keymap = {
    --- Registers a keybinding.
    --- @param modifiers string A string representing modifier keys (e.g., "Mod4", "Shift").
    --- @param key string The key to bind (e.g., "Return", "q").
    --- @param f fun() The function to execute when the keybinding is pressed.
    set = function(modifiers, key, f) end,
  },

  --- Hook functions.
  --- @class ura.hook: table
  hook = {
    --- Registers a hook.
    --- @param name string
    --- @param f fun()
    set = function(name, f) end,
  },

  --- General utility functions.
  --- @class ura.fn: table
  fn = {
    --- Sets an environment variable.
    --- @param name string The name of the environment variable.
    --- @param value string The value to set for the environment variable.
    set_env = function(name, value) end,
  },

  --- Global variables.
  --- @class ura.g: table
  g = {
    --- @type table<string, fun()> A table for storing user-defined hooks.
    hooks = {},
    --- @type table<integer, fun()> A table for storing user-defined keymaps by ID.
    keymaps = {},
  },
}
