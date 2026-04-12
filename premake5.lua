function setpaths(gamepath, exepath, scriptspath)
   scriptspath = scriptspath or "scripts/"
   if (gamepath) then
      cmdcopy = { "set \"path=" .. gamepath .. scriptspath .. "\"" }
      table.insert(cmdcopy, pbcommands)
      postbuildcommands (cmdcopy)
      debugdir (gamepath)
      if (exepath) then
         debugcommand (gamepath .. exepath)
         dir, file = exepath:match'(.*/)(.*)'
         debugdir (gamepath .. (dir or ""))
      end
   end
   targetdir ("data/%{prj.name}/" .. scriptspath)
end

function setbuildpaths_psp(gamepath, exepath, scriptspath, pspsdkpath, sourcepath, prj_name)
   if (gamepath) then
     buildcommands {"setlocal EnableDelayedExpansion"}
     rebuildcommands {"setlocal EnableDelayedExpansion"}
     local ppsspppath = os.getenv "PPSSPPMemstick"
     if (ppsspppath == nil) then
         buildcommands {"set _PPSSPPMemstick=" .. gamepath .. "memstick/PSP"}
         rebuildcommands {"set _PPSSPPMemstick=" .. gamepath .. "memstick/PSP"}
     else
         buildcommands {"set _PPSSPPMemstick=!PPSSPPMemstick!"}
         rebuildcommands {"set _PPSSPPMemstick=!PPSSPPMemstick!"}
     end
      
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
        debugcommand (gamepath .. exepath)
        dir, file = exepath:match'(.*/)(.*)'
        debugdir (gamepath .. (dir or ""))
     end
   end
   targetdir ("data/%{prj.name}/" .. scriptspath)
end

function setbuildpaths_ps2(gamepath, exepath, scriptspath, ps2sdkpath, sourcepath, prj_name)
   if (gamepath) then
     buildcommands {"setlocal EnableDelayedExpansion"}
     rebuildcommands {"setlocal EnableDelayedExpansion"}
     local pcsx2fpath = os.getenv "PCSX2FDir"
     if (pcsx2fpath == nil) then
         buildcommands {"set _PCSX2FDir=" .. gamepath}
         rebuildcommands {"set _PCSX2FDir=" .. gamepath}
     else
         buildcommands {"set _PCSX2FDir=!PCSX2FDir!"}
         rebuildcommands {"set _PCSX2FDir=!PCSX2FDir!"}
     end
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
         debugcommand (gamepath .. exepath)
         dir, file = exepath:match'(.*/)(.*)'
         debugdir (gamepath .. (dir or ""))
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
      
      local dxsdk = os.getenv "DXSDK_DIR"
      if dxsdk then
         includedirs { dxsdk .. "/include" }
         libdirs { dxsdk .. "/lib/x86" }
      elseif os.isdir("external/minidx9") then
         includedirs { "external/minidx9/Include" }
         libdirs { "external/minidx9/Lib/x86" }
      else
         includedirs { "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/include" }
         libdirs { "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/lib/x86" }
      end
      
      pbcommands = {
         "setlocal EnableDelayedExpansion",
         "set file=$(TargetPath)",
         "FOR %%i IN (\"%file%\") DO (",
         "set filename=%%~ni",
         "set fileextension=%%~xi",
         "set target=!path!!filename!!fileextension!",
         "if exist \"!target!\" copy /y \"%%~fi\" \"!target!\"",
         ")" 
      }

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
      filter { "platforms:x64" }
         architecture "x64"
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
   setpaths("Z:/WFP/Games/Bully Scholarship Edition/", "Bully.exe", "plugins/")

project "CallOfCthulhu.WidescreenFix"
   setpaths("Z:/WFP/Games/Call of Cthulhu/", "Engine/CoCMainWin32.exe", "Engine/scripts/")

project "ColdFear.WidescreenFix"
   setpaths("Z:/WFP/Games/ColdFear/", "ColdFear_retail.exe")

project "Condemned.WidescreenFix"
   setpaths("Z:/WFP/Games/Condemned Criminal Origins/", "Condemned.exe")

project "DeerAvenger4.WidescreenFix"
   setpaths("Z:/WFP/Games/Deer Avenger 4/", "DA4.exe")

project "Driv3r.WidescreenFix"
   setpaths("Z:/WFP/Games/DRIV3R/", "driv3r.exe")

project "DriverParallelLines.WidescreenFix"
   setpaths("Z:/WFP/Games/Driver Parallel Lines/", "DriverParallelLines.exe")

project "EnterTheMatrix.WidescreenFix"
   setpaths("Z:/WFP/Games/Enter the Matrix/", "Matrix.exe")

project "FarCry.WidescreenFix"
   setpaths("Z:/WFP/Games/Far Cry/", "Bin32/FarCry.exe", "Bin32/")

group "GrandTheftAuto"
project "GTA1.WidescreenFix"
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto 1 London 1969 1961/", "WINO/Grand Theft Auto.exe", "WINO/scripts/")
project "GTA2.WidescreenFix"
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto 2/", "gta2.exe")
project "GTA3.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("Z:/WFP/Games/Grand Theft Auto/GTAIII/", "gta3.exe")
project "GTAVC.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto Vice City/", "gta-vc.exe")
project "GTASA.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("Z:/WFP/Games/Grand Theft Auto/GTA San Andreas/", "gta_sa.exe")
group ""

project "Gun.WidescreenFix"
   setpaths("Z:/WFP/Games/GUN/", "Gun.exe")

project "HiddenandDangerous2.WidescreenFix"
   setpaths("Z:/WFP/Games/Hidden and Dangerous 2/", "hd2.exe")

project "JustCause.WidescreenFix"
   setpaths("Z:/WFP/Games/Just Cause/", "JustCause.exe")

project "KingKong.WidescreenFix"
   prebuildcommands {
      "for /R \"../source/%{prj.name}/\" %%f in (*.ps) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T ps_3_0 /nologo /E main /Fo \"../source/%{prj.name}/%%~nf.pso\" %%f)",
      "for /R \"../source/%{prj.name}/\" %%f in (*.vs) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T vs_3_0 /nologo /E main /Fo \"../source/%{prj.name}/%%~nf.vso\" %%f)"
   }
   files { "source/%{prj.name}/*.ps", "source/%{prj.name}/*.vs", "source/%{prj.name}/*.rc" }
   defines { "IDR_BLURPS=200" }
   defines { "IDR_BLURVS=201" }
   defines { "IDR_REMANANCEPS=202" }
   setpaths("Z:/WFP/Games/King Kong Gamers Edition/", "KingKong8.exe")

project "KnightRider.WidescreenFix"
   setpaths("Z:/WFP/Games/Knight Rider/", "Knight Rider.exe")

project "KnightRider2.WidescreenFix"
   setpaths("Z:/WFP/Games/Knight Rider 2/", "KR2.exe")

project "LARush.WidescreenFix"
   setpaths("Z:/WFP/Games/LA Rush/", "LARush.exe", "plugins/")

project "Mafia.FusionFix"
   setpaths("Z:/WFP/Games/Mafia/", "Setup.exe")
   targetdir "data/Mafia.WidescreenFix/scripts"

project "Mafia.WidescreenFix"
   setpaths("Z:/WFP/Games/Mafia/", "GameV12.exe")

project "Manhunt.WidescreenFix"
   buildoptions { "/Zc:strictStrings-" }
   includedirs { "source/%{prj.name}/inc" }
   files { "source/%{prj.name}/**/*.cpp" }
   removefiles { "includes/stdafx.h", "includes/stdafx.cpp" }
   setpaths("Z:/WFP/Games/Manhunt/", "manhunt.exe")

group "MaxPayne"
project "MaxPayne.MSVCP60Wrapper"
   setpaths("Z:/WFP/Games/Max Payne/Max Payne/", "MaxPayne.exe", "")
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
   libdirs { "includes/dxsdk/dx8" }
   setpaths("Z:/WFP/Games/Max Payne/Max Payne/", "MaxPayne.exe")
project "MaxPayne2.WidescreenFix"
   debugargs { "-skipstartup -developer -window -nodialog" }
   linkoptions { "/SAFESEH:NO" }
   libdirs { "includes/dxsdk/dx8" }
   setpaths("Z:/WFP/Games/Max Payne/Max Payne 2 The Fall of Max Payne/", "MaxPayne2.exe")
group ""

group "NeedForSpeed"
project "NFSTheRun.FusionFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed The Run/", "Need For Speed The Run.exe", "plugins/")
project "NFSCarbon.WidescreenFix"
   prebuildcommands {
   "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T fx_2_0 /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)"
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.ps", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=201" }
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Carbon/", "NFSC.exe")
project "NFSMostWanted.WidescreenFix"
   prebuildcommands {
   "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T fx_2_0 /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)"
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.ps", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=201" }
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Most Wanted/", "speed.exe")
project "NFSProStreet.FusionFix"
   prebuildcommands {
   "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T fx_2_0 /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)"
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.ps", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=201" }
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed ProStreet/", "nfsps.exe")
project "NFSUndercover.FusionFix"
   prebuildcommands {
   "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T fx_2_0 /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)"
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.ps", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=201" }
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Undercover/", "nfs.exe")
project "NFSUnderground.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need For Speed Underground/", "speed.exe")
   files { "textures/NFS/NFSU/icon.rc" }
   defines { "IDR_NFSUICON=200" }
project "NFSUnderground2.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need For Speed Underground 2/", "speed2.exe")
group ""

project "Onimusha3.WidescreenFix"
   setpaths("Z:/WFP/Games/Onimusha 3/", "ONI3.exe")
project "ParadiseCracked.WidescreenFix"
   setpaths("Z:/WFP/Games/Paradise Cracked/", "game.exe")
project "PsiOpsTheMindgateConspiracy.WidescreenFix"
   setpaths("Z:/WFP/Games/PSI-OPS/", "PsiOps.exe")
project "Psychonauts.WidescreenFix"
   setpaths("Z:/WFP/Games/Psychonauts/", "Psychonauts.exe")

group "ResidentEvil"
project "ResidentEvil0.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvil0/", "re0hd.exe", "scripts/")
project "ResidentEvil.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvil/", "bhd.exe", "scripts/")
project "ResidentEvilRevelations.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvilRevelations/", "rerev.exe", "scripts/")
project "ResidentEvilRevelations2.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvilRevelations2/", "rerev2.exe", "scripts/")
project "ResidentEvil5.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvil5/", "re5dx9.exe", "scripts/")
project "ResidentEvil6.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvil6/", "BH6.exe", "scripts/")
group ""

project "Scarface.FusionFix"
   setpaths("Z:/WFP/Games/Scarface/", "scarface.exe", "scripts/")

project "SecondSight.WidescreenFix"
   setpaths("Z:/WFP/Games/Second Sight/", "secondsight.exe")

group "SilentHill"
project "SilentHill2.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 2/", "sh2pc.exe")
project "SilentHill3.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 3/", "sh3.exe")
project "SilentHill4.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 4 The Room/", "Silent Hill 4.exe")
group ""

project "SniperElite.WidescreenFix"
   setpaths("Z:/WFP/Games/Sniper Elite/", "SniperElite.exe")

project "SonicHeroes.WidescreenFix"
   setpaths("Z:/WFP/Games/SONICHEROES/", "Tsonic_win.exe")

group "SplinterCell"
project "SplinterCell.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell/", "system/SplinterCell.exe", "system/scripts/")
project "SplinterCellChaosTheory.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splintercell Chaos Theory/", "system/splintercell3.exe", "system/scripts/")
project "SplinterCellConviction.FusionFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Tom Clancy's Splinter Cell Conviction/", "src/system/conviction_game.exe", "src/system/scripts/")
project "SplinterCellBlacklist.FusionFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell Blacklist/", "src/SYSTEM/Blacklist_DX11_game.exe", "src/system/scripts/")
project "SplinterCellDoubleAgent.WidescreenFix"
   prebuildcommands {
   "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T fx_2_0 /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)",
   "for /R \"../source/%{prj.name}/\" %%f in (*.ps) do (\"../includes/dxsdk/lib/x86/asm_shader.exe\" %%f \"../source/%{prj.name}/%%~nf.pso\")",
   "for /R \"../source/%{prj.name}/\" %%f in (*.vs) do (\"../includes/dxsdk/lib/x86/asm_shader.exe\" %%f \"../source/%{prj.name}/%%~nf.vso\")",
   }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.ps", "source/%{prj.name}/*.rc" }
   files { "textures/SCDA/icon.rc" }
   defines { "IDR_SCDAICON=200" }
   defines { "IDR_POSTFX=201" }
   defines { "IDR_SHADER_BB6378E1=202" }
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell - Double Agent/", "SCDA-Offline/System/SplinterCell4.exe", "SCDA-Offline/System/scripts/")
project "SplinterCellPandoraTomorrow.WidescreenFix"
   prebuildcommands {
   "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /Tps_1_1 /LD /Ewaterblend /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)",
   "for /R \"../source/%{prj.name}/\" %%f in (*.ps) do (\"../includes/dxsdk/lib/x86/asm_shader.exe\" %%f \"../source/%{prj.name}/%%~nf.pso\")",
   "for /R \"../source/%{prj.name}/\" %%f in (*.vs) do (\"../includes/dxsdk/lib/x86/asm_shader.exe\" %%f \"../source/%{prj.name}/%%~nf.vso\")",
   }
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc" }
   defines { "IDR_WATER_BLEND=200" }
   debugargs { "-uplay_steam_mode" }
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell Pandora Tomorrow/", "system/SplinterCell2.exe", "system/scripts/")
group ""

project "StreetRacingSyndicate.WidescreenFix"
   setpaths("Z:/WFP/Games/Street Racing Syndicate/", "Bin/srs.exe", "Bin/scripts/")

project "TheGodfather.WidescreenFix"
   setpaths("Z:/WFP/Games/The Godfather/", "godfather.exe")

project "TheMatrixPathOfNeo.WidescreenFix"
   setpaths("Z:/WFP/Games/The Matrix - Path Of Neo/", "Matrix3.exe")

project "ThePunisher.WidescreenFix"
   setpaths("Z:/WFP/Games/The Punisher/", "pun.exe")

project "TheSaboteur.FusionFix"
   setpaths("Z:/WFP/Games/The Saboteur/", "Saboteur.exe")

project "TheSuffering.WidescreenFix"
   setpaths("Z:/WFP/Games/The Suffering/The Suffering/", "suffering.exe")

group "TonyHawks"
project "TonyHawksAmericanWasteland.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's American Wasteland/", "Game/THAW.exe", "Game/scripts/")
project "TonyHawksProSkater2.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/THPS2/", "THawk2.exe")
project "TonyHawksProSkater3.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Pro Skater 3/", "Skate3.exe")
project "TonyHawksProSkater4.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawks Pro Skater 4/", "Game/Skate4.exe", "Game/scripts/")
project "TonyHawksUnderground.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's Underground/", "Game/THUG.exe", "Game/scripts/")
project "TonyHawksUnderground2.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's Underground 2/", "Game/THUG2.exe", "Game/scripts/")
group ""

project "TotalOverdose.WidescreenFix"
   setpaths("Z:/WFP/Games/Total Overdose/", "TOD.exe")

project "TrueCrimeNewYorkCity.WidescreenFix"
   prebuildcommands { "for /R \"../source/%{prj.name}/\" %%f in (*.fx) do (\"../includes/dxsdk/lib/x86/fxc.exe\" /T fx_2_0 /Fo \"../source/%{prj.name}/%%~nf.fxo\" %%f)" }
   includedirs {"Resources"}
   files { "source/%{prj.name}/*.fx", "source/%{prj.name}/*.rc" }
   defines { "IDR_POSTFX=200" }
   defines { "IDR_AREATEX=201" }
   defines { "IDR_SEARCHTEX=202" }
   setpaths("Z:/WFP/Games/True Crime New York City/", "True Crime New York City.exe")

project "TrueCrimeStreetsofLA.WidescreenFix"
   setpaths("Z:/WFP/Games/True Crime Streets of LA/", "TrueCrimeMB.exe")

project "UltimateSpiderMan.WidescreenFix"
   setpaths("Z:/WFP/Games/Ultimate Spider-Man/", "USM.exe")
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
   files { "source/FarCry.WidescreenFix/*.cpp" }
   setpaths("Z:/WFP/Games/Far Cry/", "Bin64/FarCry.exe", "Bin64/")
   targetdir "data/FarCry.WidescreenFix/Bin64/"

group "GrandTheftAuto"
project "GTA3DE.FusionFix"
   add_kananlib()
   setpaths("Z:/WFP/Games/Grand Theft Auto The Definitive Edition/GTA III - Definitive Edition/", "Gameface/Binaries/Win64/LibertyCity.exe", "Gameface/Binaries/Win64/scripts/")
project "GTAVCDE.FusionFix"
   add_kananlib()
   setpaths("Z:/WFP/Games/Grand Theft Auto The Definitive Edition/GTA Vice City - Definitive Edition/", "Gameface/Binaries/Win64/ViceCity.exe", "Gameface/Binaries/Win64/scripts/")
project "GTASADE.FusionFix"
   add_kananlib()
   setpaths("Z:/WFP/Games/Grand Theft Auto The Definitive Edition/GTA San Andreas - Definitive Edition/", "Gameface/Binaries/Win64/SanAndreas.exe", "Gameface/Binaries/Win64/scripts/")
group ""

group "ResidentEvil"
project "ResidentEvil4.FusionFix"
   setpaths("Z:/WFP/Games/ResidentEvil4/", "re4.exe", "scripts/")
group ""

project "SpyroReignitedTrilogy.WidescreenFix"
   setpaths("Z:/WFP/Games/Spyro Reignited Trilogy/", "Falcon/Binaries/Win64/Spyro-Win64-Shipping.exe", "Falcon/Binaries/Win64/scripts/")

project "RedDeadRedemption.FusionFix"
   add_kananlib()
   setpaths("Z:/WFP/Games/Red Dead Redemption/", "RDR.exe", "plugins/")
group ""

-- ====================== PCSX2F SOLUTION ======================
CommonWorkspaceSetup("Win32", "PCSX2F")

group ""
project "Burnout3.PCSX2F.WidescreenFix"
   kind "Makefile"
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "Burnout3.PCSX2F.WidescreenFix")
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
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PCSX2F.WidescreenFix")
   writemakefile_ps2("GTALCS.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "lodl.o", "cpad.o", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTALCS.PCSX2F.WidescreenFix")

project "GTAVCS.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "GTALCS.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.WidescreenFix")
   writemakefile_ps2("GTAVCS.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a", "cpad.o", "ckey.o", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.WidescreenFix")

project "GTAVCS.PCSX2F.Project2DFX"
   kind "Makefile"
   dependson { "GTAVCS.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.Project2DFX")
   writemakefile_ps2("GTAVCS.PCSX2F.Project2DFX", "PLUGINS/", "0x03100000", "-l:libc.a", "lodl.o", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.Project2DFX")

project "GTAVCS.PCSX2F.ImVehLM"
   kind "Makefile"
   dependson { "GTAVCS.PCSX2F.Project2DFX" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.ImVehLM")
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
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "KnightRider.PCSX2F.WidescreenFix")
   writemakefile_ps2("KnightRider.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("KnightRider.PCSX2F.WidescreenFix")

project "PCSX2F.XboxRainDroplets"
   kind "Makefile"
   dependson { "KnightRider.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "PCSX2F.XboxRainDroplets")
   writemakefile_ps2("PCSX2F.XboxRainDroplets", "PLUGINS/", "0x03F00000", "-l:libc.a", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("PCSX2F.XboxRainDroplets")

project "SplinterCellDoubleAgent.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "PCSX2F.XboxRainDroplets" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SplinterCellDoubleAgent.PCSX2F.WidescreenFix")
   writemakefile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix")

project "TrueCrimeNewYorkCity.PCSX2F.WidescreenFix"
   kind "Makefile"
   dependson { "SplinterCellDoubleAgent.PCSX2F.WidescreenFix" }
   add_ps2sdk()
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2-qtx64-clang.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "TrueCrimeNewYorkCity.PCSX2F.WidescreenFix")
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
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.WidescreenFix")
   writemakefile_psp("GTALCS.PPSSPP.WidescreenFix")

project "GTALCS.PPSSPP.Project2DFX"
   kind "Makefile"
   dependson { "GTALCS.PPSSPP.WidescreenFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.Project2DFX")
   writemakefile_psp("GTALCS.PPSSPP.Project2DFX", "lodl.c")

project "GTALCS.PPSSPP.ImVehLM"
   kind "Makefile"
   dependson { "GTALCS.PPSSPP.Project2DFX" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.ImVehLM")
   writemakefile_psp("GTALCS.PPSSPP.ImVehLM")

project "GTAVCS.PPSSPP.WidescreenFix"
   kind "Makefile"
   dependson { "GTALCS.PPSSPP.ImVehLM" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.WidescreenFix")
   writemakefile_psp("GTAVCS.PPSSPP.WidescreenFix")

project "GTAVCS.PPSSPP.Project2DFX"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.WidescreenFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.Project2DFX")
   writemakefile_psp("GTAVCS.PPSSPP.Project2DFX", "lodl.c")

project "GTAVCS.PPSSPP.ImVehLM"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.Project2DFX" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.ImVehLM")
   writemakefile_psp("GTAVCS.PPSSPP.ImVehLM")

project "GTAVCS.PPSSPP.GamepadIcons"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.ImVehLM" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.GamepadIcons")
   writemakefile_psp("GTAVCS.PPSSPP.GamepadIcons")

project "GTACTW.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "GTAVCS.PPSSPP.ImVehLM" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "GTACTW.PPSSPP.FusionFix")
   writemakefile_psp("GTACTW.PPSSPP.FusionFix")
group ""

project "MidnightClubLARemix.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "GTACTW.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "MidnightClubLARemix.PPSSPP.FusionFix")
   writemakefile_psp("MidnightClubLARemix.PPSSPP.FusionFix")

project "PPSSPP.XboxRainDroplets"
   kind "Makefile"
   dependson { "MidnightClubLARemix.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/PPSSPP.XboxRainDroplets/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "PPSSPP.XboxRainDroplets")
   writemakefile_psp("PPSSPP.XboxRainDroplets")

project "SplinterCellEssentials.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "PPSSPP.XboxRainDroplets" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SplinterCellEssentials.PPSSPP.FusionFix")
   writemakefile_psp("SplinterCellEssentials.PPSSPP.FusionFix")

group "SOCOM"
project "SOCOM.FireteamBravo.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SplinterCellEssentials.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo.PPSSPP.FusionFix")
   writemakefile_psp("SOCOM.FireteamBravo.PPSSPP.FusionFix")
project "SOCOM.FireteamBravo2.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SOCOM.FireteamBravo.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo2.PPSSPP.FusionFix")
   writemakefile_psp("SOCOM.FireteamBravo2.PPSSPP.FusionFix")
project "SOCOM.FireteamBravo3.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SOCOM.FireteamBravo2.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "SOCOM.FireteamBravo3.PPSSPP.FusionFix")
   writemakefile_psp("SOCOM.FireteamBravo3.PPSSPP.FusionFix")
group ""

project "TheWarriors.PPSSPP.FusionFix"
   kind "Makefile"
   dependson { "SOCOM.FireteamBravo3.PPSSPP.FusionFix" }
   add_pspsdk()
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/", "%{wks.location}/../external/pspsdk/vsmake.ps1", "%{wks.location}/../source/%{prj.name}/", "TheWarriors.PPSSPP.FusionFix")
   writemakefile_psp("TheWarriors.PPSSPP.FusionFix")
group ""

-- ====================== DOLPHIN SOLUTION ======================
CommonWorkspaceSetup("x64", "Dolphin")

group ""
project "ResidentEvil2.RE3.Dolphin.FusionFix"
   add_kananlib()
   setpaths("Z:/WFP/Games/Dolphin-x64/", "Dolphin.exe", "scripts/")
group ""

-- ====================== CXBXR SOLUTION ======================
CommonWorkspaceSetup("Win32", "CXBXR")

group ""
project "Mafia.CXBXR.WidescreenFix"
   setpaths("Z:/WFP/Games/CXBXR/", "cxbx.exe")
   files { "includes/cxbxr/cxbxr.h" }

project "SplinterCellDoubleAgent.CXBXR.WidescreenFix"
   setpaths("Z:/WFP/Games/CXBXR/", "cxbx.exe")
   files { "includes/cxbxr/cxbxr.h" }
group ""
