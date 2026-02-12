local M = {}

function M.setup()
  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "floating" then
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
    elseif e.from == "floating" then
      local geo = win:geometry()
      assert(geo)
      win:update_userdata({ floating = geo })
    end
  end, { priority = 0 })

  ura.hook.add("window-new", function(e)
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
