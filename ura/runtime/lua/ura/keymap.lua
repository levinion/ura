local M = {}

M._keymaps = {}

local KEYMAPS = {}
local mode = "normal"

---@param pattern string
---@param f fun()
---@param opt table|nil
function M.set(pattern, f, opt)
  local id = ura.api.get_keybinding_id(pattern)
  assert(id)
  local m = opt and opt.mode or "normal"
  if KEYMAPS[id] == nil then
    KEYMAPS[id] = {}
  end
  if KEYMAPS[id][m] == nil then
    KEYMAPS[id][m] = f
  end
  if M._keymaps[id] == nil then
    M._keymaps[id] = function()
      KEYMAPS[id][mode]()
    end
  end
end

---@param pattern string
---@param opt table|nil
function M.unset(pattern, opt)
  local id = ura.api.get_keybinding_id(pattern)
  assert(id)
  local m = (opt and opt.mode) or "normal"
  KEYMAPS[id][m] = nil
  if #KEYMAPS[id] == 0 then
    KEYMAPS[id] = nil
    M._keymaps[id] = nil
  end
end

---@param m string
function M.set_mode(m)
  mode = m
end

function M._reset()
  M._keymaps = {}
  KEYMAPS = {}
end

return M
