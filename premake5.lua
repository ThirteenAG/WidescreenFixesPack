workspace "WidescreenFixesPack"
   configurations { "Release", "Debug" }
   platforms { "Win32" }
   architecture "x32"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
   include "makefile.lua"
   
   kind "SharedLib"
   language "C++"
   targetdir "data/%{prj.name}/scripts"
   targetextension ".asi"
   characterset ("UNICODE")
   staticruntime "On"
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://thirteenag.github.io/wfp\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/WidescreenFixesPack\"" }
   
   files { "source/%{prj.name}/*.cpp" }
   files { "data/%{prj.name}/**" }
   files { "Resources/*.rc" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   files { "includes/stdafx.h", "includes/stdafx.cpp" }
   includedirs { "includes" }
   includedirs { "external/hooking" }
   includedirs { "external/injector/include" }
   includedirs { "external/inireader" }
   includedirs { "external/spdlog/include" }
   includedirs { "external/filewatch" }
   includedirs { "external/modutils" }
   
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
      --"set \"path=" .. (gamepath) .. "\"",
      "set file=$(TargetPath)",
      "FOR %%i IN (\"%file%\") DO (",
      "set filename=%%~ni",
      "set fileextension=%%~xi",
      "set target=!path!!filename!!fileextension!",
      "if exist \"!target!\" copy /y \"%%~fi\" \"!target!\"",
      ")" }

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
      local pbcmd = {}
      for k,v in pairs(pbcommands) do
        pbcmd[k] = v
      end
      if (gamepath) then
         cmdcopy = { "set \"path=" .. gamepath .. scriptspath .. "\"" }
         pbcmd[2] = "set \"file=../data/" .. prj_name .. "/" .. scriptspath .. prj_name ..".prx\""
         table.insert(cmdcopy, pbcmd)
         buildcommands   { "call " .. pspsdkpath .. " -C " .. sourcepath, cmdcopy }
         rebuildcommands { "call " .. pspsdkpath .. " -C " .. sourcepath .. " clean && " .. pspsdkpath .. " -C " .. sourcepath, cmdcopy }
         cleancommands   { "call " .. pspsdkpath .. " -C " .. sourcepath .. " clean" }
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
      local pbcmd = {}
      for k,v in pairs(pbcommands) do
        pbcmd[k] = v
      end
      if (gamepath) then
         cmdcopy = { "set \"path=" .. gamepath .. scriptspath .. "\"" }
         pbcmd[2] = "set \"file=../data/" .. prj_name .. "/" .. scriptspath .. prj_name ..".elf\""
         table.insert(cmdcopy, pbcmd)
         buildcommands   { "call " .. ps2sdkpath .. " -C " .. sourcepath, cmdcopy }
         rebuildcommands { "call " .. ps2sdkpath .. " -C " .. sourcepath .. " clean && " .. ps2sdkpath .. " -C " .. sourcepath, cmdcopy }
         cleancommands   { "call " .. ps2sdkpath .. " -C " .. sourcepath .. " clean" }
         debugdir (gamepath)
         if (exepath) then
            debugcommand (gamepath .. exepath)
            dir, file = exepath:match'(.*/)(.*)'
            debugdir (gamepath .. (dir or ""))
         end
      end
      targetdir ("data/" .. scriptspath)
   end
   
   function add_asmjit()
      files { "external/asmjit/src/**.cpp" }
      includedirs { "external/asmjit/src" }
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
   
   filter "configurations:Debug*"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release*"
      defines "NDEBUG"
      optimize "On"

project "Assembly64.TestApp"
   kind "ConsoleApp"
   targetextension ".exe"
   platforms { "Win64" }
   architecture "x64"
   setpaths("./data/%{prj.name}", "%{prj.name}.exe", "")
project "Assembly64.TestAsi"
   platforms { "Win64" }
   architecture "x64"
   add_asmjit()
   setpaths("./data/Assembly64.TestApp/", "Assembly64.TestApp.exe", "")
project "Bully.WidescreenFix"
   setpaths("Z:/WFP/Games/Bully Scholarship Edition/", "Bully.exe", "plugins/")
   writeghaction("bully", "Bully.WidescreenFix")
project "Burnout3.PCSX2F.WidescreenFix"
   kind "Makefile"
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2x64.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "Burnout3.PCSX2F.WidescreenFix")
   writemakefile_ps2("Burnout3.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("Burnout3.PCSX2F.WidescreenFix")
   writeghaction("burnout3", "Burnout3.PCSX2F.WidescreenFix")
project "CallOfCthulhu.WidescreenFix"
   setpaths("Z:/WFP/Games/Call of Cthulhu/", "Engine/CoCMainWin32.exe", "Engine/scripts/")
   writeghaction("callofcthulhu", "CallOfCthulhu.WidescreenFix")
project "ColdFear.WidescreenFix"
   setpaths("Z:/WFP/Games/ColdFear/", "ColdFear_retail.exe")
   writeghaction("coldfear", "ColdFear.WidescreenFix")
project "Condemned.WidescreenFix"
   setpaths("Z:/WFP/Games/Condemned Criminal Origins/", "Condemned.exe")
   writeghaction("condemned", "Condemned.WidescreenFix")
project "DeerAvenger4.WidescreenFix"
   setpaths("Z:/WFP/Games/Deer Avenger 4/", "DA4.exe")
   writeghaction("da4", "DeerAvenger4.WidescreenFix")
project "Driv3r.WidescreenFix"
   setpaths("Z:/WFP/Games/DRIV3R/", "driv3r.exe")
   writeghaction("driv3r", "Driv3r.WidescreenFix")
project "DriverParallelLines.WidescreenFix"
   setpaths("Z:/WFP/Games/Driver Parallel Lines/", "DriverParallelLines.exe")
   writeghaction("driverpl", "DriverParallelLines.WidescreenFix")
project "EnterTheMatrix.WidescreenFix"
   setpaths("Z:/WFP/Games/Enter the Matrix/", "Matrix.exe")
   writeghaction("enterthematrix", "EnterTheMatrix.WidescreenFix")
project "FarCry.WidescreenFix"
   setpaths("Z:/WFP/Games/Far Cry/", "Bin32/FarCry.exe", "Bin32/")
   writeghaction("farcry", "FarCry.WidescreenFix")
project "FarCry64.WidescreenFix"
   platforms { "Win64" }
   architecture "x64"
   files { "source/FarCry.WidescreenFix/*.cpp" }
   setpaths("Z:/WFP/Games/Far Cry/", "Bin64/FarCry.exe", "Bin64/")
   targetdir "data/FarCry.WidescreenFix/Bin64/"
project "Flatout.WidescreenFix"
   setpaths("Z:/WFP/Games/Flatout/FlatOut/", "flatout.exe")
   writeghaction("flatout", "Flatout.WidescreenFix")
project "Flatout2.WidescreenFix"
   setpaths("Z:/WFP/Games/Flatout/FlatOut2/", "FlatOut2.exe")
   writeghaction("flatout2", "Flatout2.WidescreenFix")
project "FlatoutUltimateCarnage.WidescreenFix"
   setpaths("Z:/WFP/Games/Flatout/FlatOut - Ultimate Carnage/", "Fouc.exe")
   writeghaction("flatoutuc", "FlatoutUltimateCarnage.WidescreenFix")
project "GettingUp.WidescreenFix"
   setpaths("Z:/WFP/Games/Marc Ecko's Getting Up 2/", "_Bin/GettingUp.exe", "_Bin/")
   targetdir "data/GettingUp.WidescreenFix/_Bin/"
   debugargs { "map=M01_HOO_Tranes_Hood_3" }
   buildoptions { "/Zc:threadSafeInit-" }
project "GTA1.WidescreenFix"
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto 1 London 1969 1961/", "WINO/Grand Theft Auto.exe", "WINO/scripts/")
   writeghaction("gta1", "GTA1.WidescreenFix")
project "GTA2.WidescreenFix"
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto 2/", "gta2.exe")
   writeghaction("gta2", "GTA2.WidescreenFix")
project "GTA3.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("Z:/WFP/Games/Grand Theft Auto/GTAIII/", "gta3.exe")
   writeghaction("gta3", "GTA3.WidescreenFix")
project "GTA3DE.FusionMod"
   platforms { "Win64" }
   architecture "x64"
   add_asmjit()
   setpaths("Z:/WFP/Games/Grand Theft Auto The Definitive Edition/GTA III - Definitive Edition/", "Gameface/Binaries/Win64/LibertyCity.exe", "Gameface/Binaries/Win64/scripts/")
   writeghaction("gta3de", "GTA3DE.FusionMod")
project "GTAVC.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto Vice City/", "gta-vc.exe")
   writeghaction("gtavc", "GTAVC.WidescreenFix")
project "GTAVCDE.FusionMod"
   platforms { "Win64" }
   architecture "x64"
   add_asmjit()
   setpaths("Z:/WFP/Games/Grand Theft Auto The Definitive Edition/GTA Vice City - Definitive Edition/", "Gameface/Binaries/Win64/ViceCity.exe", "Gameface/Binaries/Win64/scripts/")
   writeghaction("gtavcde", "GTAVCDE.FusionMod")
project "GTASA.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
   setpaths("Z:/WFP/Games/Grand Theft Auto/GTA San Andreas/", "gta_sa.exe")
   writeghaction("gtasa", "GTASA.WidescreenFix")
project "GTASADE.FusionMod"
   platforms { "Win64" }
   architecture "x64"
   add_asmjit()
   setpaths("Z:/WFP/Games/Grand Theft Auto The Definitive Edition/GTA San Andreas - Definitive Edition/", "Gameface/Binaries/Win64/SanAndreas.exe", "Gameface/Binaries/Win64/scripts/")
   writeghaction("gtasade", "GTASADE.FusionMod")
project "GTALCS.PCSX2F.WidescreenFix"
   kind "Makefile"
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2x64.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PCSX2F.WidescreenFix")
   writemakefile_ps2("GTALCS.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "lodl.o", "cpad.o", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTALCS.PCSX2F.WidescreenFix")
   writeghaction("gtalcs", "GTALCS.PCSX2F.WidescreenFix")
project "GTAVCS.PCSX2F.WidescreenFix"
   kind "Makefile"
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2x64.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.WidescreenFix")
   writemakefile_ps2("GTAVCS.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a", "cpad.o", "ckey.o", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.WidescreenFix")
   writeghaction("gtavcs", "GTAVCS.PCSX2F.WidescreenFix /t:GTAVCS.PCSX2F.Project2DFX")
project "GTAVCS.PCSX2F.Project2DFX"
   kind "Makefile"
   dependson { "GTAVCS.PCSX2F.WidescreenFix" }
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2x64.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PCSX2F.Project2DFX")
   writemakefile_ps2("GTAVCS.PCSX2F.Project2DFX", "PLUGINS/", "0x03100000", "-l:libc.a", "lodl.o", "../../includes/pcsx2/memalloc.o",
   "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o","../../includes/pcsx2/inireader.o",
   "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("GTAVCS.PCSX2F.Project2DFX")
project "GTALCS.PPSSPP.WidescreenFix"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.WidescreenFix")
   writemakefile_psp("GTALCS.PPSSPP.WidescreenFix")
   writeghaction("gtalcspsp", "GTALCS.PPSSPP.WidescreenFix /t:GTALCS.PPSSPP.Project2DFX /t:GTALCS.PPSSPP.ImVehLM")
project "GTALCS.PPSSPP.Project2DFX"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.Project2DFX")
   writemakefile_psp("GTALCS.PPSSPP.Project2DFX", "lodl.c")
project "GTALCS.PPSSPP.ImVehLM"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTALCS.PPSSPP.ImVehLM")
   writemakefile_psp("GTALCS.PPSSPP.ImVehLM")
project "GTAVCS.PPSSPP.WidescreenFix"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.WidescreenFix")
   writemakefile_psp("GTAVCS.PPSSPP.WidescreenFix")
   writeghaction("gtavcspsp", "GTAVCS.PPSSPP.WidescreenFix /t:GTAVCS.PPSSPP.Project2DFX /t:GTAVCS.PPSSPP.ImVehLM")
project "GTAVCS.PPSSPP.Project2DFX"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.Project2DFX")
   writemakefile_psp("GTAVCS.PPSSPP.Project2DFX", "lodl.c")
project "GTAVCS.PPSSPP.ImVehLM"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c", "source/%{prj.name}/*.cpp" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTAVCS.PPSSPP.ImVehLM")
   writemakefile_psp("GTAVCS.PPSSPP.ImVehLM")
project "GTACTW.PPSSPP.FusionMod"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/GTACTW.PPSSPP.FusionMod/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "GTACTW.PPSSPP.FusionMod")
   writemakefile_psp("GTACTW.PPSSPP.FusionMod")
   writeghaction("gtactwpsp", "GTACTW.PPSSPP.FusionMod")
project "GTASA.UWP.Test"
   setpaths("Z:/WFP/Games/GTASAUWP/", "GTASA.exe")
project "Gun.WidescreenFix"
   setpaths("Z:/WFP/Games/GUN/", "Gun.exe")
   writeghaction("gun", "Gun.WidescreenFix")
project "HiddenandDangerous2.WidescreenFix"
   setpaths("Z:/WFP/Games/Hidden and Dangerous 2/", "hd2.exe")
   writeghaction("hd2", "HiddenandDangerous2.WidescreenFix")
project "JustCause.WidescreenFix"
   setpaths("Z:/WFP/Games/Just Cause/", "JustCause.exe")
   writeghaction("justcause", "JustCause.WidescreenFix")
project "KingKong.WidescreenFix"
   setpaths("Z:/WFP/Games/King Kong/", "CheckApplication.exe")
   writeghaction("kingkong", "KingKong.WidescreenFix")
project "KnightRider.WidescreenFix"
   configurations { "Release", "Debug" }
      setpaths("Z:/WFP/Games/Knight Rider/", "Knight Rider.exe")
      writeghaction("kr", "KnightRider.WidescreenFix /t:KnightRider.PCSX2F.WidescreenFix")
project "KnightRider.PCSX2F.WidescreenFix"
   kind "Makefile"
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2x64.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "KnightRider.PCSX2F.WidescreenFix")
   writemakefile_ps2("KnightRider.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("KnightRider.PCSX2F.WidescreenFix")
project "KnightRider2.WidescreenFix"
   configurations { "Release", "Debug" }
      setpaths("Z:/WFP/Games/Knight Rider 2/", "KR2.exe")
      writeghaction("kr2", "KnightRider2.WidescreenFix")
project "LARush.WidescreenFix"
   setpaths("Z:/WFP/Games/LA Rush/", "LARush.exe", "plugins/")
   writeghaction("larush", "LARush.WidescreenFix")
project "Mafia.GenericFix"
   setpaths("Z:/WFP/Games/Mafia/", "Setup.exe")
   targetdir "data/Mafia.WidescreenFix/scripts"
project "Mafia.WidescreenFix"
   setpaths("Z:/WFP/Games/Mafia/", "GameV12.exe")
   writeghaction("mafia", "Mafia.WidescreenFix")
project "Mafia.CXBXR.WidescreenFix"
   configurations { "ReleaseCXBXR", "DebugCXBXR" }
      setpaths("Z:/WFP/Games/CXBXR/", "cxbx.exe")
      files { "includes/cxbxr/cxbxr.h" }
   writeghaction("mafiaxbox", "Mafia.CXBXR.WidescreenFix")
project "Manhunt.WidescreenFix"
   setpaths("Z:/WFP/Games/Manhunt/", "manhunt.exe")
   writeghaction("manhunt", "Manhunt.WidescreenFix")
project "MassEffectTrilogy.FOVFix"
   setpaths("Z:/WFP/Games/Mass Effect/Mass Effect 2/Binaries/", "MassEffect2.exe", "/")
   writeghaction("masseffect", "MassEffectTrilogy.FOVFix")
project "MaxPayne.WidescreenFix"
   setpaths("Z:/WFP/Games/Max Payne/Max Payne/", "MaxPayne.exe")
   writeghaction("mp1", "MaxPayne.WidescreenFix")
project "MidnightClubLARemix.PPSSPP.FusionMod"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionMod/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "MidnightClubLARemix.PPSSPP.FusionMod")
   writemakefile_psp("MidnightClubLARemix.PPSSPP.FusionMod")
project "MaxPayne2.WidescreenFix"
   setpaths("Z:/WFP/Games/Max Payne/Max Payne 2 The Fall of Max Payne/", "MaxPayne2.exe")
   writeghaction("mp2", "MaxPayne2.WidescreenFix")
project "NFSCarbon.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Carbon/", "NFSC.exe")
   writeghaction("nfsc", "NFSCarbon.WidescreenFix")
project "NFSMostWanted.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Most Wanted/", "speed.exe")
   writeghaction("nfsmw", "NFSMostWanted.WidescreenFix")
project "NFSProStreet.GenericFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed ProStreet/", "nfsps.exe")
   writeghaction("nfsps", "NFSProStreet.GenericFix")
project "NFSUndercover.GenericFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Undercover/", "nfs.exe")
   writeghaction("nfsuc", "NFSUndercover.GenericFix")
project "NFSUnderground.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need For Speed Underground/", "speed.exe")
   files { "textures/NFS/NFSU/icon.rc" }
   defines { "IDR_NFSUICON=200" }
   writeghaction("nfsu", "NFSUnderground.WidescreenFix")
project "NFSUnderground2.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need For Speed Underground 2/", "speed2.exe")
   writeghaction("nfsu2", "NFSUnderground2.WidescreenFix")
project "Onimusha3.WidescreenFix"
   setpaths("Z:/WFP/Games/Onimusha 3/", "ONI3.exe")
   writeghaction("onimusha3", "Onimusha3.WidescreenFix")
project "ParadiseCracked.WidescreenFix"
   setpaths("Z:/WFP/Games/Paradise Cracked/", "game.exe")
   writeghaction("paradisecracked", "ParadiseCracked.WidescreenFix")
project "PsiOpsTheMindgateConspiracy.WidescreenFix"
   setpaths("Z:/WFP/Games/PSI-OPS/", "PsiOps.exe")
   writeghaction("psiops", "PsiOpsTheMindgateConspiracy.WidescreenFix")
project "Psychonauts.WidescreenFix"
   setpaths("Z:/WFP/Games/Psychonauts/", "Psychonauts.exe")
   writeghaction("psychonauts", "Psychonauts.WidescreenFix")
project "ResidentEvil2.Dolphin.FusionMod"
   platforms { "Win64" }
   architecture "x64"
   add_asmjit()
   setpaths("Z:/WFP/Games/Dolphin-x64/", "Dolphin.exe", "scripts/")
   writeghaction("re2gc", "ResidentEvil2.Dolphin.FusionMod")
project "ResidentEvil3.Dolphin.FusionMod"
   platforms { "Win64" }
   architecture "x64"
   add_asmjit()
   setpaths("Z:/WFP/Games/Dolphin-x64/", "Dolphin.exe", "scripts/")
   writeghaction("re3gc", "ResidentEvil3.Dolphin.FusionMod")
project "Scarface.FusionFix"
   setpaths("Z:/WFP/Games/Scarface/", "scarface.exe", "scripts/")
   writeghaction("scarface", "Scarface.FusionFix")
project "SecondSight.WidescreenFix"
   setpaths("Z:/WFP/Games/Second Sight/", "secondsight.exe")
   writeghaction("secondsight", "SecondSight.WidescreenFix")
project "SilentHill2.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 2/", "sh2pc.exe")
   writeghaction("sh2", "SilentHill2.WidescreenFix")
project "SilentHill3.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 3/", "sh3.exe")
   writeghaction("sh3", "SilentHill3.WidescreenFix")
project "SilentHill4.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 4 The Room/", "Silent Hill 4.exe")
   writeghaction("sh4", "SilentHill4.WidescreenFix")
project "SniperElite.WidescreenFix"
   setpaths("Z:/WFP/Games/Sniper Elite/", "SniperElite.exe")
   writeghaction("sniperelite", "SniperElite.WidescreenFix")
project "SonicHeroes.WidescreenFix"
   setpaths("Z:/WFP/Games/SONICHEROES/", "Tsonic_win.exe")
   writeghaction("sonicheroes", "SonicHeroes.WidescreenFix")
project "SplinterCell.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell/", "system/SplinterCell.exe", "system/scripts/")
   writeghaction("sc", "SplinterCell.WidescreenFix")
project "SplinterCellChaosTheory.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splintercell Chaos Theory/", "system/splintercell3.exe", "system/scripts/")
   writeghaction("scct", "SplinterCellChaosTheory.WidescreenFix")
project "SplinterCellConviction.FusionMod"
   setpaths("Z:/WFP/Games/Splinter Cell/Tom Clancy's Splinter Cell Conviction/", "src/system/conviction_game.exe", "src/system/scripts/")
   writeghaction("scc", "SplinterCellConviction.FusionMod")
project "SplinterCellDoubleAgent.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell - Double Agent/", "SCDA-Offline/System/SplinterCell4.exe", "SCDA-Offline/System/scripts/")
   files { "textures/SCDA/icon.rc" }
   defines { "IDR_SCDAICON=200" }
   writeghaction("scda", "SplinterCellDoubleAgent.WidescreenFix")
project "SplinterCellDoubleAgent.PCSX2F.WidescreenFix"
   kind "Makefile"
   includedirs { "external/ps2sdk/ps2sdk/ee" }
   files { "source/%{prj.name}/*.h" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".elf"
   setbuildpaths_ps2("Z:/GitHub/PCSX2-Fork-With-Plugins/bin/", "pcsx2x64.exe", "PLUGINS/", "%{wks.location}/../external/ps2sdk/ee/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "SplinterCellDoubleAgent.PCSX2F.WidescreenFix")
   writemakefile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix", "PLUGINS/", "0x02100000", "-l:libc.a -l:libm.a -l:libgcc.a", "../../includes/pcsx2/log.o",
   "../../includes/pcsx2/memalloc.o", "../../includes/pcsx2/patterns.o", "../../includes/pcsx2/injector.o", "../../includes/pcsx2/rini.o",
   "../../includes/pcsx2/inireader.o", "../../includes/pcsx2/mips.o")
   writelinkfile_ps2("SplinterCellDoubleAgent.PCSX2F.WidescreenFix")
   writeghaction("scdaps2", "SplinterCellDoubleAgent.PCSX2F.WidescreenFix")
project "SplinterCellDoubleAgent.CXBXR.WidescreenFix"
   configurations { "ReleaseCXBXR", "DebugCXBXR" }
      setpaths("Z:/WFP/Games/CXBXR/", "cxbx.exe")
      files { "includes/cxbxr/cxbxr.h" }
   writeghaction("scdaxbox", "SplinterCellDoubleAgent.CXBXR.WidescreenFix")
project "SplinterCellPandoraTomorrow.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell Pandora Tomorrow/", "offline/system/SplinterCell2.exe", "offline/system/scripts/")
   writeghaction("scpt", "SplinterCellPandoraTomorrow.WidescreenFix")
project "SplinterCellEssentials.PPSSPP.FusionMod"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionMod/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "SplinterCellEssentials.PPSSPP.FusionMod")
   writemakefile_psp("SplinterCellEssentials.PPSSPP.FusionMod")
   writeghaction("sce", "SplinterCellEssentials.PPSSPP.FusionMod")
project "StreetRacingSyndicate.WidescreenFix"
   setpaths("Z:/WFP/Games/Street Racing Syndicate/", "Bin/srs.exe", "Bin/scripts/")
   writeghaction("srs", "StreetRacingSyndicate.WidescreenFix")
project "StubbstheZombie.WidescreenFix"
   setpaths("Z:/WFP/Games/Stubbs the Zombie/", "Stubbs.exe")
   writeghaction("stubbsthezombie", "StubbstheZombie.WidescreenFix")
project "TheGodfather.WidescreenFix"
   setpaths("Z:/WFP/Games/The Godfather/", "godfather.exe")
   writeghaction("thegodfather", "TheGodfather.WidescreenFix")
project "TheMatrixPathOfNeo.WidescreenFix"
   setpaths("Z:/WFP/Games/The Matrix - Path Of Neo/", "Matrix3.exe")
   writeghaction("thematrixpathofneo", "TheMatrixPathOfNeo.WidescreenFix")
project "ThePunisher.WidescreenFix"
   setpaths("Z:/WFP/Games/The Punisher/", "pun.exe")
   writeghaction("punisher", "ThePunisher.WidescreenFix")
project "TheSuffering.WidescreenFix"
   setpaths("Z:/WFP/Games/The Suffering/The Suffering/", "suffering.exe")
   writeghaction("thesuffering", "TheSuffering.WidescreenFix")
project "TheWarriors.PPSSPP.FusionMod"
   kind "Makefile"
   includedirs { "external/pspsdk/psp/sdk/include" }
   includedirs { "external/pspsdk/bin" }
   includedirs { "external/pspsdk/psp/sdk/include" }
   files { "source/%{prj.name}/*.c" }
   targetextension ".prx"
   setbuildpaths_psp("Z:/WFP/Games/PPSSPP/", "PPSSPPWindows64.exe", "memstick/PSP/PLUGINS/TheWarriors.PPSSPP.FusionMod/", "%{wks.location}/../external/pspsdk/bin/vsmake", "%{wks.location}/../source/%{prj.name}/", "TheWarriors.PPSSPP.FusionMod")
   writemakefile_psp("TheWarriors.PPSSPP.FusionMod")
   writeghaction("thewarriors", "TheWarriors.PPSSPP.FusionMod")
project "TonyHawksAmericanWasteland.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's American Wasteland/", "Game/THAW.exe", "Game/scripts/")
   writeghaction("thaw", "TonyHawksAmericanWasteland.WidescreenFix")
project "TonyHawksProSkater2.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/THPS2/", "THawk2.exe")
   writeghaction("thps2", "TonyHawksProSkater2.WidescreenFix")
project "TonyHawksProSkater3.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Pro Skater 3/", "Skate3.exe")
   writeghaction("thps3", "TonyHawksProSkater3.WidescreenFix")
project "TonyHawksProSkater4.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawks Pro Skater 4/", "Game/Skate4.exe", "Game/scripts/")
   writeghaction("thps4", "TonyHawksProSkater4.WidescreenFix")
project "TonyHawksUnderground.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's Underground/", "Game/THUG.exe", "Game/scripts/")
   writeghaction("thug", "TonyHawksUnderground.WidescreenFix")
project "TonyHawksUnderground2.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's Underground 2/", "Game/THUG2.exe", "Game/scripts/")
   writeghaction("thug2", "TonyHawksUnderground2.WidescreenFix")
project "TotalOverdose.WidescreenFix"
   setpaths("Z:/WFP/Games/Total Overdose/", "TOD.exe")
   writeghaction("tod", "TotalOverdose.WidescreenFix")
project "TrueCrimeNewYorkCity.WidescreenFix"
   setpaths("Z:/WFP/Games/True Crime New York City/", "True Crime New York City.exe")
   writeghaction("truecrimenyc", "TrueCrimeNewYorkCity.WidescreenFix")
project "TrueCrimeStreetsofLA.WidescreenFix"
   setpaths("Z:/WFP/Games/True Crime Streets of LA/", "TrueCrimeMB.exe")
   writeghaction("truecrimesola", "TrueCrimeStreetsofLA.WidescreenFix")
project "UltimateSpiderMan.WidescreenFix"
   setpaths("Z:/WFP/Games/Ultimate Spider-Man/", "USM.exe")
   writeghaction("usm", "UltimateSpiderMan.WidescreenFix")
