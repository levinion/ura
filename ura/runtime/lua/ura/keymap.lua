local M = {}

M._keymaps = {}
M._KEYMAPS = {}
local mode = "normal"

---@param patterns table<string>
---@param f fun():boolean|nil
---@param opt table|nil
function M.set(patterns, f, opt)
  local m = opt and opt.mode or "normal"
  for _, pattern in ipairs(patterns) do
    local id = ura.api.get_keybinding_id(pattern)
    if id then
      if M._KEYMAPS[id] == nil then
        M._KEYMAPS[id] = {}
      end
      if M._KEYMAPS[id][m] == nil then
        M._KEYMAPS[id][m] = f
      end
      if M._keymaps[id] == nil then
        M._keymaps[id] = function()
          M._KEYMAPS[id][mode]()
        end
      end
    end
  end
end

---@param patterns table<string>
---@param opt table|nil
function M.unset(patterns, opt)
  local m = (opt and opt.mode) or "normal"
  for _, pattern in ipairs(patterns) do
    local id = ura.api.get_keybinding_id(pattern)
    if id then
      M._KEYMAPS[id][m] = nil
      if #M._KEYMAPS[id] == 0 then
        M._KEYMAPS[id] = nil
        M._keymaps[id] = nil
      end
    end
  end
end

---@param m string
function M.set_mode(m)
  mode = m
end

return M
