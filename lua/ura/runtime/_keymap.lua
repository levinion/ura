local M = {}

---@param pattern string
---@param f fun()
---@param opt table
function M.set(pattern, f, opt)
	local mode = (opt and opt.mode) or "normal"
	ura.api.set_keymap(pattern, mode, f)
end

return M
