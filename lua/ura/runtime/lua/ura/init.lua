ura.opt = require("ura.opt")
ura.fn = require("ura.fn")
ura.hook = require("ura.hook")
ura.keymap = require("ura.keymap")
ura.layout = require("ura.layout")
ura.win = require("ura.win")
ura.cmd = require("ura.cmd")

local output = ura.fn.shell("find ~/.local/share/ura/ -maxdepth 1")
local plugins = ura.fn.split(output, "\n")
for _, plugin in ipairs(plugins) do
  ura.api.prepend_package_path(plugin .. "/lua/?/init.lua")
  ura.api.prepend_package_path(plugin .. "/lua/?.lua")
end

ura.api.prepend_package_path(ura.api.expanduser("~/.config/ura/lua/?/init.lua"))
ura.api.prepend_package_path(ura.api.expanduser("~/.config/ura/lua/?.lua"))
ura.api.prepend_package_path(ura.api.expandvars("$XDG_CONFIG_HOME/ura/lua/?/init.lua"))
ura.api.prepend_package_path(ura.api.expandvars("$XDG_CONFIG_HOME/ura/lua/?.lua"))
