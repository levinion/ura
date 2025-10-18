--- @meta

--- @class ura.cmd
ura.cmd = {
  --- @param layout string The layout name to switch to (e.g., "tiling", "floating").
  toggle_layout = function(layout) end,

  --- Closes the currently focused window.
  close = function() end,

  --- Moves focus to the previous window in the current workspace's list.
  focus_prev = function() end,

  --- Moves focus to the next window in the current workspace's list.
  focus_next = function() end,

  --- Swaps the current window's position with the previous window in the list.
  swap_prev = function() end,

  --- Swaps the current window's position with the next window in the list.
  swap_next = function() end,

  --- Switches to the previous workspace. Also destroys the current workspace.
  switch_prev = function() end,

  --- Switches to the next workspace, creating a new one if necessary.
  switch_next = function() end,

  move_to_prev = function() end,
  move_to_next = function() end,
}
