local output = ura.output.get_current()
if not output then return end
if arg[1] ~= "-t" then
  ura.output.set_dpms(output.name, false)
else
  ura.output.set_dpms(output.name, true)
end
