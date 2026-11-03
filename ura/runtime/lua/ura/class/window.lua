--- @class UraWindow
--- @field id integer
local UraWindow = {}
UraWindow.__index = UraWindow

function UraWindow.__eq(a, b)
  if rawequal(a, b) then
    return true
  end
  if type(a) == "table" and type(b) == "table" then
    return a.id == b.id
  end
  return false
end

--- @param id integer
--- @return UraWindow
function UraWindow:new(id)
  local instance = setmetatable({}, UraWindow)
  instance.id = id
  return instance
end
--- Resolve animation policy through the optional animation plugin. Without
--- the plugin only explicit per-call duration enables animation.
local function resolve_animation(name, event, opt)
  local spec
  if ura.animation and ura.animation.resolve then
    spec = ura.animation.resolve(name, event, false)
  else
    spec = { enabled = false }
  end

  if not spec.enabled and opt and opt.duration ~= nil then
    spec.enabled = opt.duration > 0
    spec.duration = opt.duration
    spec.easing = opt.easing or spec.easing or "in_out_cubic"
  end

  spec.duration = math.max(0, tonumber(spec.duration) or 0)
  spec.easing = spec.easing or "in_out_cubic"
  return spec
end


--- @return UraWindow|nil
function UraWindow:current()
  local id = ura.api.get_current_window()
  return id and UraWindow:new(id) or nil
end

--- @return table<UraWindow>
function UraWindow:all()
  local wins = ura.api.get_all_windows()
  local t = {}
  for _, win in ipairs(wins) do
    table.insert(t, ura.class.UraWindow:new(win))
  end
  return t
end

---@param tags table<string>
---@return table<UraWindow>
function UraWindow:from_tags(tags)
  local wins = self:all()
  local t = {}
  for _, win in ipairs(wins) do
    local contains = false
    for _, tag in ipairs(tags) do
      if ura.fn.find(win:tags(), function(v)
        return v == tag
      end) then
        contains = true
        break
      end
    end
    if contains then
      table.insert(t, win)
    end
  end
  return t
end

---@param x integer
---@param y integer
---@return table<UraWindow>
function UraWindow:from_pos(x, y)
  local wins = self:all()
  local t = {}
  for _, win in ipairs(wins) do
    local geo = win:geometry()
    if win:is_mapped() and geo.x <= x and geo.x + geo.width >= x and geo.y <= y and geo.y + geo.height >= y then
      table.insert(t, win)
    end
  end
  return t
end

function UraWindow:close()
  ura.api.close_window(self.id)
end

function UraWindow:focus()
  ura.api.focus_window(self.id)
end

function UraWindow:activate()
  ura.api.activate_window(self.id)
end

---@return UraOutput|nil
function UraWindow:output()
  local id = ura.api.get_window_output(self.id)
  return id and ura.class.UraOutput:new(id) or nil
end

---@return string|nil
function UraWindow:app_id()
  return ura.api.get_window_app_id(self.id)
end

---@return string|nil
function UraWindow:title()
  return ura.api.get_window_title(self.id)
end

---@return table|nil
function UraWindow:geometry()
  return ura.api.get_window_geometry(self.id)
end

---@return table|nil
function UraWindow:visual_geometry()
  return ura.api.get_window_visual_geometry(self.id)
end

---@return integer|nil
function UraWindow:z_index()
  return ura.api.get_window_z_index(self.id)
end

--- @param z integer
function UraWindow:set_z_index(z)
  ura.api.set_window_z_index(self.id, z)
end

---@return boolean|nil
function UraWindow:is_fullscreen()
  return ura.api.is_window_fullscreen(self.id)
end

---@return boolean|nil
function UraWindow:is_fullscreen_requested()
  return ura.api.is_window_fullscreen_requested(self.id)
end

--- @param flag boolean
function UraWindow:set_fullscreen(flag)
  ura.api.set_window_fullscreen(self.id, flag)
end

---@return boolean|nil
function UraWindow:is_resizing()
  return ura.api.is_window_resizing(self.id)
end

--- @param flag boolean
function UraWindow:set_resizing(flag)
  ura.api.set_window_resizing(self.id, flag)
end

---@return boolean|nil
function UraWindow:is_maximized()
  return ura.api.is_window_maximized(self.id)
end

---@return boolean|nil
function UraWindow:is_maximized_requested()
  return ura.api.is_window_maximized_requested(self.id)
end

--- @param flag boolean
function UraWindow:set_maximized(flag)
  ura.api.set_window_maximized(self.id, flag)
end

---@return boolean|nil
function UraWindow:is_mapped()
  return ura.api.is_window_mapped(self.id)
end

---@return boolean|nil
function UraWindow:is_focused()
  return ura.api.is_window_focused(self.id)
end

---@return number|nil
function UraWindow:rotation()
  return ura.api.get_window_rotation(self.id)
end

---@param degrees number
---@param opt { duration?: integer, fps?: number, easing?: string|fun(t: number): number }|nil
function UraWindow:set_rotation(degrees, opt)
  local current = self:rotation() or 0
  local spec = resolve_animation("window-rotation", {
    id = self.id,
    app_id = self:app_id(),
    title = self:title(),
    from = current,
    to = degrees,
    opt = opt,
  }, opt)
  if not spec.enabled or (spec.duration or 0) <= 0 then
    ura.api.animate_window_rotation(self.id, degrees, 0, spec.easing)
    return
  end
  ura.api.animate_window_rotation(self.id, degrees, spec.duration, spec.easing)
end

---@return number|nil
function UraWindow:opacity()
  return ura.api.get_window_opacity(self.id)
end

---@param opacity number
--- @param opt { duration?: integer, fps?: number, easing?: string|fun(t: number): number }|nil
function UraWindow:set_opacity(opacity, opt)
  local start_opacity = ura.api.get_window_opacity(self.id)
  if not start_opacity or start_opacity == opacity then
    return
  end

  local spec = resolve_animation("window-opacity", {
    id = self.id,
    app_id = self:app_id(),
    title = self:title(),
    from = start_opacity,
    to = opacity,
    opt = opt,
  }, opt)

  if not spec.enabled or (spec.duration or 0) <= 0 then
    ura.api.set_window_opacity(self.id, opacity)
    return
  end

  ura.api.animate_window_opacity(self.id, opacity, spec.duration, spec.easing)
end

---@param effect table
---@param uniforms table|nil
---@param options table|nil
function UraWindow:set_effect(effect, uniforms, options)
  if type(effect) ~= "table" or not effect.id then
    error("set_effect expects a pipeline returned by ura.render.effect")
  end
  local ok = ura.api.set_window_effect_instance(
    self.id,
    effect.id,
    effect.generation or 1,
    uniforms or {},
    options or {}
  )
  if not ok then
    error("failed to apply effect")
  end
end

---@param name string
---@param from number|table|boolean
---@param to number|table|boolean
---@param opt { duration?: integer, easing?: string|fun(t: number): number }|nil
function UraWindow:animate_effect_uniform(name, from, to, opt)
  opt = opt or {}
  ura.api.animate_window_effect_uniform(
    self.id,
    name,
    from,
    to,
    opt.duration or 200,
    opt.easing or "in_out_cubic"
  )
end

function UraWindow:clear_effect()
  ura.api.clear_window_effect(self.id)
end
---@param uniforms table
---@param opt { duration?: integer, easing?: string|fun(t: number): number }|nil
function UraWindow:animate_effect_uniforms(uniforms, opt)
  opt = opt or {}
  for name, spec in pairs(uniforms or {}) do
    if type(spec) == "table" and spec.from ~= nil and spec.to ~= nil then
      self:animate_effect_uniform(name, spec.from, spec.to, opt)
    end
  end
end


---@return string|nil
function UraWindow:border_color()
  return ura.api.get_window_border_color(self.id)
end

---@param color string
function UraWindow:set_border_color(color)
  ura.api.set_window_border_color(self.id, color)
end

--- @param x integer
--- @param y integer
--- @param opt { duration?: integer, fps?: number, easing?: string|fun(t: number): number }|nil
function UraWindow:move(x, y, opt)
  x = math.floor(x)
  y = math.floor(y)

  local geo = self:geometry()
  assert(geo)
  if geo.x == x and geo.y == y then
    return
  end

  local spec = resolve_animation("window-move", {
    id = self.id,
    app_id = self:app_id(),
    title = self:title(),
    from = geo,
    to = { x = x, y = y, width = geo.width, height = geo.height },
    opt = opt,
  }, opt)

  if not spec.enabled or (spec.duration or 0) <= 0 then
    ura.api.move_window(self.id, x, y)
    return
  end

  -- The compositor keeps the visual state and retargets from the current
  -- position if another layout change interrupts this animation.
  ura.api.animate_window_move(self.id, x, y, spec.duration, spec.easing)
end

--- @param width integer
--- @param height integer
--- @param opt { duration?: integer, fps?: number, easing?: string|fun(t: number): number }|nil
function UraWindow:resize(width, height, opt)
  width = math.floor(width)
  height = math.floor(height)

  local geo = self:geometry()
  assert(geo)
  if geo.width == width and geo.height == height then
    return
  end

  local event = {
    id = self.id,
    app_id = self:app_id(),
    title = self:title(),
    from = geo,
    to = { x = geo.x, y = geo.y, width = width, height = height },
    opt = opt,
  }

  local spec = resolve_animation("window-resize", event, opt)

  -- A layout move usually also changes size. If the user only configured
  -- window-move, let that rule cover the combined resize+move transition.
  if not spec.enabled and not (opt and opt.duration) then
    spec = resolve_animation("window-move", event, nil)
  end

  if not spec.enabled or (spec.duration or 0) <= 0 then
    -- Still commit-first: the compositor waits for the client's final content
    -- and snaps the SceneNode when it arrives.
    ura.api.resize_window(self.id, width, height)
    return
  end

  -- One configure is sent for the final size. The compositor waits for the
  -- client to commit that size, then runs a FLIP animation from the old
  -- visible rectangle to the new one.
  ura.api.animate_window_resize(self.id, width, height, spec.duration, spec.easing)
end

--- @param opt { duration?: integer, fps?: number }|nil
function UraWindow:center(opt)
  local geo = self:geometry()
  assert(geo)
  local o_geo = self:output():usable_geometry()
  assert(o_geo)
  local target_x = o_geo.x + math.floor((o_geo.width - geo.width) / 2)
  local target_y = o_geo.y + math.floor((o_geo.height - geo.height) / 2)
  self:move(target_x, target_y, opt)
end

---@return integer|nil
function UraWindow:lru()
  return ura.api.get_window_lru(self.id)
end

--- @param tags table<string>
function UraWindow:set_tags(tags)
  tags = ura.fn.natural_sort(tags)
  ura.api.set_window_tags(self.id, tags)
end

---@return table<string>
function UraWindow:tags()
  return ura.api.get_window_tags(self.id) or {}
end

---@return table
function UraWindow:userdata()
  return ura.api.get_userdata(self.id) or {}
end

---@param tbl table
function UraWindow:set_userdata(tbl)
  ura.api.set_userdata(self.id, tbl)
end

---@param f fun(t: table)
function UraWindow:update_userdata(f)
  local userdata = self:userdata() or {}
  f(userdata)
  self:set_userdata(userdata)
end

return UraWindow
