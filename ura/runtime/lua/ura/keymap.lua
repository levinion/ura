local M = {}

---@param pattern string
---@param f fun()
---@param opt table|nil
function M.set(pattern, f, opt)
  local mode = (opt and opt.mode) or "normal"
  ura.api.set_keymap(pattern, mode, f)
end

---@param pattern string
---@param opt table|nil
function M.unset(pattern, opt)
  local mode = (opt and opt.mode) or "normal"
  ura.api.unset_keymap(pattern, mode)
end

return M
