-- This configuration file is provided as an example only. It may not work on your system.
-- Please make sure you understand each part before using it. Do not run any code you're unfamiliar with.
-- Have a great day!

ura.fn.prepend_package_path(ura.fn.expanduser("~/.config/ura/lua/?.lua"))
require("keymap")
require("hook")
require("option")
require("layout")
