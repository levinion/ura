---@meta

--- @class UraWindow
local UraWindow = {
	--- @type integer
	id = 0,
	--- @type integer
	workspace_id = 0,
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
	z_id = 0,
	--- @type boolean
	pinned = false,
}

--- @class UraWorkspace
local UraWorkspace = {
	--- @type integer
	id = 0,
	--- @type string|nil
	name = "",
	--- @type UraWindow[]
	windows = {},
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
		workspaces = {},
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
		height = 0,
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
	scale = 0,
}

--- @class UraCursorTheme
local UraCursorTheme = {
	--- @type string|nil
	theme = "",
	--- @type integer
	size = 0,
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
		-- base

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
		set_idle_inhibitor = function(flag) end,

		-- window

		--- @param id integer
		focus_window = function(id) end,
		--- @param id integer
		close_window = function(id) end,
		--- @param window_id integer
		--- @param workspace_id integer
		move_window_to_workspace = function(window_id, workspace_id) end,
		--- @return integer
		get_window_number = function() end,
		--- @return integer|nil
		get_current_window = function() end,
		--- @param index integer
		--- @return integer|nil
		get_window = function(index) end,
		--- @param workspace_id integer
		--- @param window_id integer
		activate_window = function(workspace_id, window_id) end,
		---@param id integer
		---@param x integer
		---@param y integer
		move_window = function(id, x, y) end,
		---@param id integer
		---@param width integer
		---@param height integer
		resize_window = function(id, width, height) end,
		---@param id integer
		---@param layout string
		set_window_layout = function(id, layout) end,
		---@param id integer
		---@param z_index integer
		set_window_z_index = function(id, z_index) end,
		---@param id integer
		---@param flag boolean
		set_window_draggable = function(id, flag) end,
		---@param src integer
		---@param dst integer
		swap_window = function(src, dst) end,
		---@param id integer
		redraw_window = function(id) end,

		-- input

		--- @param rate integer The repeat rate (keys per second).
		--- @param delay number The initial delay before repeating (in milliseconds).
		set_keyboard_repeat = function(rate, delay) end,
		--- @param theme UraCursorTheme
		set_cursor_theme = function(theme) end,
		--- @param visible boolean True to show, false to hide.
		set_cursor_visible = function(visible) end,
		--- @return boolean
		is_cursor_visible = function(visible) end,
		--- @param name string The name of the cursor shape (e.g., "arrow", "hand").
		set_cursor_shape = function(name) end,
		---@param pattern string
		---@param properties UraPointerProperties
		set_pointer_properties = function(pattern, properties) end,

		-- ws

		create_workspace = function() end,
		--- @param id integer The index of the workspace to switch to (0-based).
		switch_workspace = function(id) end,
		--- @param id integer The 0-based index of the workspace to switch to. If the index is greater than the current number of workspaces, new ones will be created until this index is available.
		switch_or_create_workspace = function(id) end,
		--- @return integer The size of workspaces in current output.
		get_workspace_number = function() end,
		--- @param id integer The index of the workspace to destroy (0-based).
		destroy_workspace = function(id) end,
		--- @return integer|nil
		get_current_workspace = function() end,
		--- @param id integer
		--- @return integer|nil
		get_workspace = function(id) end,
		--- @return UraWorkspace[]
		list_workspaces = function() end,
		redraw_workspace = function() end,

		-- output

		--- @return integer|nil
		get_current_output = function() end,
		--- @return UraOutput|nil
		--- @param name string
		get_output = function(name) end,
		--- @param id integer
		--- @param flag boolean
		set_output_dpms = function(id, flag) end,

		-- layout

		--- @param name string
		--- @param f fun(integer)
		set_layout = function(name, f) end,
		--- @param name string
		unset_layout = function(name) end,

		-- keymap

		--- wrapper of ura.keymap.set_mode, equals to ura.keymap.set_mode("normal",...)
		--- @param pattern string A string representing key (e.g. "super+shift+p")
		--- @param mode string
		--- @param f fun() The function to execute when the keybinding is pressed.
		set_keymap = function(pattern, mode, f) end,
		--- wrapper of ura.keymap.unset_mode, equals to ura.keymap.unset_mode("normal",...)
		--- @param pattern string
		--- @param opt table
		unset = function(pattern, opt) end,
		--- @param mode string
		enter_mode = function(mode) end,
		--- @return string
		get_current_mode = function() end,

		-- hook

		--- @param name string
		--- @param f function
		set_hook = function(name, f) end,
		--- @param name string
		unset_hook = function(name) end,
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
	g = {},
}
