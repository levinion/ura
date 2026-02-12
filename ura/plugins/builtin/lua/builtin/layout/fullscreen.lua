local M = {}

function M.setup()
  local function apply(win)
    local geo = win:output():logical_geometry()
    assert(geo)
    win:resize(geo.width, geo.height)
    win:move(geo.x, geo.y)
  end

  local function apply_all()
    local wins = ura.class.UraWindow:from_tags(ura.class.UraOutput:current():tags())
    for _, win in ipairs(wins) do
      if win:layout() == "fullscreen" then
        apply(win)
      end
    end
  end

  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "fullscreen" then
      win:set_draggable(false)
      win:set_z_index(250)
      win:set_fullscreen(true)
      apply(win)
    elseif e.from == "fullscreen" then
      win:set_fullscreen(false)
    end
  end)

  ura.hook.add("output-usable-geometry-change", function(_)
    apply_all()
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })

  ura.hook.add("window-request-fullscreen", function(e)
    ura.class.UraWindow:new(e.id):toggle_layout("fullscreen")
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })
end

return M
