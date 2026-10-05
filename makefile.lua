   function writemakefile_psp(prj_name, ...)
      local args = {...}
      local project = "source/" .. prj_name .. "/"
      local sources = { os.isfile(project .. "main.cpp") and "main.cpp" or "main.c" }
      for i, v in ipairs( args ) do
          table.insert(sources, v)
      end
      for _, helper in ipairs({"injector", "log", "patterns", "rini", "inireader", "gvm", "mips", "memalloc"}) do
         table.insert(sources, "../../external/injector/include/psp/" .. helper .. ".c")
      end
      local quoted = {}
      for _, source in ipairs(sources) do table.insert(quoted, string.format("%q", source)) end
      if not os.isfile(project .. "module.json") then
         io.writefile(project .. "module.json", '{\n  "sources": [' .. table.concat(quoted, ", ") .. '],\n  "output": "../../data/' .. prj_name .. '/memstick/PSP/PLUGINS/' .. prj_name .. '/' .. prj_name .. '.prx",\n  "exports": "exports.exp",\n  "startup": "module_start",\n  "defines": ["PSP_HOOK_RAM_END=0x0DD00000"],\n  "libraries": ["-lpspsystemctrl_user", "-lm"]\n}\n')
      end
      io.writefile(project .. "makefile", [[CONFIGURATION ?= Release
.PHONY: all clean
all:
	powershell -NoProfile -ExecutionPolicy Bypass -File "../../external/pspsdk/plugins/build-module.ps1" -Project module.json -Configuration "$(CONFIGURATION)"
clean:
	powershell -NoProfile -ExecutionPolicy Bypass -File "../../external/pspsdk/plugins/build-module.ps1" -Project module.json -Configuration "$(CONFIGURATION)" -Clean
]])
   end

function writemakefile_ps2(prj_name, scripts_addr, libs, ...)
   local project = "source/" .. prj_name .. "/"
   local sources = { os.isfile(project .. "main.cpp") and "main.cpp" or "main.c" }
   for _, object in ipairs({...}) do
      local source = object:gsub("%.o$", ".c")
      local cpp = source:gsub("%.c$", ".cpp")
      table.insert(sources, os.isfile(project .. cpp) and cpp or source)
   end
   local quoted = {}
   for _, source in ipairs(sources) do table.insert(quoted, string.format("%q", source)) end
   if not os.isfile(project .. "module.json") then
      io.writefile(project .. "module.json", '{\n  "sources": [' .. table.concat(quoted, ", ") .. '],\n  "output": "../../data/' .. prj_name .. '/' .. scripts_addr .. prj_name .. '.elf"\n}\n')
   end
   io.writefile(project .. "makefile", [[.PHONY: all clean
all:
	powershell -NoProfile -ExecutionPolicy Bypass -File "../../external/ps2sdk/plugins/build-module.ps1" -Project module.json
clean:
	powershell -NoProfile -ExecutionPolicy Bypass -File "../../external/ps2sdk/plugins/build-module.ps1" -Project module.json -Clean
]])
end

-- Kept as a no-op while callers migrate. ET_REL needs no fixed-address script.
function writelinkfile_ps2(prj_name) end
