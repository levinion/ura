local M = {}

function M.setup()
  local function apply(win)
    local geo = win:output():logical_geometry()
    assert(geo)
    win:resize(geo.width, geo.height, { duration = 50 })
    win:move(geo.x, geo.y, { duration = 50 })
  end

  local function reset_fullscreen(w)
    local wins = ura.class.UraWindow:from_tags(ura.class.UraOutput:current():tags())
    for _, win in ipairs(wins) do
      if win ~= w and win:layout() == "fullscreen" then
        win:toggle_layout("fullscreen")
      end
    end
  end

  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "fullscreen" then
      win:set_z_index(ura.g.layer.fullscreen)
      win:set_fullscreen(true)
      apply(win)
      reset_fullscreen(win)
    elseif e.from == "fullscreen" then
      win:set_fullscreen(false)
    end
  end, { ns = "layout.fullscreen" })

  ura.hook.add("output-usable-geometry-change", function(_)
    local wins = ura.class.UraWindow:from_tags(ura.class.UraOutput:current():tags())
    for _, win in ipairs(wins) do
      if win:layout() == "fullscreen" then
        apply(win)
      end
    end
  end, { ns = "layout.fullscreen" })

  ura.hook.add("window-request-fullscreen", function(e)
    ura.class.UraWindow:new(e.id):toggle_layout("fullscreen")
  end, { ns = "layout.fullscreen" })
end

return M
