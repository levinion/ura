local M = {}

---@param id integer
function M.close(id)
	ura.api.close_window(id)
end

return M
