require "vstudio"

newoption {
   trigger = "no-shared-release-deps",
   description = "Compile common native Release dependencies separately for each plugin"
}

-- PS2 and PSP as Visual Studio platforms of their own (as premake-consoles does for
-- consoles); their projects are Makefile ones, so no MSBuild platform files are needed.
premake.vstudio.vs2010_architectures.ps2 = "PS2"
premake.vstudio.vs2010_architectures.psp = "PSP"
premake.api.addAllowed("system", { "ps2", "psp" })

-- The folder a project is deployed to, and the game it is started from when debugging,
-- is the path of one machine and does not belong in the repository. It is read from a
-- `.env` file next to this script, which is not tracked by git and holds one
-- `<KEY>=<folder>` line per game (quotes and a trailing slash are optional). A project
-- whose key is missing is not deployed at all.
local envkeys = nil
function envdir(key)
   if not envkeys then
      envkeys = {}
      local text = io.readfile(path.join(_SCRIPT_DIR, ".env")) or ""
      for line in text:gmatch("[^\r\n]+") do
         local k, v = line:match("^%s*([%w_]+)%s*=%s*(.-)%s*$")
         if k and v ~= "" then
            v = v:gsub('^"', ""):gsub('"$', ""):gsub("^'", ""):gsub("'$", "")
            envkeys[k] = v
         end
      end
   end

   local value = envkeys[key]
   if not value then return nil end

   value = value:gsub("[%s\\/]+$", "")
   if value == "" then return nil end

   return path.translate(value)
end

-- Deploys the built .asi into the script folder of the game that `key` names in the .env
-- file, and starts the game from there when debugging. Only a plugin that is already
-- installed in the game folder is replaced, a folder without one is left alone.
function setpaths(key, exepath, scriptspath)
   scriptspath = scriptspath or "scripts/"
   local gamepath = envdir(key)
   if gamepath then
      local target = gamepath .. "\\" .. path.translate(scriptspath)
      postbuildcommands {
         "if exist \"" .. target .. "$(TargetFileName)\" copy /y \"$(TargetPath)\" \"" .. target .. "\"",
      }
      debugdir (gamepath)
      if (exepath) then
         debugcommand (gamepath .. "\\" .. path.translate(exepath))
         local dir = exepath:match'(.*/)(.*)'
         debugdir (gamepath .. "\\" .. path.translate(dir or ""))
      end
   end
   targetdir ("data/%{prj.name}/" .. scriptspath)
end

-- Checks a settings description with the validator (AsiSettingsValidator). Only the Win32 solution builds it: in the
-- other solutions the check is skipped with a warning until the Win32 solution has built it once.
function settings_check(description, text)
   local exe = '%{wks.location}tools/asi-settings-validate.exe'
   return 'if exist "' .. exe .. '" ("' .. exe .. '" "' .. description .. '" "' .. text .. '" || exit /b 1) else ' ..
      'echo asi-settings-validate : warning : settings not checked, build AsiSettingsValidator in the Win32 solution first'
end

-- The settings description and every language of its text next to an emulator plugin, as
-- <plugin>.asisettings.json and <plugin>.asisettings.<lang>.json (the languages present at build time)
function settings_sidecars(scriptspath, prj_name, settingsText)
   local output = ('%{wks.location}../data/%{prj.name}/' .. scriptspath):gsub('/', '\\')
   local description = ('%{wks.location}../source/%{prj.name}/settings.json'):gsub('/', '\\')
   local text = ('%{wks.location}../text/' .. settingsText):gsub('/', '\\')
   return { 'copy /y "' .. description .. '" "' .. output .. prj_name .. '.asisettings.json" || exit /b 1',
      'for %%f in ("' .. text .. '\\*.json") do copy /y "%%f" "' .. output .. prj_name .. '.asisettings.%%~nf.json" || exit /b 1' }
end

function setbuildpaths_psp(key, exepath, scriptspath, pspsdkpath, sourcepath, prj_name, settingsText)
   local command = 'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}/../external/pspsdk/plugins/build-module.ps1" -Project "' .. sourcepath .. 'module.json" -Configuration "%{cfg.buildcfg}"'
   local validate = {}
   local sidecars = {}
   if settingsText then
      validate = { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/' .. settingsText .. '/en.json') }
      sidecars = settings_sidecars(scriptspath, prj_name, settingsText)
   end
   local gamepath = envdir(key)
   local deploy = {}
   if gamepath then
      local target = path.join(gamepath, "memstick/PSP/PLUGINS/", prj_name)
      deploy = { 'if /I "$(PostBuildEventUseInBuild)"=="false" exit /b 0', 'if not exist "' .. target .. '" mkdir "' .. target .. '"',
         'copy /y "$(NMakeOutput)" "' .. target .. '"' }
      debugdir(gamepath)
      debugcommand(path.join(gamepath, exepath))
   end
   buildcommands { validate, command .. ' || exit /b 1', sidecars, deploy }
   rebuildcommands { validate, command .. ' -Clean || exit /b 1', command .. ' || exit /b 1', sidecars, deploy }
   cleancommands { command .. ' -Clean' }
   targetdir("data/%{prj.name}/" .. scriptspath)
end

function setbuildpaths_ps2(key, exepath, scriptspath, ps2sdkpath, sourcepath, prj_name, settingsText)
   local command = 'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}/../external/ps2sdk/plugins/build-module.ps1" -Project "' .. sourcepath .. 'module.json"'
   local validate = {}
   local sidecars = {}
   if settingsText then
      validate = { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/' .. settingsText .. '/en.json') }
      sidecars = settings_sidecars(scriptspath, prj_name, settingsText)
   end
   local gamepath = envdir(key)
   local deploy = {}
   if gamepath then
      local target = path.join(gamepath, scriptspath)
      deploy = { 'if /I "$(PostBuildEventUseInBuild)"=="false" exit /b 0', 'if not exist "' .. target .. '" mkdir "' .. target .. '"',
         'copy /y "$(NMakeOutput)" "' .. target .. '"' }
      debugdir(gamepath)
      debugcommand(path.join(gamepath, os.isfile(path.join(gamepath, "pcsx2-qtx64.exe")) and "pcsx2-qtx64.exe" or "pcsx2-qt.exe"))
   end
   buildcommands { validate, command .. ' || exit /b 1', sidecars, deploy }
   rebuildcommands { validate, command .. ' -Clean || exit /b 1', command .. ' || exit /b 1', sidecars, deploy }
   cleancommands { command .. ' -Clean' }
   targetdir("data/%{prj.name}/" .. scriptspath)
end

function add_kananlib()
   defines { "BDDISASM_HAS_MEMSET", "BDDISASM_HAS_VSNPRINTF" }
   files { "external/injector/kananlib/include/utility/**.hpp", "external/injector/kananlib/src/**.cpp" }
   files { "external/injector/bddisasm/bddisasm/*.c" }
   files { "external/injector/bddisasm/bdshemu/*.c" }
   includedirs { "external/injector/kananlib/include" }
   includedirs { "external/injector/bddisasm/inc" }
   includedirs { "external/injector/bddisasm/bddisasm/include" }
end

-- Compiles shaders with the DirectX SDK tools shipped in tools/x86, one custom build step per file. Same idea
-- as Visual Studio's FxCompile build action, but with the June 2010 compiler, which is the one that still
-- handles the D3D9 era profiles (fx_2_0, ps_1_1 with /LD, ...). Unlike a prebuild command this only runs when
-- a shader actually changed, and the output is tracked by MSBuild.
-- Both the input and the output are passed as paths relative to the project file, as written in the project
-- and with the output next to the shader, where the .rc files expect it. Do not use %(FullPath)/%(Directory)
-- here: they expand to a drive-stripped absolute path, which makes the June 2010 compiler write the output to
-- a bogus "build/<source tree>" mirror and fails there on some machines (AppVeyor) instead of compiling.
-- Rules: { files = <file pattern>, ext = <output extension>, tool = "fxc" (default) | "asm_shader",
--          args = <tool arguments>, out = <optional output path, defaults to a sibling of the source> }
-- `args` and `out` may use the input file's metadata, written with a single % (e.g. %(Filename)).
function buildshaders(rules)
   for _, rule in ipairs(rules) do
      local tool = rule.tool or "fxc"
      local exe = "../tools/x86/" .. tool .. ".exe"
      local input = "%(RelativeDir)%(Filename)%(Extension)"
      local out = rule.out or ("%(RelativeDir)%(Filename)" .. (rule.ext or ".fxo"))
      local command
      if tool == "asm_shader" then
         command = string.format('"%s" "%s" "%s"', exe, input, out)
      else
         command = string.format('"%s" %s /Fo "%s" "%s"', exe, rule.args or "", out, input)
      end
      filter { "files:" .. rule.files }
         buildaction "CustomBuild"
         buildmessage("Compiling %(Filename)%(Extension) with " .. tool)
         buildcommands { command }
         buildoutputs { out }
      filter {}
   end
end

function add_postfx(id_postfx, id_areatex, id_searchtex)
   id_postfx = id_postfx or 201
   id_areatex = id_areatex or id_postfx + 1
   id_searchtex = id_searchtex or id_postfx + 2
   buildshaders {
      { files = "includes/postfx/*.fx", args = "/T fx_2_0", ext = ".fxo" }
   }
   includedirs { "Resources", "includes/postfx" }
   files { "includes/postfx/postfxcore.ixx", "includes/postfx/postfx.fx", "includes/postfx/postfx.fxo", "includes/postfx/postfx.rc" }
   defines { "IDR_POSTFX=" .. id_postfx }
   defines { "IDR_AREATEX=" .. id_areatex }
   defines { "IDR_SEARCHTEX=" .. id_searchtex }
end

-- The shared temporal anti-aliasing (source/Shared/AA: TAA, DLAA and FSR through FusionFixHub's bridge): its pixel
-- shaders, compiled at build time, and the resource file embedding them
function add_antialiasing()
   buildshaders {
      { files = "source/Shared/AA/shaders/*.hlsl", args = "/T ps_3_0 /E main", ext = ".pso" }
   }
   -- The D3D9 resources only; D3D11 games (AntiAliasingD3D11.rc) add their own
   files { "source/Shared/AA/*.h", "source/Shared/AA/*.hpp", "source/Shared/AA/AntiAliasing.rc", "source/Shared/AA/shaders/*.hlsl" }
end

function add_pspsdk()
   includedirs { "external/pspsdk/usr/local/pspdev/psp/sdk/include" }
   includedirs { "external/pspsdk/usr/local/pspdev/bin" }
   files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.hpp", "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp", "source/%{prj.name}/makefile", "source/%{prj.name}/module.json", "source/%{prj.name}/exports.exp" }
end

function add_ps2sdk()
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.hpp", "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp", "source/%{prj.name}/makefile", "source/%{prj.name}/module.json" }
end

function writeghaction(tag, prj_name)
   file = io.open(".github/workflows/" .. tag .. ".yml", "w")
   if (file) then
str = [[
name: %s

on:
  workflow_dispatch:

jobs:
  call-workflow-passing-data:
    uses: ThirteenAG/WidescreenFixesPack/.github/workflows/all.yml@master
    with:
      tag_list: %s
      project: /t:%s
]]
      file:write(string.format(str, tag, tag, prj_name:gsub("%.", "_")))
      file:close()
   end
end

function CommonWorkspaceSetup(platform, prefix)
   workspace (prefix .. ".WidescreenFixesPack")
      configurations { "Release", "Debug" }
      platforms { platform }
      location "build"
      objdir ("build/obj")
      buildlog ("build/log/%{prj.name}.log")
      cppdialect "C++latest"
      include "makefile.lua"
      buildoptions { "/Zc:__cplusplus /utf-8" }
      multiprocessorcompile ("On")

      kind "SharedLib"
      language "C++"
      targetdir "data/%{prj.name}/scripts"
      targetextension ".asi"
      characterset ("UNICODE")
      staticruntime "On"

      defines { "rsc_CompanyName=\"ThirteenAG\"" }
      defines { "rsc_LegalCopyright=\"MIT License\""}
      defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{cfg.buildtarget.name}\"" }
      defines { "rsc_FileDescription=\"https://thirteenag.github.io/wfp\"" }
      defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/WidescreenFixesPack\"" }

      local major = os.date("%d")
      local minor = os.date("%m")
      local build = os.date("%Y")
      local revision = os.date("%H") .. os.date("%M")

      local githash = ""
      local f = io.popen("git rev-parse --short HEAD")
      if f then
         githash = f:read("*a"):gsub("%s+", "")
         f:close()
      end

      local productVersion = major .. "." .. minor .. "." .. build .. "." .. revision
      if githash ~= "" then
         productVersion = productVersion .. "-" .. githash
      end

      defines { "rsc_FileVersion_MAJOR=" .. major }
      defines { "rsc_FileVersion_MINOR=" .. minor }
      defines { "rsc_FileVersion_BUILD=" .. build }
      defines { "rsc_FileVersion_REVISION=" .. revision }
      defines { "rsc_FileVersion=\"" .. major .. "." .. minor .. "." .. build .. "\"" }
      defines { "rsc_ProductVersion=\"" .. productVersion .. "\"" }
      defines { "rsc_GitSHA1=\"" .. githash .. "\"" }
      defines { "rsc_GitSHA1W=L\"" .. githash .. "\"" }

      resincludedirs { "$(IntDir)" }
      prebuildcommands { 'powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%{wks.location}../tools/write-build-time-utc.ps1" -OutputPath "$(IntDir)BuildTimeUtc.h"' }

      files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.hpp", "source/%{prj.name}/*.cpp", "source/%{prj.name}/*.hxx", "source/%{prj.name}/*.ixx" }
      files { "data/%{prj.name}/**" }
      files { "Resources/*.rc" }
      files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
      files { "external/injector/safetyhook/include/**.hpp", "external/injector/safetyhook/src/**.cpp" }
      files { "external/injector/minhook/include/*.h", "external/injector/minhook/src/**.h", "external/injector/minhook/src/**.c" }
      files { "external/injector/utility/FunctionHookMinHook.hpp", "external/injector/utility/FunctionHookMinHook.cpp" }
      files { "external/injector/zydis/**.h", "external/injector/zydis/**.c" }
      files { "includes/stdafx.h", "includes/stdafx.cpp" }
      includedirs { "external/injector/minhook/include" }
      includedirs { "external/injector/utility" }
      includedirs { "external/injector/safetyhook/include" }
      includedirs { "external/injector/zydis" }
      includedirs { "external/hooking" }
      includedirs { "external/injector/include" }
      includedirs { "external/inireader" }
      includedirs { "external/spdlog/include" }
      includedirs { "external/filewatch" }
      includedirs { "external/modutils" }
      includedirs { "includes" }

      includedirs { "includes/LED" }
      libdirs { "includes/LED" }

      includedirs { "external/minidx9/Include" }

      vpaths {
         ["source"] = { "source/**.*" },
         ["shaders"] = { "source/**.fx", "source/**.vs", "source/**.ps", "source/**.hlsl" },
         ["ini"] = { "data/**.ini" },
         ["data"] = { "data/**.cfg", "data/**.dat" },
         ["resources/*"] = { "resources/*" },
         ["includes/*"] = { "includes/**" },
         ["external/*"] = { "external/**" },
      }

      filter { "platforms:Win32" }
         architecture "x86"
         libdirs { "external/minidx9/Lib/x86" }
      filter { "platforms:x64" }
         architecture "x64"
         libdirs { "external/minidx9/Lib/x64" }
      -- unknown VS platforms: keep the system PATH for the build commands
      filter { "platforms:PS2" }
         system "ps2"
         bindirs { "$(PATH)" }
      filter { "platforms:PSP" }
         system "psp"
         bindirs { "$(PATH)" }
      filter {}

      filter "configurations:Debug*"
         defines "DEBUG"
         symbols "On"

      filter "configurations:Release*"
         defines "NDEBUG"
         optimize "On"
end

-- ====================== WIN32 SOLUTION ======================
CommonWorkspaceSetup("Win32", "Win32")

group ""
project "Bully.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/Bully/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Bully/en.json') }
   setpaths("BULLY_SCHOLARSHIP_EDITION_DIR", "Bully.exe", "plugins/")

project "CallOfCthulhu.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/CallOfCthulhu/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/CallOfCthulhu/en.json') }
   setpaths("CALL_OF_CTHULHU_DIR", "Engine/CoCMainWin32.exe", "Engine/scripts/")

project "ColdFear.WidescreenFix"
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/ColdFear/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ColdFear/en.json') }
   setpaths("COLDFEAR_DIR", "ColdFear_retail.exe")

project "Condemned.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/Condemned/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Condemned/en.json') }
   setpaths("CONDEMNED_CRIMINAL_ORIGINS_DIR", "Condemned.exe")

project "DeerAvenger4.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/DeerAvenger4/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/DeerAvenger4/en.json') }
   setpaths("DEER_AVENGER_4_DIR", "DA4.exe")

project "Driv3r.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/Driv3r/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Driv3r/en.json') }
   add_postfx()
   setpaths("DRIV3R_DIR", "driv3r.exe")

project "DriverParallelLines.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/DriverParallelLines/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/DriverParallelLines/en.json') }
   add_postfx()
   setpaths("DRIVER_PARALLEL_LINES_DIR", "DriverParallelLines.exe")

project "EnterTheMatrix.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/EnterTheMatrix/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/EnterTheMatrix/en.json') }
   setpaths("ENTER_THE_MATRIX_DIR", "Matrix.exe")

project "FarCry.WidescreenFix"
   files { "source/FarCry.WidescreenFix/resources.rc", "source/FarCry.WidescreenFix/settings.json", "source/FarCry.WidescreenFix/FixSettings.hpp", "text/FarCry/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/FarCry.WidescreenFix/settings.json', '%{wks.location}../text/FarCry/en.json') }
   add_postfx()
   setpaths("FAR_CRY_DIR", "Bin32/FarCry.exe", "Bin32/")

group "GrandTheftAuto"
project "GTA1.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/GTAClassic/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAClassic/en.json') }
   setpaths("GRAND_THEFT_AUTO_1_LONDON_1969_1961_DIR", "WINO/Grand Theft Auto.exe", "WINO/scripts/")
project "GTA2.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/GTA2/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTA2/en.json') }
   setpaths("GRAND_THEFT_AUTO_2_DIR", "gta2.exe")
project "GTA3.WidescreenFix"
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/RWGTA/*.hpp", "text/RWGTA/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/RWGTA/en.json') }
   setpaths("GTAIII_DIR", "gta3.exe")
project "GTAVC.WidescreenFix"
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/RWGTA/*.hpp", "text/RWGTA/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/RWGTA/en.json') }
   setpaths("GRAND_THEFT_AUTO_VICE_CITY_DIR", "gta-vc.exe")
project "GTASA.WidescreenFix"
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/RWGTA/*.hpp", "text/RWGTA/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/RWGTA/en.json') }
   setpaths("GTA_SAN_ANDREAS_DIR", "gta_sa.exe")
group ""

project "Gun.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/Gun/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Gun/en.json') }
   setpaths("GUN_DIR", "Gun.exe")

project "HiddenandDangerous2.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/HiddenandDangerous2/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/HiddenandDangerous2/en.json') }
   setpaths("HIDDEN_AND_DANGEROUS_2_DIR", "hd2.exe")

project "JustCause.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/JustCause/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/JustCause/en.json') }
   setpaths("JUST_CAUSE_DIR", "JustCause.exe")

project "KingKong.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/KingKong/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/KingKong/en.json') }
   buildshaders {
      { files = "source/*/*.ps", args = "/T ps_3_0 /nologo /E main", ext = ".pso" },
      { files = "source/*/*.vs", args = "/T vs_3_0 /nologo /E main", ext = ".vso" }
   }
   files { "source/%{prj.name}/*.ps", "source/%{prj.name}/*.vs", "source/%{prj.name}/*.rc" }
   defines { "IDR_BLURPS=200" }
   defines { "IDR_BLURVS=201" }
   defines { "IDR_REMANANCEPS=202" }
   setpaths("KING_KONG_GAMERS_EDITION_DIR", "KingKong8.exe")

project "KnightRider.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/KnightRider/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/KnightRider/en.json') }
   setpaths("KNIGHT_RIDER_DIR", "Knight Rider.exe")

project "KnightRider2.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/KnightRider2/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/KnightRider2/en.json') }
   setpaths("KNIGHT_RIDER_2_DIR", "KR2.exe")

project "LARush.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/LARush/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/LARush/en.json') }
   setpaths("LA_RUSH_DIR", "LARush.exe", "plugins/")

project "Mafia.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/Mafia/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Mafia/en.json') }
   add_postfx()
   setpaths("MAFIA_DIR", "Game.exe")

project "Manhunt.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/RWGTA/GameThread.hpp", "text/Manhunt/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Manhunt/en.json') }
   setpaths("MANHUNT_DIR", "manhunt.exe")

group "MaxPayne"
project "MaxPayne.MSVCP60Wrapper"
   setpaths("MAX_PAYNE_DIR", "MaxPayne.exe", "")
   targetdir "data/MaxPayne.WidescreenFix"
   targetname "MSVCP60"
   targetextension ".dll"
   files { "source/%{prj.name}/*.def" }
   files { "source/%{prj.name}/*.rc" }
   files { "source/%{prj.name}/MemoryModule.h", "source/%{prj.name}/MemoryModule.c" }
project "MaxPayne.WidescreenFix"
   dependson { "MaxPayne.MSVCP60Wrapper" }
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/MaxPayne/*.hpp", "text/MaxPayne/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/MaxPayne/en.json') }
   debugargs { "-skipstartup -window -developer -screenshot -nodialog" }
   linkoptions { "/SAFESEH:NO" }
   libdirs { "includes/minidx8" }
   setpaths("MAX_PAYNE_DIR", "MaxPayne.exe")
project "MaxPayne2.WidescreenFix"
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/MaxPayne/*.hpp", "text/MaxPayne/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/MaxPayne/en.json') }
   debugargs { "-skipstartup -developer -window -nodialog" }
   linkoptions { "/SAFESEH:NO" }
   libdirs { "includes/minidx8" }
   setpaths("MAX_PAYNE_2_THE_FALL_OF_MAX_PAYNE_DIR", "MaxPayne2.exe")
group ""

group "Tools"
-- Checks each embedded settings description with the shared settings validator.
project "AsiSettingsValidator"
   kind "ConsoleApp"
   targetextension ".exe"
   targetname "asi-settings-validate"
   targetdir "build/tools"
   files { "source/Shared/AsiSettings/tools/validator.cpp" }
   removefiles { "external/**", "includes/**", "Resources/*.rc" }
group ""

group "NeedForSpeed"
project "NFSTheRun.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/NFSTheRun/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFSTheRun/en.json') }
   setpaths("NEED_FOR_SPEED_THE_RUN_DIR", "Need For Speed The Run.exe", "plugins/")
project "NFSCarbon.WidescreenFix"
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "textures/NFS/NFSC/textures/*.dds", "text/NFS/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFS/en.json') }
   setpaths("NEED_FOR_SPEED_CARBON_DIR", "NFSC.exe")
project "NFSMostWanted.WidescreenFix"
   add_antialiasing()
   buildshaders {
      { files = "source/*/*.fx", args = "/T fx_2_0", ext = ".fxo" }
   }
   includedirs { "Resources", "includes/postfx" }
   files { "includes/postfx/postfxcore.ixx", "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc", "source/%{prj.name}/settings.json", "textures/NFS/NFSMW/textures/*.dds", "text/NFS/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFS/en.json') }
   defines { "IDR_POSTFX=201" }
   defines { "IDR_AREATEX=202" }
   defines { "IDR_SEARCHTEX=203" }
   setpaths("NEED_FOR_SPEED_MOST_WANTED_DIR", "speed.exe")
project "NFSProStreet.FusionFix"
   add_postfx()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "textures/NFS/NFSPS/textures/*.dds", "text/NFS/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFS/en.json') }
   setpaths("NEED_FOR_SPEED_PROSTREET_DIR", "nfsps.exe")
project "NFSUndercover.FusionFix"
   add_postfx()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "textures/NFS/NFSUC/textures/*.dds", "text/NFS/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFS/en.json') }
   setpaths("NEED_FOR_SPEED_UNDERCOVER_DIR", "nfs.exe")
project "NFSUnderground.WidescreenFix"
   add_postfx()
   add_antialiasing()
   defines { "IDR_NFSUICON=200" }
   files { "textures/NFS/NFSU/icon.rc", "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "textures/NFS/NFSU/HudLayout.dat", "textures/NFS/NFSU/textures/*.dds", "text/NFS/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFS/en.json') }
   setpaths("NEED_FOR_SPEED_UNDERGROUND_DIR", "speed.exe")
project "NFSUnderground2.WidescreenFix"
   add_postfx()
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "textures/NFS/NFSU2/HudLayout.dat", "textures/NFS/NFSU/textures/*.dds", "text/NFS/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/NFS/en.json') }
   setpaths("NEED_FOR_SPEED_UNDERGROUND_2_DIR", "speed2.exe")
group ""

project "Onimusha3.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/Onimusha3/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Onimusha3/en.json') }
   setpaths("ONIMUSHA_3_DIR", "ONI3.exe")
project "ParadiseCracked.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ParadiseCracked/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ParadiseCracked/en.json') }
   setpaths("PARADISE_CRACKED_DIR", "game.exe")
project "PsiOpsTheMindgateConspiracy.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/PsiOps/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/PsiOps/en.json') }
   setpaths("PSI_OPS_DIR", "PsiOps.exe")
project "Psychonauts.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/Psychonauts/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Psychonauts/en.json') }
   setpaths("PSYCHONAUTS_DIR", "Psychonauts.exe")

group "ResidentEvil"
project "ResidentEvil0.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvil0/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvil0/en.json') }
   -- The replacement pixel shaders of MAIN.DisableNoise/DisableColorCorrection/DisableFog, assembled at build time
   buildshaders {
      { files = "source/ResidentEvil0.FusionFix/*.ps", tool = "asm_shader", ext = ".pso" }
   }
   files { "source/%{prj.name}/*.ps" }
   setpaths("RESIDENTEVIL0_DIR", "re0hd.exe", "scripts/")
project "ResidentEvil.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvil/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvil/en.json') }
   -- The replacement pixel shaders of MAIN.DisableNoise/DisableColorCorrection/DisableFog, assembled at build time
   buildshaders {
      { files = "source/ResidentEvil.FusionFix/*.ps", tool = "asm_shader", ext = ".pso" }
   }
   files { "source/%{prj.name}/*.ps" }
   setpaths("RESIDENTEVIL_DIR", "bhd.exe", "scripts/")
project "ResidentEvilRevelations.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvilRevelations/*.json" }
   -- Replacement shaders, compiled at build time
   buildshaders {
      { files = "source/ResidentEvilRevelations.FusionFix/shaders/*.hlsl", args = "/T vs_3_0 /E main /nologo", ext = ".vso" }
   }
   files { "source/%{prj.name}/shaders/*.vs", "source/%{prj.name}/shaders/*.hlsl" }
   defines { "IDR_SHADER_MOVIE_VS=201" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvilRevelations/en.json') }
   setpaths("RESIDENTEVILREVELATIONS_DIR", "rerev.exe", "scripts/")
project "ResidentEvilRevelations2.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvilRevelations2/*.json" }
   -- Replacement shaders, compiled at build time
   buildshaders {
      { files = "source/ResidentEvilRevelations2.FusionFix/shaders/*.vs", tool = "asm_shader", ext = ".vso" },
      { files = "source/ResidentEvilRevelations2.FusionFix/shaders/*.hlsl", args = "/T vs_3_0 /E main /nologo", ext = ".vso" }
   }
   files { "source/%{prj.name}/shaders/*.vs", "source/%{prj.name}/shaders/*.hlsl" }
   defines { "IDR_SHADER_OVERLAY_VS=200", "IDR_SHADER_MOVIE_VS=201" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvilRevelations2/en.json') }
   setpaths("RESIDENTEVILREVELATIONS2_DIR", "rerev2.exe", "scripts/")
project "ResidentEvil5.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvil5/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvil5/en.json') }
   setpaths("RESIDENTEVIL5_DIR", "re5dx9.exe", "scripts/")
project "ResidentEvil6.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvil6/*.json" }
   -- Replacement shaders, assembled at build time
   buildshaders {
      { files = "source/ResidentEvil6.FusionFix/shaders/*.vs", tool = "asm_shader", ext = ".vso" },
      { files = "source/ResidentEvil6.FusionFix/shaders/*.ps", tool = "asm_shader", ext = ".pso" }
   }
   files { "source/%{prj.name}/shaders/*.vs", "source/%{prj.name}/shaders/*.ps" }
   defines { "IDR_SHADER_OVERLAY_VS=200", "IDR_SHADER_DAMAGEOVERLAY_VS=201" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvil6/en.json') }
   setpaths("RESIDENTEVIL6_DIR", "BH6.exe", "scripts/")
group ""

project "Scarface.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/Scarface/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Scarface/en.json') }
   add_postfx()
   files { "source/%{prj.name}/ControllerIcons.rc", "textures/Scarface/Controller/*.dds" }
   setpaths("SCARFACE_DIR", "scarface.exe", "scripts/")

project "SecondSight.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/SecondSight/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SecondSight/en.json') }
   setpaths("SECOND_SIGHT_DIR", "secondsight.exe")

group "SilentHill"
project "SilentHill2.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/SilentHill/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SilentHill/en.json') }
   setpaths("SILENT_HILL_2_DIR", "sh2pc.exe")
project "SilentHill3.WidescreenFix"
   add_antialiasing()
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/SilentHill/*.hpp", "text/SilentHill/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SilentHill/en.json') }
   setpaths("SILENT_HILL_3_DIR", "sh3.exe")
project "SilentHill4.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/Shared/SilentHill/*.hpp", "text/SilentHill/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SilentHill/en.json') }
   setpaths("SILENT_HILL_4_THE_ROOM_DIR", "Silent Hill 4.exe")
group ""

project "SniperElite.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/SniperElite/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SniperElite/en.json') }
   setpaths("SNIPER_ELITE_DIR", "SniperElite.exe")

project "SonicHeroes.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/SonicHeroes/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SonicHeroes/en.json') }
   setpaths("SONICHEROES_DIR", "Tsonic_win.exe")

group "SplinterCell"
project "SplinterCell.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/SplinterCell/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCell/en.json') }
   setpaths("SPLINTER_CELL_DIR", "system/SplinterCell.exe", "system/scripts/")
project "SplinterCellChaosTheory.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/SplinterCellChaosTheory/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCellChaosTheory/en.json') }
   setpaths("SPLINTERCELL_CHAOS_THEORY_DIR", "system/splintercell3.exe", "system/scripts/")
project "SplinterCellConviction.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/SplinterCellConviction/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCellConviction/en.json') }
   setpaths("TOM_CLANCYS_SPLINTER_CELL_CONVICTION_DIR", "src/system/conviction_game.exe", "src/system/scripts/")
project "SplinterCellBlacklist.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/SplinterCellBlacklist/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCellBlacklist/en.json') }
   setpaths("SPLINTER_CELL_BLACKLIST_DIR", "src/SYSTEM/Blacklist_DX11_game.exe", "src/system/scripts/")
project "SplinterCellDoubleAgent.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/SplinterCellDoubleAgent/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCellDoubleAgent/en.json') }
   buildshaders {
      { files = "includes/postfx/*.fx", args = "/T fx_2_0", ext = ".fxo" },
      { files = "source/*/*.ps", tool = "asm_shader", ext = ".pso" },
      { files = "source/*/*.vs", tool = "asm_shader", ext = ".vso" }
   }
   includedirs { "Resources", "includes/postfx" }
   files { "includes/postfx/postfxcore.ixx", "includes/postfx/postfx.fx", "includes/postfx/postfx.fxo", "includes/postfx/postfx.rc" }
   files { "source/%{prj.name}/*.ps", "source/%{prj.name}/*.rc" }
   files { "textures/SCDA/icon.rc" }
   defines { "IDR_SCDAICON=200" }
   defines { "IDR_POSTFX=201" }
   defines { "IDR_SHADER_BB6378E1=202" }
   defines { "IDR_AREATEX=203" }
   defines { "IDR_SEARCHTEX=204" }
   setpaths("SPLINTER_CELL_DOUBLE_AGENT_DIR", "SCDA-Offline/System/SplinterCell4.exe", "SCDA-Offline/System/scripts/")
project "SplinterCellPandoraTomorrow.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/SplinterCellPandoraTomorrow/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCellPandoraTomorrow/en.json') }
   buildshaders {
      { files = "source/*/*.fx", args = "/Tps_1_1 /LD /Ewaterblend", ext = ".fxo" },
      { files = "source/*/*.ps", tool = "asm_shader", ext = ".pso" },
      { files = "source/*/*.vs", tool = "asm_shader", ext = ".vso" }
   }
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc" }
   defines { "IDR_WATER_BLEND=200" }
   debugargs { "-uplay_steam_mode" }
   setpaths("SPLINTER_CELL_PANDORA_TOMORROW_DIR", "system/SplinterCell2.exe", "system/scripts/")
group ""

project "StreetRacingSyndicate.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/SRS/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SRS/en.json') }
   setpaths("STREET_RACING_SYNDICATE_DIR", "Bin/srs.exe", "Bin/scripts/")

project "TheGodfather.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TheGodfather/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TheGodfather/en.json') }
   setpaths("THE_GODFATHER_DIR", "godfather.exe")

project "TheMatrixPathOfNeo.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TheMatrixPathOfNeo/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TheMatrixPathOfNeo/en.json') }
   setpaths("THE_MATRIX_PATH_OF_NEO_DIR", "Matrix3.exe")

project "ThePunisher.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ThePunisher/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ThePunisher/en.json') }
   setpaths("THE_PUNISHER_DIR", "pun.exe")

project "TheSaboteur.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TheSaboteur/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TheSaboteur/en.json') }
   add_postfx()
   setpaths("THE_SABOTEUR_DIR", "Saboteur.exe")

project "TheSuffering.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TheSuffering/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TheSuffering/en.json') }
   setpaths("THE_SUFFERING_DIR", "suffering.exe")

group "TonyHawks"
project "TonyHawksAmericanWasteland.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TonyHawksAmericanWasteland/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TonyHawksAmericanWasteland/en.json') }
   setpaths("TONY_HAWKS_AMERICAN_WASTELAND_DIR", "Game/THAW.exe", "Game/scripts/")
project "TonyHawksProSkater2.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/TonyHawksProSkater2/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TonyHawksProSkater2/en.json') }
   setpaths("THPS2_DIR", "THawk2.exe")
project "TonyHawksProSkater3.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TonyHawksProSkater3/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TonyHawksProSkater3/en.json') }
   setpaths("PRO_SKATER_3_DIR", "Skate3.exe")
project "TonyHawksProSkater4.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/TonyHawksProSkater4/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TonyHawksProSkater4/en.json') }
   setpaths("TONY_HAWKS_PRO_SKATER_4_DIR", "Game/Skate4.exe", "Game/scripts/")
project "TonyHawksUnderground.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TonyHawksUnderground/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TonyHawksUnderground/en.json') }
   setpaths("TONY_HAWKS_UNDERGROUND_DIR", "Game/THUG.exe", "Game/scripts/")
project "TonyHawksUnderground2.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "text/TonyHawksUnderground2/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TonyHawksUnderground2/en.json') }
   setpaths("TONY_HAWKS_UNDERGROUND_2_DIR", "Game/THUG2.exe", "Game/scripts/")
group ""

project "TotalOverdose.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TotalOverdose/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TotalOverdose/en.json') }
   setpaths("TOTAL_OVERDOSE_DIR", "TOD.exe")

project "TrueCrimeNewYorkCity.WidescreenFix"
   add_antialiasing()
   includedirs { "external/imgui" }
   files { "source/Shared/Overlay/*.hpp", "external/imgui/imgui.cpp", "external/imgui/imgui_draw.cpp",
      "external/imgui/imgui_tables.cpp", "external/imgui/imgui_widgets.cpp", "external/imgui/backends/imgui_impl_win32.cpp",
      "external/imgui/backends/imgui_impl_dx9.cpp" }
   buildshaders {
      { files = "source/*/*.fx", args = "/T fx_2_0", ext = ".fxo" }
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc", "source/%{prj.name}/settings.json", "text/TrueCrime/*.json" }
   -- A broken settings description fails the build
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TrueCrime/en.json') }
   defines { "IDR_POSTFX=200" }
   defines { "IDR_AREATEX=201" }
   defines { "IDR_SEARCHTEX=202" }
   setpaths("TRUE_CRIME_NEW_YORK_CITY_DIR", "True Crime New York City.exe")

project "TrueCrimeStreetsofLA.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/TrueCrimeStreetsofLA/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TrueCrimeStreetsofLA/en.json') }
   setpaths("TRUE_CRIME_STREETS_OF_LA_DIR", "TrueCrimeMB.exe")

project "UltimateSpiderMan.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/UltimateSpiderMan/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/UltimateSpiderMan/en.json') }
   add_postfx()
   setpaths("ULTIMATE_SPIDER_MAN_DIR", "USM.exe")
group ""

-- ====================== WIN64 SOLUTION ======================
CommonWorkspaceSetup("x64", "Win64")

group ""
--project "EmbedPDB"
--   kind "ConsoleApp"
--   targetextension ".exe"
--   files { "./source/%{prj.name}/*.h", "./source/%{prj.name}/*.c" }
--   setpaths("./data/%{prj.name}/", "%{prj.name}.exe", "")
project "FarCry64.WidescreenFix"
   files { "source/FarCry.WidescreenFix/resources.rc", "source/FarCry.WidescreenFix/settings.json", "source/FarCry.WidescreenFix/FixSettings.hpp", "text/FarCry/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/FarCry.WidescreenFix/settings.json', '%{wks.location}../text/FarCry/en.json') }
   add_postfx()
   files { "source/FarCry.WidescreenFix/*.cpp", "source/FarCry.WidescreenFix/*.ixx" }
   setpaths("FAR_CRY_DIR", "Bin64/FarCry.exe", "Bin64/")
   targetdir "data/FarCry.WidescreenFix/Bin64/"

group "GrandTheftAuto"
project "GTA3DE.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/GTA3DE/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTA3DE/en.json') }
   setpaths("GTA_III_DEFINITIVE_EDITION_DIR", "Gameface/Binaries/Win64/LibertyCity.exe", "Gameface/Binaries/Win64/scripts/")
project "GTAVCDE.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/GTAVCDE/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAVCDE/en.json') }
   setpaths("GTA_VICE_CITY_DEFINITIVE_EDITION_DIR", "Gameface/Binaries/Win64/ViceCity.exe", "Gameface/Binaries/Win64/scripts/")
project "GTASADE.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/GTASADE/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTASADE/en.json') }
   setpaths("GTA_SAN_ANDREAS_DEFINITIVE_EDITION_DIR", "Gameface/Binaries/Win64/SanAndreas.exe", "Gameface/Binaries/Win64/scripts/")
group ""

project "SpyroReignitedTrilogy.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/SpyroReignitedTrilogy/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SpyroReignitedTrilogy/en.json') }
   setpaths("SPYRO_REIGNITED_TRILOGY_DIR", "Falcon/Binaries/Win64/Spyro-Win64-Shipping.exe", "Falcon/Binaries/Win64/scripts/")

project "RedDeadRedemption.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/RedDeadRedemption/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/RedDeadRedemption/en.json') }
   add_kananlib()
   setpaths("RED_DEAD_REDEMPTION_DIR", "RDR.exe", "plugins/")
group ""

-- ====================== PCSX2F SOLUTION ======================
CommonWorkspaceSetup("PS2", "PCSX2F")

group ""
project "Burnout3.PCSX2F.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/Burnout3/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Burnout3/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "Burnout3.PCSX2F.WidescreenFix", "Burnout3")
   writemakefile_ps2("Burnout3.PCSX2F.WidescreenFix", "PLUGINS/", "-l:libc.a -l:libm.a -l:libgcc.a", "../../external/injector/include/ps2/log.o",
   "../../external/injector/include/ps2/memalloc.o", "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o",
   "../../external/injector/include/ps2/inireader.o", "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("Burnout3.PCSX2F.WidescreenFix")

group "GrandTheftAuto"
project "GTALCS.PCSX2F.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/GTALCSPS2/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTALCSPS2/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PCSX2F.WidescreenFix", "GTALCSPS2")
   writemakefile_ps2("GTALCS.PCSX2F.WidescreenFix", "PLUGINS/", "-l:libc.a -l:libm.a -l:libgcc.a", "Camera.o", "Draw.o", "Hud.o", "../../external/injector/include/ps2/log.o",
   "../../external/injector/include/ps2/memalloc.o", "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o",
   "../../external/injector/include/ps2/inireader.o", "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("GTALCS.PCSX2F.WidescreenFix")

project "GTAVCS.PCSX2F.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/GTAVCSPS2/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAVCSPS2/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.WidescreenFix", "GTAVCSPS2")
   writemakefile_ps2("GTAVCS.PCSX2F.WidescreenFix", "PLUGINS/", "-l:libc.a", "Camera.o", "Cheats.o", "Controls.o", "Draw.o", "Hud.o", "Menu.o", "MouseCamera.o", "../../external/injector/include/ps2/memalloc.o",
   "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o","../../external/injector/include/ps2/inireader.o",
   "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.WidescreenFix")

project "GTAVCS.PCSX2F.ImVehLM"
   files { "source/%{prj.name}/settings.json", "text/GTAVCSPS2Lights/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAVCSPS2Lights/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.ImVehLM", "GTAVCSPS2Lights")
   writemakefile_ps2("GTAVCS.PCSX2F.ImVehLM", "PLUGINS/", "-l:libc.a", "../../external/injector/include/ps2/memalloc.o",
   "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o","../../external/injector/include/ps2/inireader.o",
   "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.ImVehLM")

project "GTALCS.PCSX2F.ImVehLM"
   files { "source/%{prj.name}/settings.json", "text/GTALCSPS2Lights/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTALCSPS2Lights/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PCSX2F.ImVehLM", "GTALCSPS2Lights")
   writemakefile_ps2("GTALCS.PCSX2F.ImVehLM", "PLUGINS/", "-l:libc.a", "../../external/injector/include/ps2/memalloc.o",
   "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o","../../external/injector/include/ps2/inireader.o",
   "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("GTALCS.PCSX2F.ImVehLM")
group ""

project "KnightRider.PCSX2F.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/KnightRiderPS2/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/KnightRiderPS2/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "KnightRider.PCSX2F.WidescreenFix", "KnightRiderPS2")
   writemakefile_ps2("KnightRider.PCSX2F.WidescreenFix", "PLUGINS/", "-l:libc.a -l:libm.a -l:libgcc.a", "../../external/injector/include/ps2/log.o",
   "../../external/injector/include/ps2/memalloc.o", "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o",
   "../../external/injector/include/ps2/inireader.o", "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("KnightRider.PCSX2F.WidescreenFix")

project "SplinterCellDoubleAgent.PCSX2F.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/SCDAPS2/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SCDAPS2/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SplinterCellDoubleAgent.PCSX2F.WidescreenFix", "SCDAPS2")
   writemakefile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix", "PLUGINS/", "-l:libc.a -l:libm.a -l:libgcc.a", "../../external/injector/include/ps2/log.o",
   "Hud.o",
   "../../external/injector/include/ps2/memalloc.o", "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o",
   "../../external/injector/include/ps2/inireader.o", "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix")

project "TrueCrimeNewYorkCity.PCSX2F.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/TCNYCPS2/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/TCNYCPS2/en.json') }
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "TrueCrimeNewYorkCity.PCSX2F.WidescreenFix", "TCNYCPS2")
   writemakefile_ps2("TrueCrimeNewYorkCity.PCSX2F.WidescreenFix", "PLUGINS/", "-l:libc.a -l:libm.a -l:libgcc.a", "Hud.o", "Timing.o",
   "../../external/injector/include/ps2/memalloc.o", "../../external/injector/include/ps2/patterns.o", "../../external/injector/include/ps2/injector.o", "../../external/injector/include/ps2/rini.o",
   "../../external/injector/include/ps2/inireader.o", "../../external/injector/include/ps2/mips.o")
   writelinkfile_ps2("TrueCrimeNewYorkCity.PCSX2F.WidescreenFix")
group ""

-- ====================== PPSSPP SOLUTION ======================
CommonWorkspaceSetup("PSP", "PPSSPP")

group ""
group "GrandTheftAuto"
project "GTALCS.PPSSPP.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/GTALCSPSP/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTALCSPSP/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.WidescreenFix", "GTALCSPSP")
   writemakefile_psp("GTALCS.PPSSPP.WidescreenFix")

project "GTALCS.PPSSPP.ImVehLM"
   files { "source/%{prj.name}/settings.json", "text/GTALCSPSPLights/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTALCSPSPLights/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.ImVehLM", "GTALCSPSPLights")
   writemakefile_psp("GTALCS.PPSSPP.ImVehLM")

project "GTAVCS.PPSSPP.WidescreenFix"
   files { "source/%{prj.name}/settings.json", "text/GTAVCSPSP/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAVCSPSP/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.WidescreenFix", "GTAVCSPSP")
   writemakefile_psp("GTAVCS.PPSSPP.WidescreenFix")

project "GTAVCS.PPSSPP.ImVehLM"
   files { "source/%{prj.name}/settings.json", "text/GTAVCSPSPLights/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAVCSPSPLights/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.ImVehLM", "GTAVCSPSPLights")
   writemakefile_psp("GTAVCS.PPSSPP.ImVehLM")

project "GTAVCS.PPSSPP.GamepadIcons"
   files { "source/%{prj.name}/settings.json", "text/GTAVCSIcons/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTAVCSIcons/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.GamepadIcons", "GTAVCSIcons")
   writemakefile_psp("GTAVCS.PPSSPP.GamepadIcons")

project "GTACTW.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/GTACTW/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/GTACTW/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTACTW.PPSSPP.FusionFix", "GTACTW")
   writemakefile_psp("GTACTW.PPSSPP.FusionFix")
group ""

project "MidnightClubLARemix.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/MCLARemix/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/MCLARemix/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "MidnightClubLARemix.PPSSPP.FusionFix", "MCLARemix")
   writemakefile_psp("MidnightClubLARemix.PPSSPP.FusionFix")

project "SplinterCellEssentials.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/SCEssentials/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SCEssentials/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SplinterCellEssentials.PPSSPP.FusionFix", "SCEssentials")
   writemakefile_psp("SplinterCellEssentials.PPSSPP.FusionFix")

group "SOCOM"
project "SOCOM.FireteamBravo.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/SocomFTB1/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SocomFTB1/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo.PPSSPP.FusionFix", "SocomFTB1")
   writemakefile_psp("SOCOM.FireteamBravo.PPSSPP.FusionFix")
project "SOCOM.FireteamBravo2.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/SocomFTB2/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SocomFTB2/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo2.PPSSPP.FusionFix", "SocomFTB2")
   writemakefile_psp("SOCOM.FireteamBravo2.PPSSPP.FusionFix")
project "SOCOM.FireteamBravo3.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/SocomFTB3/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SocomFTB3/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo3.PPSSPP.FusionFix", "SocomFTB3")
   writemakefile_psp("SOCOM.FireteamBravo3.PPSSPP.FusionFix")
group ""

project "TheWarriors.PPSSPP.FusionFix"
   files { "source/%{prj.name}/settings.json", "text/Warriors/*.json" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/Warriors/en.json') }
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "TheWarriors.PPSSPP.FusionFix", "Warriors")
   writemakefile_psp("TheWarriors.PPSSPP.FusionFix")
group ""

-- ====================== CXBXR SOLUTION ======================
CommonWorkspaceSetup("Win32", "CXBXR")

group ""
project "SplinterCellDoubleAgent.CXBXR.WidescreenFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/SplinterCellDoubleAgentXbox/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/SplinterCellDoubleAgentXbox/en.json') }
   setpaths("CXBXR_DIR", "cxbx.exe", "scripts/")
group ""

-- ====================== DOLPHIN SOLUTION ======================
CommonWorkspaceSetup("x64", "Dolphin")

group ""
project "ResidentEvil2.RE3.Dolphin.FusionFix"
   files { "source/%{prj.name}/resources.rc", "source/%{prj.name}/settings.json", "source/%{prj.name}/FixSettings.hpp", "text/ResidentEvilDolphin/*.json" }
   dependson { "AsiSettingsValidator" }
   prebuildcommands { settings_check('%{wks.location}../source/%{prj.name}/settings.json', '%{wks.location}../text/ResidentEvilDolphin/en.json') }
   add_kananlib()
   setpaths("DOLPHIN_DIR", "Dolphin.exe", "scripts/")
group ""

if not _OPTIONS["no-shared-release-deps"] then
   include "tools/msbuild/shared-dependencies.lua"
end
