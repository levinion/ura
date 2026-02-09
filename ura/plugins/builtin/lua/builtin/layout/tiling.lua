local M = {}

local function horizontal(opt)
  ura.layout.register("tiling", {
    ---@param win UraWindow
    enter = function(win)
      win:set_draggable(false)
      win:set_z_index(100)
    end,
    ---@param win UraWindow
    apply = function(win)
      local output = win:output()
      assert(output)
      local usable = output:usable_geometry()
      assert(usable)
      local windows = ura.class.UraWindow:from_tags(output:tags())
      assert(windows)

      local tiling_windows = {}

      for _, w in ipairs(windows) do
        if w:layout() == "tiling" then
          table.insert(tiling_windows, w)
        end
      end

      local sum = #tiling_windows
      if sum == 0 then
        return
      end

      local index = -1
      for i, w in ipairs(tiling_windows) do
        if w == win then
          index = i - 1
          break
        end
      end

      assert(index ~= -1)

      local outer_r = opt and opt.outer_r or 10
      local outer_l = opt and opt.outer_l or 10
      local outer_t = opt and opt.outer_t or 10
      local outer_b = opt and opt.outer_b or 10
      local inner = opt and opt.inner or 10

      local gaps = sum - 1
      local w = (usable.width - (outer_r + outer_l) - inner * gaps) / sum
      local h = usable.height - (outer_t + outer_b)
      local x = usable.x + outer_l + (w + inner) * index
      local y = usable.y + outer_t
      win:resize(w, h)
      win:move(x, y)
    end,
  })
end

---@param opt table?
function M.setup(opt)
  horizontal(opt)

  ura.hook.set("window-new", function(e)
    local win = ura.class.UraWindow:new(e.id)
    assert(win)
    if win:layout() ~= nil then
      return
    end
    win:set_layout("tiling")
  end, { ns = "layout.tiling", priority = 100, desc = "set tiling as the fallback layout" })

  local function apply_layouts()
    local tags = ura.class.UraOutput:current():tags()
    local windows = ura.class.UraWindow:from_tags(tags)
    for _, win in ipairs(windows) do
      win:apply_layout()
    end
  end

  for _, name in ipairs({ "window-resize", "window-close", "output-tags-change", "window-tags-change" }) do
    ura.hook.set(name, function()
      apply_layouts()
    end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout" })
  end

  ura.hook.set("output-usable-geometry-change", function(_)
    apply_layouts()
  end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })
end

return M
