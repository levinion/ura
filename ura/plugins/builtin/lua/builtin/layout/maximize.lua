local M = {}

function M.setup()
  local function apply(win)
    local geo = win:output():usable_geometry()
    assert(geo)
    win:resize(geo.width, geo.height, { duration = 50 })
    win:move(geo.x, geo.y, { duration = 50 })
  end

  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "maximize" then
      win:set_z_index(ura.g.layer.normal)
      apply(win)
    end
  end, { ns = "layout.maximize" })

  ura.hook.add("output-usable-geometry-change", function(_)
    local wins = ura.class.UraWindow:from_tags(ura.class.UraOutput:current():tags())
    for _, win in ipairs(wins) do
      if win:layout() == "maximize" then
        apply(win)
      end
    end
  end, { ns = "layout.maximize" })

  ura.hook.add("window-request-maximize", function(e)
    ura.class.UraWindow:new(e.id):toggle_layout("maximize")
  end, { ns = "layout.maximize" })
end

return M
