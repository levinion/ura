local M = {}

function M.setup()
  local function apply(win)
    local window_geo = win:geometry()
    if not window_geo or window_geo.width <= 0 or window_geo.height <= 0 then
      return
    end
    local output = win:output()
    if not output then
      return
    end
    local geo = output:logical_geometry()
    if not geo or geo.width <= 0 or geo.height <= 0 then
      return
    end
    win:resize(geo.width, geo.height)
    win:move(geo.x, geo.y)
  end

  local function reset_fullscreen(w)
    local output = w:output()
    if not output then
      return
    end
    local wins = ura.class.UraWindow:from_tags(output:tags())
    for _, win in ipairs(wins) do
      if win ~= w and win:output() == output and win:layout() == "fullscreen" then
        win:toggle_layout("fullscreen")
      end
    end
  end

  ura.hook.add("window-new", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if win:layout() == "fullscreen" then
      win:set_fullscreen(true)
      apply(win)
      reset_fullscreen(win)
    elseif ura.api.is_window_fullscreen_requested(e.id) then
      win:set_layout("fullscreen")
    end
  end, { ns = "layout.fullscreen", priority = ura.g.priority.instant })

  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "fullscreen" then
      win:set_z_index(ura.g.layer.fullscreen)
      win:set_fullscreen(true)
      win:update_userdata(function(t)
        t.focus_exclusive = true
      end)
      apply(win)
      reset_fullscreen(win)
    elseif e.from == "fullscreen" then
      win:set_fullscreen(false)
      win:update_userdata(function(t)
        t.focus_exclusive = nil
      end)
    end
  end, { ns = "layout.fullscreen" })

  ura.hook.add("window-map", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if win:layout() == "fullscreen" then
      apply(win)
    end
  end, { ns = "layout.fullscreen" })

  ura.hook.add("output-usable-geometry-change", function(e)
    local output = ura.class.UraOutput:new(e.id)
    local wins = ura.class.UraWindow:from_tags(output:tags())
    for _, win in ipairs(wins) do
      if win:output() == output and win:layout() == "fullscreen" then
        apply(win)
      end
    end
  end, { ns = "layout.fullscreen" })

  ura.hook.add("window-request-fullscreen", function(e)
    local win = ura.class.UraWindow:new(e.id)
    local req = e.requested_fullscreen
    if req == nil then
      req = e.fullscreen
    end

    if req == nil then
      win:toggle_layout("fullscreen")
      return
    end

    local is_fs = win:layout() == "fullscreen"
    if req and not is_fs then
      win:set_layout("fullscreen")
    elseif req and is_fs then
      apply(win)
      reset_fullscreen(win)
    elseif not req and is_fs then
      win:toggle_layout("fullscreen")
    end
  end, { ns = "layout.fullscreen" })
end

return M
