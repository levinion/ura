local M = {}

function M.setup()
  ura.layout.register("floating", {
    ---@param win UraWindow
    enter = function(win)
      win:set_draggable(true)
      win:set_z_index(150)
      -- recover window size
      local userdata = win:userdata()
      assert(userdata)
      assert(ura.fn.validate(userdata, "floating", "table"))
      if userdata.floating.width and userdata.floating.height then
        win:resize(userdata.floating.width, userdata.floating.height)
      end
      if userdata.floating.x and userdata.floating.y then
        win:move(userdata.floating.x, userdata.floating.y)
      end
    end,
    ---@param win UraWindow
    leave = function(win)
      local geo = win:geometry()
      assert(geo)
      win:update_userdata({ floating = geo })
    end,
  })

  ura.hook.set("window-new", function(e)
    local win = ura.class.UraWindow:new(e.id)
    assert(win)
    local geo = win:geometry()
    assert(geo)
    win:update_userdata({ floating = geo })
  end, {
    ns = "layout.floating",
    priority = 0,
    desc = "preserve initial geometry",
  })
end

return M
