workspace "WidescreenFixesPack"
   configurations { "Release", "Debug" }
   platforms { "Win32" }
   architecture "x32"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   
   kind "SharedLib"
   language "C++"
   targetdir "data/%{prj.name}/scripts"
   targetextension ".asi"
   characterset ("MBCS")
   flags { "StaticRuntime" }
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://thirteenag.github.io/wfp\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/WidescreenFixesPack\"" }
   
   files { "source/%{prj.name}/*.cpp" }
   files { "Resources/*.rc" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   files { "includes/stdafx.h", "includes/stdafx.cpp" }
   includedirs { "includes" }
   includedirs { "external/hooking" }
   includedirs { "external/injector/include" }
   includedirs { "external/inireader" }
   
   pbcommands = { 
      "setlocal EnableDelayedExpansion",
	  --"set \"path=" .. (gamepath) .. "\"",
      "set file=$(TargetPath)",
      "FOR %%i IN (\"%file%\") DO (",
      "set filename=%%~ni",
      "set fileextension=%%~xi",
      "set target=!path!!filename!!fileextension!",
      "if exist \"!target!\" copy /y \"!file!\" \"!target!\"",
      ")" }

   function setpaths (gamepath, exepath, scriptspath)
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
   
   filter "configurations:Debug"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "On"


project "BGAE.WidescreenFix"
   setpaths("Z:/WFP/Games/Beyond Good and Evil/", "BGE.exe")
project "CallOfCthulhu.WidescreenFix"
   setpaths("Z:/WFP/Games/Call of Cthulhu/", "Engine/CoCMainWin32.exe", "Engine/scripts/")
project "ColdFear.WidescreenFix"
   setpaths("Z:/WFP/Games/ColdFear/", "ColdFear_retail.exe")
project "DeerAvenger4.WidescreenFix"
   setpaths("Z:/WFP/Games/Deer Avenger 4/", "DA4.exe")
project "DevilMayCry3.WidescreenFix"
   setpaths("Z:/WFP/Games/Devil May Cry 3/", "dmc3se.exe")
project "DriverParallelLines.WidescreenFix"
   setpaths("Z:/WFP/Games/Driver Parallel Lines/", "DriverParallelLines.exe")
project "Fahrenheit.WidescreenFix"
project "Flatout.WidescreenFix"
   setpaths("Z:/WFP/Games/Flatout/FlatOut/", "flatout.exe")
project "Flatout2.WidescreenFix"
   setpaths("Z:/WFP/Games/Flatout/FlatOut2/", "FlatOut2.exe")
project "FlatoutUltimateCarnage.WidescreenFix"
   setpaths("Z:/WFP/Games/Flatout/FlatOut - Ultimate Carnage/", "Fouc.exe")
project "GTA1.WidescreenFix"
   setpaths("Z:/WFP/Games/Grand Theft Auto/Grand Theft Auto 1/WINO/", "Grand Theft Auto.exe")
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
project "Gun.WidescreenFix"
   setpaths("Z:/WFP/Games/GUN/", "Gun.exe")
project "KingKong.WidescreenFix"
   setpaths("Z:/WFP/Games/King Kong/", "CheckApplication.exe")
project "Mafia.WidescreenFix"
   setpaths("Z:/WFP/Games/Mafia/", "Game.exe")
project "Manhunt.WidescreenFix"
   setpaths("Z:/WFP/Games/Manhunt/", "manhunt.exe")
project "MaxPayne.WidescreenFix"
   setpaths("Z:/WFP/Games/Max Payne/Max Payne/", "MaxPayne.exe")
project "MaxPayne2.WidescreenFix"
   setpaths("Z:/WFP/Games/Max Payne/Max Payne 2 The Fall of Max Payne/", "MaxPayne2.exe")
project "NFSCarbon.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Carbon/", "NFSC.exe")
project "NFSMostWanted.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need for Speed Most Wanted/", "speed.exe")
project "NFSUnderground.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need For Speed Underground/", "speed.exe")
project "NFSUnderground2.WidescreenFix"
   setpaths("Z:/WFP/Games/Need For Speed/Need For Speed Underground 2/", "speed2.exe")
project "Onimusha3.WidescreenFix"
   setpaths("Z:/WFP/Games/Onimusha 3/", "ONI3.exe")
project "ParadiseCracked.WidescreenFix"
   setpaths("Z:/WFP/Games/Paradise Cracked/", "game.exe")
project "PsiOpsTheMindgateConspiracy.WidescreenFix"
   setpaths("Z:/WFP/Games/PSI-OPS/", "PsiOps.exe")
project "SecondSight.WidescreenFix"
   setpaths("Z:/WFP/Games/Second Sight/", "secondsight.exe")
project "SilentHill2.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 2/", "sh2pc.exe")
project "SilentHill3.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 3/", "sh3.exe")
project "SilentHill4.WidescreenFix"
   setpaths("Z:/WFP/Games/Silent Hill/Silent Hill 4 The Room/", "Silent Hill 4.exe")
project "SniperElite.WidescreenFix"
   setpaths("Z:/WFP/Games/Sniper Elite/", "SniperElite.exe")
project "SonicHeroes.WidescreenFix"
   setpaths("Z:/WFP/Games/SONICHEROES/", "Tsonic_win.exe")
project "SplinterCell.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell/system/", "SplinterCell.exe", "system/scripts")
project "SplinterCellChaosTheory.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splintercell Chaos Theory/", "splintercell3.exe", "system/scripts")
project "SplinterCellDoubleAgent.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell - Double Agent/", "SCDA-Offline/System/SplinterCell4.exe", "SCDA-Offline/System/scripts")
project "SplinterCellPandoraTomorrow.WidescreenFix"
   setpaths("Z:/WFP/Games/Splinter Cell/Splinter Cell Pandora Tomorrow/", "offline/system/SplinterCell2.exe", "offline/system/scripts")
project "StreetRacingSyndicate.WidescreenFix"
   setpaths("Z:/WFP/Games/Street Racing Syndicate/", "Bin/srs.exe", "Bin/scripts")
project "StubbstheZombie.WidescreenFix"
   setpaths("Z:/WFP/Games/Stubbs the Zombie/", "Stubbs.exe")
project "ThePunisher.WidescreenFix"
   setpaths("Z:/WFP/Games/The Punisher/", "punisher.exe")
project "TonyHawksAmericanWasteland.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's American Wasteland/", "Game/THAW.exe", "Game/scripts")
project "TonyHawksProSkater2.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Pro Skater 2/", "THawk2.exe")
project "TonyHawksProSkater3.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Pro Skater 3/", "Skate3.exe")
project "TonyHawksProSkater4.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Pro Skater 4/", "Game/Skate4.exe", "Game/scripts")
project "TonyHawksUnderground.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's Underground/", "Game/THUG.exe", "Game/scripts")
project "TonyHawksUnderground2.WidescreenFix"
   setpaths("Z:/WFP/Games/Tony Hawks/Tony Hawk's Underground 2/", "Game/THUG2.exe", "Game/scripts")
project "TotalOverdose.WidescreenFix"
   setpaths("Z:/WFP/Games/Total Overdose/", "TOD.exe")
project "TrueCrimeNewYorkCity.WidescreenFix"
   setpaths("Z:/WFP/Games/True Crime New York City/", "True Crime New York City.exe")
project "TrueCrimeStreetsofLA.WidescreenFix"
   setpaths("Z:/WFP/Games/True Crime Streets of LA/", "TrueCrimeMB.exe")