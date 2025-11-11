local M = {}

function M.setup()
  ura.layout.register("floating", {
    enter = function(win)
      ura.api.set_window_draggable(win, true)
      ura.api.set_window_z_index(win, 150)
      -- recover window size
      local userdata = ura.api.get_userdata(win)
      assert(userdata)
      assert(ura.fn.validate(userdata, "floating", "table"))
      if userdata.floating.width and userdata.floating.height then
        ura.api.resize_window(win, userdata.floating.width, userdata.floating.height)
      end
      if userdata.floating.x and userdata.floating.y then
        ura.api.move_window(win, userdata.floating.x, userdata.floating.y)
      end
    end,
    leave = function(win)
      local geo = ura.api.get_window_geometry(win)
      assert(geo)
      ura.fn.update_userdata(win, { floating = geo })
    end,
  })

  ura.hook.set("window-new", function(e)
    local win = e.id
    assert(win)
    local geo = ura.api.get_window_geometry(win)
    assert(geo)
    ura.fn.update_userdata(win, { floating = geo })
  end, {
    ns = "layout.floating",
    priority = 0,
    desc = "preserve initial geometry",
  })
end

return M
