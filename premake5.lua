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

function setbuildpaths_psp(key, exepath, scriptspath, pspsdkpath, sourcepath, prj_name)
   local gamepath = envdir(key)
   if (gamepath) then
     buildcommands {"setlocal EnableDelayedExpansion"}
     rebuildcommands {"setlocal EnableDelayedExpansion"}
     buildcommands {"set _PPSSPPMemstick=" .. gamepath .. "\\memstick\\PSP"}
     rebuildcommands {"set _PPSSPPMemstick=" .. gamepath .. "\\memstick\\PSP"}

     buildcommands {
     "powershell -ExecutionPolicy Bypass -File \"" .. pspsdkpath .. "\" -C \"" .. sourcepath .. "\"\r\n" ..
     "if !errorlevel! neq 0 exit /b !errorlevel!\r\n" ..
     "if not defined _PPSSPPMemstick goto :eof\r\n" ..
     "if not exist !_PPSSPPMemstick! goto :eof\r\n" ..
     "if not exist !_PPSSPPMemstick!/PLUGINS/ mkdir !_PPSSPPMemstick!/PLUGINS/\r\n" ..
     "set target=!_PPSSPPMemstick!/PLUGINS/$(ProjectName)\r\n" ..
     "copy /y $(NMakeOutput) \"!target!\"\r\n"
     }
     rebuildcommands {
     "powershell -ExecutionPolicy Bypass -File \"" .. pspsdkpath .. "\" -C \"" .. sourcepath .. "\" clean\r\n" ..
     "powershell -ExecutionPolicy Bypass -File \"" .. pspsdkpath .. "\" -C \"" .. sourcepath .. "\"\r\n" ..
     "if !errorlevel! neq 0 exit /b !errorlevel!\r\n" ..
     "if not defined _PPSSPPMemstick goto :eof\r\n" ..
     "if not exist !_PPSSPPMemstick! goto :eof\r\n" ..
     "set target=!_PPSSPPMemstick!/PLUGINS/$(ProjectName)\r\n" ..
     "copy /y $(NMakeOutput) \"!target!\"\r\n"
     }
     cleancommands {
     "setlocal EnableDelayedExpansion\r\n" ..
     "powershell -ExecutionPolicy Bypass -File \"" .. pspsdkpath .. "\" -C \"" .. sourcepath .. "\" clean\r\n" ..
     "if !errorlevel! neq 0 exit /b !errorlevel!\r\n"
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

function setbuildpaths_ps2(key, exepath, scriptspath, ps2sdkpath, sourcepath, prj_name)
   local gamepath = envdir(key)
   if (gamepath) then
     buildcommands {"setlocal EnableDelayedExpansion"}
     rebuildcommands {"setlocal EnableDelayedExpansion"}
     buildcommands {"set _PCSX2FDir=" .. gamepath}
     rebuildcommands {"set _PCSX2FDir=" .. gamepath}
     buildcommands {
     "powershell -ExecutionPolicy Bypass -File \"" .. ps2sdkpath .. "\" -C \"" .. sourcepath .. "\"\r\n" ..
     "if !errorlevel! neq 0 exit /b !errorlevel!\r\n" ..
     "if not defined _PCSX2FDir goto :eof\r\n" ..
     "if not exist !_PCSX2FDir! goto :eof\r\n" ..
     "if not exist !_PCSX2FDir!/PLUGINS mkdir !_PCSX2FDir!/PLUGINS\r\n" ..
     "set target=!_PCSX2FDir!/PLUGINS/\r\n" ..
     "copy /y $(NMakeOutput) \"!target!\"\r\n"
     }
     rebuildcommands {
     "powershell -ExecutionPolicy Bypass -File \"" .. ps2sdkpath .. "\" -C \"" .. sourcepath .. "\" clean\r\n" ..
     "powershell -ExecutionPolicy Bypass -File \"" .. ps2sdkpath .. "\" -C \"" .. sourcepath .. "\"\r\n" ..
     "if !errorlevel! neq 0 exit /b !errorlevel!\r\n" ..
     "if not defined _PCSX2FDir goto :eof\r\n" ..
     "if not exist !_PCSX2FDir! goto :eof\r\n" ..
     "if not exist !_PCSX2FDir!/PLUGINS mkdir !_PCSX2FDir!/PLUGINS\r\n" ..
     "set target=!_PCSX2FDir!/PLUGINS/\r\n" ..
     "copy /y $(NMakeOutput) \"!target!\"\r\n"
     }
     cleancommands {
     "setlocal EnableDelayedExpansion\r\n" ..
     "powershell -ExecutionPolicy Bypass -File \"" .. ps2sdkpath .. "\" -C \"" .. sourcepath .. "\" clean\r\n" ..
     "if !errorlevel! neq 0 exit /b !errorlevel!"
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

function add_pspsdk()
   includedirs { "external/pspsdk/usr/local/pspdev/psp/sdk/include" }
   includedirs { "external/pspsdk/usr/local/pspdev/bin" }
   files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp", "source/%{prj.name}/makefile" }
end

function add_ps2sdk()
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp", "source/%{prj.name}/makefile" }
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

      files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.cpp", "source/%{prj.name}/*.hxx", "source/%{prj.name}/*.ixx" }
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
   setpaths("BULLY_SCHOLARSHIP_EDITION_DIR", "Bully.exe", "plugins/")

project "CallOfCthulhu.WidescreenFix"
   setpaths("CALL_OF_CTHULHU_DIR", "Engine/CoCMainWin32.exe", "Engine/scripts/")

project "ColdFear.WidescreenFix"
   setpaths("COLDFEAR_DIR", "ColdFear_retail.exe")

project "Condemned.WidescreenFix"
   setpaths("CONDEMNED_CRIMINAL_ORIGINS_DIR", "Condemned.exe")

project "DeerAvenger4.WidescreenFix"
   setpaths("DEER_AVENGER_4_DIR", "DA4.exe")

project "Driv3r.WidescreenFix"
   add_postfx()
   setpaths("DRIV3R_DIR", "driv3r.exe")

project "DriverParallelLines.WidescreenFix"
   add_postfx()
   setpaths("DRIVER_PARALLEL_LINES_DIR", "DriverParallelLines.exe")

project "EnterTheMatrix.WidescreenFix"
   setpaths("ENTER_THE_MATRIX_DIR", "Matrix.exe")

project "FarCry.WidescreenFix"
   add_postfx()
   setpaths("FAR_CRY_DIR", "Bin32/FarCry.exe", "Bin32/")

group "GrandTheftAuto"
project "GTA1.WidescreenFix"
   setpaths("GRAND_THEFT_AUTO_1_LONDON_1969_1961_DIR", "WINO/Grand Theft Auto.exe", "WINO/scripts/")
project "GTA2.WidescreenFix"
   setpaths("GRAND_THEFT_AUTO_2_DIR", "gta2.exe")
project "GTA3.WidescreenFix"
   add_postfx()
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("GTAIII_DIR", "gta3.exe")
project "GTAVC.WidescreenFix"
   add_postfx()
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("GRAND_THEFT_AUTO_VICE_CITY_DIR", "gta-vc.exe")
project "GTASA.WidescreenFix"
   add_postfx()
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("GTA_SAN_ANDREAS_DIR", "gta_sa.exe")
group ""

project "Gun.WidescreenFix"
   setpaths("GUN_DIR", "Gun.exe")

project "HiddenandDangerous2.WidescreenFix"
   setpaths("HIDDEN_AND_DANGEROUS_2_DIR", "hd2.exe")

project "JustCause.WidescreenFix"
   setpaths("JUST_CAUSE_DIR", "JustCause.exe")

project "KingKong.WidescreenFix"
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
   setpaths("KNIGHT_RIDER_DIR", "Knight Rider.exe")

project "KnightRider2.WidescreenFix"
   setpaths("KNIGHT_RIDER_2_DIR", "KR2.exe")

project "LARush.WidescreenFix"
   setpaths("LA_RUSH_DIR", "LARush.exe", "plugins/")

project "Mafia.FusionFix"
   add_postfx()
   setpaths("MAFIA_DIR", "Setup.exe")
   targetdir "data/Mafia.WidescreenFix/scripts"

project "Mafia.WidescreenFix"
   setpaths("MAFIA_DIR", "GameV12.exe")

project "Manhunt.WidescreenFix"
   buildoptions { "/Zc:strictStrings-" }
   includedirs { "source/%{prj.name}/inc" }
   files { "source/%{prj.name}/**/*.cpp" }
   removefiles { "includes/stdafx.h", "includes/stdafx.cpp" }
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
   debugargs { "-skipstartup -window -developer -screenshot -nodialog" }
   linkoptions { "/SAFESEH:NO" }
   libdirs { "includes/minidx8" }
   setpaths("MAX_PAYNE_DIR", "MaxPayne.exe")
project "MaxPayne2.WidescreenFix"
   debugargs { "-skipstartup -developer -window -nodialog" }
   linkoptions { "/SAFESEH:NO" }
   libdirs { "includes/minidx8" }
   setpaths("MAX_PAYNE_2_THE_FALL_OF_MAX_PAYNE_DIR", "MaxPayne2.exe")
group ""

group "NeedForSpeed"
project "NFSTheRun.FusionFix"
   setpaths("NEED_FOR_SPEED_THE_RUN_DIR", "Need For Speed The Run.exe", "plugins/")
project "NFSCarbon.WidescreenFix"
   add_postfx()
   setpaths("NEED_FOR_SPEED_CARBON_DIR", "NFSC.exe")
project "NFSMostWanted.WidescreenFix"
   buildshaders {
      { files = "source/*/*.fx", args = "/T fx_2_0", ext = ".fxo" }
   }
   includedirs { "Resources", "includes/postfx" }
   files { "includes/postfx/postfxcore.ixx", "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=201" }
   defines { "IDR_AREATEX=202" }
   defines { "IDR_SEARCHTEX=203" }
   setpaths("NEED_FOR_SPEED_MOST_WANTED_DIR", "speed.exe")
project "NFSProStreet.FusionFix"
   add_postfx()
   setpaths("NEED_FOR_SPEED_PROSTREET_DIR", "nfsps.exe")
project "NFSUndercover.FusionFix"
   add_postfx()
   setpaths("NEED_FOR_SPEED_UNDERCOVER_DIR", "nfs.exe")
project "NFSUnderground.WidescreenFix"
   add_postfx()
   defines { "IDR_NFSUICON=200" }
   files { "textures/NFS/NFSU/icon.rc" }
   setpaths("NEED_FOR_SPEED_UNDERGROUND_DIR", "speed.exe")
project "NFSUnderground2.WidescreenFix"
   add_postfx()
   setpaths("NEED_FOR_SPEED_UNDERGROUND_2_DIR", "speed2.exe")
group ""

project "Onimusha3.WidescreenFix"
   setpaths("ONIMUSHA_3_DIR", "ONI3.exe")
project "ParadiseCracked.WidescreenFix"
   setpaths("PARADISE_CRACKED_DIR", "game.exe")
project "PsiOpsTheMindgateConspiracy.WidescreenFix"
   setpaths("PSI_OPS_DIR", "PsiOps.exe")
project "Psychonauts.WidescreenFix"
   setpaths("PSYCHONAUTS_DIR", "Psychonauts.exe")

group "ResidentEvil"
project "ResidentEvil0.FusionFix"
   setpaths("RESIDENTEVIL0_DIR", "re0hd.exe", "scripts/")
project "ResidentEvil.FusionFix"
   setpaths("RESIDENTEVIL_DIR", "bhd.exe", "scripts/")
project "ResidentEvilRevelations.FusionFix"
   setpaths("RESIDENTEVILREVELATIONS_DIR", "rerev.exe", "scripts/")
project "ResidentEvilRevelations2.FusionFix"
   setpaths("RESIDENTEVILREVELATIONS2_DIR", "rerev2.exe", "scripts/")
project "ResidentEvil5.FusionFix"
   setpaths("RESIDENTEVIL5_DIR", "re5dx9.exe", "scripts/")
project "ResidentEvil6.FusionFix"
   setpaths("RESIDENTEVIL6_DIR", "BH6.exe", "scripts/")
group ""

project "Scarface.FusionFix"
   setpaths("SCARFACE_DIR", "scarface.exe", "scripts/")

project "SecondSight.WidescreenFix"
   setpaths("SECOND_SIGHT_DIR", "secondsight.exe")

group "SilentHill"
project "SilentHill2.WidescreenFix"
   setpaths("SILENT_HILL_2_DIR", "sh2pc.exe")
project "SilentHill3.WidescreenFix"
   setpaths("SILENT_HILL_3_DIR", "sh3.exe")
project "SilentHill4.WidescreenFix"
   setpaths("SILENT_HILL_4_THE_ROOM_DIR", "Silent Hill 4.exe")
group ""

project "SniperElite.WidescreenFix"
   setpaths("SNIPER_ELITE_DIR", "SniperElite.exe")

project "SonicHeroes.WidescreenFix"
   setpaths("SONICHEROES_DIR", "Tsonic_win.exe")

group "SplinterCell"
project "SplinterCell.WidescreenFix"
   setpaths("SPLINTER_CELL_DIR", "system/SplinterCell.exe", "system/scripts/")
project "SplinterCellChaosTheory.WidescreenFix"
   setpaths("SPLINTERCELL_CHAOS_THEORY_DIR", "system/splintercell3.exe", "system/scripts/")
project "SplinterCellConviction.FusionFix"
   setpaths("TOM_CLANCYS_SPLINTER_CELL_CONVICTION_DIR", "src/system/conviction_game.exe", "src/system/scripts/")
project "SplinterCellBlacklist.FusionFix"
   setpaths("SPLINTER_CELL_BLACKLIST_DIR", "src/SYSTEM/Blacklist_DX11_game.exe", "src/system/scripts/")
project "SplinterCellDoubleAgent.WidescreenFix"
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
   setpaths("STREET_RACING_SYNDICATE_DIR", "Bin/srs.exe", "Bin/scripts/")

project "TheGodfather.WidescreenFix"
   setpaths("THE_GODFATHER_DIR", "godfather.exe")

project "TheMatrixPathOfNeo.WidescreenFix"
   setpaths("THE_MATRIX_PATH_OF_NEO_DIR", "Matrix3.exe")

project "ThePunisher.WidescreenFix"
   setpaths("THE_PUNISHER_DIR", "pun.exe")

project "TheSaboteur.FusionFix"
   add_postfx()
   setpaths("THE_SABOTEUR_DIR", "Saboteur.exe")

project "TheSuffering.WidescreenFix"
   setpaths("THE_SUFFERING_DIR", "suffering.exe")

group "TonyHawks"
project "TonyHawksAmericanWasteland.WidescreenFix"
   setpaths("TONY_HAWKS_AMERICAN_WASTELAND_DIR", "Game/THAW.exe", "Game/scripts/")
project "TonyHawksProSkater2.WidescreenFix"
   setpaths("THPS2_DIR", "THawk2.exe")
project "TonyHawksProSkater3.WidescreenFix"
   setpaths("PRO_SKATER_3_DIR", "Skate3.exe")
project "TonyHawksProSkater4.WidescreenFix"
   setpaths("TONY_HAWKS_PRO_SKATER_4_DIR", "Game/Skate4.exe", "Game/scripts/")
project "TonyHawksUnderground.WidescreenFix"
   setpaths("TONY_HAWKS_UNDERGROUND_DIR", "Game/THUG.exe", "Game/scripts/")
project "TonyHawksUnderground2.WidescreenFix"
   setpaths("TONY_HAWKS_UNDERGROUND_2_DIR", "Game/THUG2.exe", "Game/scripts/")
group ""

project "TotalOverdose.WidescreenFix"
   setpaths("TOTAL_OVERDOSE_DIR", "TOD.exe")

project "TrueCrimeNewYorkCity.WidescreenFix"
   buildshaders {
      { files = "source/*/*.fx", args = "/T fx_2_0", ext = ".fxo" }
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=200" }
   defines { "IDR_AREATEX=201" }
   defines { "IDR_SEARCHTEX=202" }
   setpaths("TRUE_CRIME_NEW_YORK_CITY_DIR", "True Crime New York City.exe")

project "TrueCrimeStreetsofLA.WidescreenFix"
   setpaths("TRUE_CRIME_STREETS_OF_LA_DIR", "TrueCrimeMB.exe")

project "UltimateSpiderMan.WidescreenFix"
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
   add_postfx()
   files { "source/FarCry.WidescreenFix/*.cpp", "source/FarCry.WidescreenFix/*.ixx" }
   setpaths("FAR_CRY_DIR", "Bin64/FarCry.exe", "Bin64/")
   targetdir "data/FarCry.WidescreenFix/Bin64/"

group "GrandTheftAuto"
project "GTA3DE.FusionFix"
   add_kananlib()
   setpaths("GTA_III_DEFINITIVE_EDITION_DIR", "Gameface/Binaries/Win64/LibertyCity.exe", "Gameface/Binaries/Win64/scripts/")
project "GTAVCDE.FusionFix"
   add_kananlib()
   setpaths("GTA_VICE_CITY_DEFINITIVE_EDITION_DIR", "Gameface/Binaries/Win64/ViceCity.exe", "Gameface/Binaries/Win64/scripts/")
project "GTASADE.FusionFix"
   add_kananlib()
   setpaths("GTA_SAN_ANDREAS_DEFINITIVE_EDITION_DIR", "Gameface/Binaries/Win64/SanAndreas.exe", "Gameface/Binaries/Win64/scripts/")
group ""

group "ResidentEvil"
project "ResidentEvil4.FusionFix"
   setpaths("RESIDENTEVIL4_DIR", "re4.exe", "scripts/")
group ""

project "SpyroReignitedTrilogy.WidescreenFix"
   setpaths("SPYRO_REIGNITED_TRILOGY_DIR", "Falcon/Binaries/Win64/Spyro-Win64-Shipping.exe", "Falcon/Binaries/Win64/scripts/")

project "RedDeadRedemption.FusionFix"
   add_kananlib()
   setpaths("RED_DEAD_REDEMPTION_DIR", "RDR.exe", "plugins/")
group ""

-- ====================== PCSX2F SOLUTION ======================
CommonWorkspaceSetup("Win32", "PCSX2F")

group ""
project "Burnout3.PCSX2F.WidescreenFix"
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "Burnout3.PCSX2F.WidescreenFix")
   writemakefile_ps2("Burnout3.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("Burnout3.PCSX2F.WidescreenFix")

group "GrandTheftAuto"
project "GTALCS.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "Burnout3.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PCSX2F.WidescreenFix")
   writemakefile_ps2("GTALCS.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "lodl.o", "cpad.o", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTALCS.PCSX2F.WidescreenFix")

project "GTAVCS.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "GTALCS.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.WidescreenFix")
   writemakefile_ps2("GTAVCS.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a", "cpad.o", "ckey.o", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.WidescreenFix")

project "GTAVCS.PCSX2F.Project2DFX"
   kind "Makefile"
   dependson { "GTAVCS.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.Project2DFX")
   writemakefile_ps2("GTAVCS.PCSX2F.Project2DFX", "PLUGINS/", "0x03100000", "-l:libc.a", "lodl.o", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.Project2DFX")

project "GTAVCS.PCSX2F.ImVehLM"
   kind "Makefile"
   dependson { "GTAVCS.PCSX2F.Project2DFX" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.ImVehLM")
   writemakefile_ps2("GTAVCS.PCSX2F.ImVehLM", "PLUGINS/", "0x06000000", "-l:libc.a", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.ImVehLM")
group ""

project "KnightRider.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "GTAVCS.PCSX2F.ImVehLM" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "KnightRider.PCSX2F.WidescreenFix")
   writemakefile_ps2("KnightRider.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("KnightRider.PCSX2F.WidescreenFix")

project "PCSX2F.XboxRainDroplets"
   kind "Makefile"
   dependson { "KnightRider.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "PCSX2F.XboxRainDroplets")
   writemakefile_ps2("PCSX2F.XboxRainDroplets", "PLUGINS/", "0x03F00000", "-l:libc.a", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("PCSX2F.XboxRainDroplets")

project "SplinterCellDoubleAgent.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "PCSX2F.XboxRainDroplets" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SplinterCellDoubleAgent.PCSX2F.WidescreenFix")
   writemakefile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix")

project "TrueCrimeNewYorkCity.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "SplinterCellDoubleAgent.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("PCSX2F_DIR", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "TrueCrimeNewYorkCity.PCSX2F.WidescreenFix")
   writemakefile_ps2("TrueCrimeNewYorkCity.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("TrueCrimeNewYorkCity.PCSX2F.WidescreenFix")
group ""

-- ====================== PPSSPP SOLUTION ======================
CommonWorkspaceSetup("Win32", "PPSSPP")

group ""
group "GrandTheftAuto"
project "GTALCS.PPSSPP.WidescreenFix"
   kind "Makefile"
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.WidescreenFix")
   writemakefile_psp("GTALCS.PPSSPP.WidescreenFix")

project "GTALCS.PPSSPP.Project2DFX"
   kind "Makefile"
   dependson { "GTALCS.PPSSPP.WidescreenFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.Project2DFX")
   writemakefile_psp("GTALCS.PPSSPP.Project2DFX", "lodl.c")

project "GTALCS.PPSSPP.ImVehLM"
   kind "Makefile"
   dependson { "GTALCS.PPSSPP.Project2DFX" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.ImVehLM")
   writemakefile_psp("GTALCS.PPSSPP.ImVehLM")

project "GTAVCS.PPSSPP.WidescreenFix"
   kind "Makefile"
   dependson { "GTALCS.PPSSPP.ImVehLM" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.WidescreenFix")
   writemakefile_psp("GTAVCS.PPSSPP.WidescreenFix")

project "GTAVCS.PPSSPP.Project2DFX"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.WidescreenFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.Project2DFX")
   writemakefile_psp("GTAVCS.PPSSPP.Project2DFX", "lodl.c")

project "GTAVCS.PPSSPP.ImVehLM"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.Project2DFX" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.ImVehLM")
   writemakefile_psp("GTAVCS.PPSSPP.ImVehLM")

project "GTAVCS.PPSSPP.GamepadIcons"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.ImVehLM" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.GamepadIcons")
   writemakefile_psp("GTAVCS.PPSSPP.GamepadIcons")

project "GTACTW.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.ImVehLM" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTACTW.PPSSPP.FusionFix")
   writemakefile_psp("GTACTW.PPSSPP.FusionFix")
group ""

project "MidnightClubLARemix.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "GTACTW.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "MidnightClubLARemix.PPSSPP.FusionFix")
   writemakefile_psp("MidnightClubLARemix.PPSSPP.FusionFix")

project "PPSSPP.XboxRainDroplets"
   kind "Makefile"
   dependson { "MidnightClubLARemix.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/PPSSPP.XboxRainDroplets/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "PPSSPP.XboxRainDroplets")
   writemakefile_psp("PPSSPP.XboxRainDroplets")

project "SplinterCellEssentials.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "PPSSPP.XboxRainDroplets" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SplinterCellEssentials.PPSSPP.FusionFix")
   writemakefile_psp("SplinterCellEssentials.PPSSPP.FusionFix")

group "SOCOM"
project "SOCOM.FireteamBravo.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SplinterCellEssentials.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo.PPSSPP.FusionFix")
   writemakefile_psp("SOCOM.FireteamBravo.PPSSPP.FusionFix")
project "SOCOM.FireteamBravo2.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SOCOM.FireteamBravo.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo2.PPSSPP.FusionFix")
   writemakefile_psp("SOCOM.FireteamBravo2.PPSSPP.FusionFix")
project "SOCOM.FireteamBravo3.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SOCOM.FireteamBravo2.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo3.PPSSPP.FusionFix")
   writemakefile_psp("SOCOM.FireteamBravo3.PPSSPP.FusionFix")
group ""

project "TheWarriors.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SOCOM.FireteamBravo3.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("PPSSPP_DIR", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "TheWarriors.PPSSPP.FusionFix")
   writemakefile_psp("TheWarriors.PPSSPP.FusionFix")
group ""

-- ====================== DOLPHIN SOLUTION ======================
CommonWorkspaceSetup("x64", "Dolphin")

group ""
project "ResidentEvil2.RE3.Dolphin.FusionFix"
   add_kananlib()
   setpaths("DOLPHIN_DIR", "Dolphin.exe", "scripts/")
group ""

-- ====================== CXBXR SOLUTION ======================
CommonWorkspaceSetup("Win32", "CXBXR")

group ""
project "Mafia.CXBXR.WidescreenFix"
   setpaths("CXBXR_DIR", "cxbx.exe")
   files { "includes/cxbxr/cxbxr.h" }

project "SplinterCellDoubleAgent.CXBXR.WidescreenFix"
   setpaths("CXBXR_DIR", "cxbx.exe")
   files { "includes/cxbxr/cxbxr.h" }
group ""
