local M = {}

setmetatable(M, {
  __index = function(_, key)
    return ura.api.get_option(key)
  end,
  __newindex = function(_, key, value)
    assert(type(key) == "string")
    return ura.api.set_option(key, value)
  end,
})

M.focus_follow_mouse = true

return M
