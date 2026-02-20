local M = {}

---@param t table
local function ReadOnly(t)
  return setmetatable({}, {
    __index = t,
    __newindex = function(_, key, _)
      error("Attempt to modify read-only table: " .. tostring(key), 2)
    end,
    __metatable = false,
  })
end

local G = {
  priority = {
    instant = 0,
    urgent = 10,
    ultra_fast = 20,
    very_fast = 30,
    fast = 40,
    normal = 50,
    slow = 60,
    very_slow = 70,
    ultra_slow = 80,
    slowest = 90,
  },
  layer = {
    clear = -50,
    background = 0,
    bottom = 50,
    normal = 100,
    top = 150,
    floating = 200,
    fullscreen = 250,
    popup = 300,
    overlay = 350,
    lock_screen = 400,
  },
}

M.priority = ReadOnly(G.priority)
M.layer = ReadOnly(G.layer)
return ReadOnly(M)
