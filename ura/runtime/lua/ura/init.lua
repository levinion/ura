ura.opt = require("ura.opt")
ura.fn = require("ura.fn")
ura.hook = require("ura.hook")
ura.keymap = require("ura.keymap")
ura.class = require("ura.class")
ura.cmd = require("ura.cmd")

ura.fn.load_dir("/usr/share/ura/plugins")
ura.fn.load_dir("~/.local/share/ura/plugins")
ura.fn.load("~/.config/ura")
ura.fn.load("$XDG_CONFIG_HOME/ura")

ura.fn._save_context()

local status, v = ura.fn._load_config()
if not status then
  error(v)
else
  --[[@diagnostic disable-next-line: param-type-mismatch]]
  local status2, err = ura.fn._safe_call(v)
  if not status2 then
    error(err)
  end
end
