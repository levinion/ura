local M = {}

---@param opt ?table
function M.setup(opt)
  opt = opt or {}

  ---@class UraWindow
  ---@field layout fun(self: UraWindow): string|nil
  ---@return string|nil
  function ura.class.UraWindow:layout()
    local userdata = self:userdata()
    return userdata and userdata.layout or nil
  end

  ---@class UraWindow
  ---@field set_layout fun(self: UraWindow, layout: string)
  ---@param layout string
  function ura.class.UraWindow:set_layout(layout)
    local old_layout = self:layout()
    if old_layout == layout then
      return
    end
    self:update_userdata(function(t)
      t.layout = layout
    end)
    ura.hook.emit("window-layout-change", {
      id = self.id,
      from = old_layout,
      to = layout,
    })
  end

  ---@class UraWindow
  ---@field toggle_layout fun(self: UraWindow, layout: string)
  ---@param layout string
  function ura.class.UraWindow:toggle_layout(layout)
    local old = self:layout()
    if old ~= layout then
      self:set_layout(layout)
    else
      self:set_layout(ura.opt["default_layout"] or "tiling")
    end
  end

  if opt.floating ~= false then
    require("builtin.layout.floating").setup()
  end

  if opt.fullscreen ~= false then
    require("builtin.layout.fullscreen").setup()
  end

  if opt.tiling ~= false then
    require("builtin.layout.tiling").setup(opt.tiling)
  end

  if opt.maximize ~= false then
    require("builtin.layout.maximize").setup(opt.maximize)
  end
end

return M
