local M = {}

function M.setup(opt)
  opt = opt or {}

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
