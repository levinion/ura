local M = {}

---@param layout string
function M.toggle_layout(layout)
  local win = ura.api.get_current_window()
  assert(win)
  local old = ura.layout.get(win)
  if old ~= layout then
    ura.layout.set(win, layout)
  else
    ura.layout.set(win, ura.opt["layout.default"] or "tiling")
  end
end

function M.close()
  local win = ura.api.get_current_window()
  assert(win)
  ura.win.close(win)
end

function M.focus_prev()
  local win = ura.api.get_current_window()
  if win then
    assert(ura.layout.get(win) ~= "fullscreen")
    local index = ura.api.get_window_index(win)
    assert(index)
    local ws = ura.api.get_window_workspace(win)
    assert(ws)
    local target = ura.api.get_window(ws, index - 1)
    assert(target)
    ura.api.focus_window(target)
  else
    local ws = ura.api.get_current_workspace()
    assert(ws)
    local target = ura.api.get_window(ws, 0)
    assert(target)
    ura.api.focus_window(target)
  end
end

function M.focus_next()
  local win = ura.api.get_current_window()
  if win then
    assert(ura.layout.get(win) ~= "fullscreen")
    local index = ura.api.get_window_index(win)
    assert(index)
    local ws = ura.api.get_window_workspace(win)
    assert(ws)
    local target = ura.api.get_window(ws, index + 1)
    assert(target)
    ura.api.focus_window(target)
  else
    local ws = ura.api.get_current_workspace()
    assert(ws)
    local target = ura.api.get_window(ws, 0)
    assert(target)
    ura.api.focus_window(target)
  end
end

function M.swap_prev()
  local win = ura.api.get_current_window()
  assert(win)
  local index = ura.api.get_window_index(win)
  assert(index)
  local ws = ura.api.get_current_workspace()
  assert(ws)
  local prev = ura.api.get_window(ws, index - 1)
  assert(prev)
  ura.api.swap_window(win, prev)
end

function M.swap_next()
  local win = ura.api.get_current_window()
  assert(win)
  local index = ura.api.get_window_index(win)
  assert(index)
  local ws = ura.api.get_current_workspace()
  assert(ws)
  local next = ura.api.get_window(ws, index + 1)
  assert(next)
  ura.api.swap_window(win, next)
end

function M.switch_prev()
  local ws = ura.api.get_current_workspace()
  assert(ws)
  local index = ura.api.get_workspace_index(ws)
  assert(index)
  local output = ura.api.get_current_output()
  assert(output)
  local target = ura.api.get_indexed_workspace(output, index - 1)
  assert(target)
  ura.api.switch_workspace(target)
  ura.api.destroy_workspace(ws)
end

function M.switch_next()
  local ws = ura.api.get_current_workspace()
  assert(ws)
  local index = ura.api.get_workspace_index(ws)
  assert(index)
  local output = ura.api.get_current_output()
  assert(output)
  local workspaces = ura.api.get_indexed_workspaces(output)
  assert(workspaces)
  if index == #workspaces - 1 then
    ura.api.create_indexed_workspace()
  end
  local target = ura.api.get_indexed_workspace(output, index + 1)
  assert(target)
  ura.api.switch_workspace(target)
end

function M.move_to_prev()
  local ws = ura.api.get_current_workspace()
  assert(ws)
  local win = ura.api.get_current_window()
  assert(win)
  local index = ura.api.get_workspace_index(ws)
  local output = ura.api.get_current_output()
  assert(output)
  local target = ura.api.get_indexed_workspace(output, index - 1)
  assert(target)
  ura.api.move_window_to_workspace(win, target)
  ura.api.switch_workspace(target)
  ura.api.destroy_workspace(ws)
end

function M.move_to_next()
  local ws = ura.api.get_current_workspace()
  assert(ws)
  local win = ura.api.get_current_window()
  assert(win)
  local index = ura.api.get_workspace_index(ws)
  assert(index)
  local output = ura.api.get_current_output()
  assert(output)
  local workspaces = ura.api.get_indexed_workspaces(output)
  assert(workspaces)
  if index == #workspaces - 1 then
    ura.api.create_indexed_workspace()
  end
  local target = ura.api.get_indexed_workspace(output, index + 1)
  assert(target)
  ura.api.move_window_to_workspace(win, target)
  ura.api.switch_workspace(target)
end

return M
