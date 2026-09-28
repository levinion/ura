local M = {}

---@param opt ?table
function M.setup(opt)
  local function apply(win)
    local output = win:output()
    if not output then
      return
    end
    local geo = output:usable_geometry()
    if not geo or geo.width <= 0 or geo.height <= 0 then
      return
    end
    local outer_r = opt and opt.outer_r or 10
    local outer_l = opt and opt.outer_l or 10
    local outer_t = opt and opt.outer_t or 10
    local outer_b = opt and opt.outer_b or 10
    win:resize(geo.width - (outer_l + outer_r), geo.height - (outer_t + outer_b))
    win:move(geo.x + outer_l, geo.y + outer_t)
  end

  ura.hook.add("window-new", function(e)
    if ura.api.is_window_maximized_requested(e.id) then
      ura.class.UraWindow:new(e.id):set_layout("maximize")
    end
  end, { ns = "layout.maximize", priority = ura.g.priority.instant })

  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "maximize" then
      win:set_z_index(ura.g.layer.normal)
      win:set_maximized(true)
      win:update_userdata(function(t)
        t.focus_exclusive = true
      end)
      apply(win)
    elseif e.from == "maximize" then
      win:set_maximized(false)
      win:update_userdata(function(t)
        t.focus_exclusive = nil
      end)
    end
  end, { ns = "layout.maximize" })

  ura.hook.add("window-map", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if win:layout() == "maximize" then
      apply(win)
    end
  end, { ns = "layout.maximize" })

  ura.hook.add("output-usable-geometry-change", function(e)
    local output = ura.class.UraOutput:new(e.id)
    local wins = ura.class.UraWindow:from_tags(output:tags())
    for _, win in ipairs(wins) do
      if win:output() == output and win:layout() == "maximize" then
        apply(win)
      end
    end
  end, { ns = "layout.maximize" })

  ura.hook.add("window-request-maximize", function(e)
    ura.class.UraWindow:new(e.id):toggle_layout("maximize")
  end, { ns = "layout.maximize" })
end

return M
