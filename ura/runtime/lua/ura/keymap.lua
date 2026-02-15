local M = {}

M._keymaps = {}

local keymaps = {}
local mode = "normal"

---@param pattern string
---@param f fun()
---@param opt table|nil
function M.set(pattern, f, opt)
  local id = ura.api.get_keybinding_id(pattern)
  assert(id)
  local m = opt and opt.mode or "normal"
  if keymaps[id] == nil then
    keymaps[id] = {}
  end
  if keymaps[id][m] == nil then
    keymaps[id][m] = f
  end
  if M._keymaps[id] == nil then
    M._keymaps[id] = function()
      keymaps[id][mode]()
    end
  end
end

---@param pattern string
---@param opt table|nil
function M.unset(pattern, opt)
  local id = ura.api.get_keybinding_id(pattern)
  assert(id)
  local m = (opt and opt.mode) or "normal"
  keymaps[id][m] = nil
  if #keymaps[id] == 0 then
    keymaps[id] = nil
    M._keymaps[id] = nil
  end
end

---@param m string
function M.set_mode(m)
  mode = m
end

return M
