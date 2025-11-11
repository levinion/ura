local M = {}

function M.setup()
  ura.layout.register("fullscreen", {
    enter = function(win)
      ura.api.set_window_draggable(win, false)
      ura.api.set_window_z_index(win, 250)
      ura.api.set_window_fullscreen(win, true)
    end,
    apply = function(win)
      local output = ura.api.get_window_output(win)
      assert(output)
      local geo = ura.api.get_output_logical_geometry(output)
      assert(geo)
      ura.api.resize_window(win, geo.width, geo.height)
      ura.api.move_window(win, geo.x, geo.y)
    end,
    leave = function(win)
      ura.api.set_window_fullscreen(win, false)
    end,
  })

  ura.hook.set("output-usable-geometry-change", function(_)
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })

  ura.hook.set("window-request-fullscreen", function(e)
    ura.layout.set(e.id, "fullscreen")
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })
end

return M
