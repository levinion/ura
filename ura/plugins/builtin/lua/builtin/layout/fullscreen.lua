local M = {}

function M.setup()
  ura.layout.register("fullscreen", {
    ---@param win UraWindow
    enter = function(win)
      win:set_draggable(false)
      win:set_z_index(250)
      win:set_fullscreen(true)
    end,
    ---@param win UraWindow
    apply = function(win)
      local output = win:output()
      assert(output)
      local geo = output:logical_geometry()
      assert(geo)
      win:resize(geo.width, geo.height)
      win:move(geo.x, geo.y)
    end,
    ---@param win UraWindow
    leave = function(win)
      win:set_fullscreen(false)
    end,
  })

  local function apply_layouts()
    local tags = ura.class.UraOutput:current():tags()
    local windows = ura.fn.get_windows_by_tags(tags)
    for _, win in ipairs(windows) do
      win:apply_layout()
    end
  end

  ura.hook.set("output-usable-geometry-change", function(_)
    apply_layouts()
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })

  ura.hook.set("window-request-fullscreen", function(e)
    ura.class.UraWindow:new(e.id):toggle_layout("fullscreen")
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })
end

return M
