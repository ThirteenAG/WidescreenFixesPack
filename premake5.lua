workspace "WidescreenFixesPack"
   configurations { "Release", "Debug" }
   platforms { "Win32" }
   architecture "x32"
   location "build"
   
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
   includedirs { "includes" }
   includedirs { "external/hooking" }
   includedirs { "external/injector/include" }
   includedirs { "external/inireader" }
   
   filter "configurations:Debug"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "On"


project "BGAE.WidescreenFix"
   targetdir "data/%{prj.name}/scripts"
project "CallOfCthulhu.WidescreenFix"
   targetdir "data/%{prj.name}/Engine/scripts"
project "ColdFear.WidescreenFix"
project "DeerAvenger4.WidescreenFix"
project "DevilMayCry3.WidescreenFix"
project "Fahrenheit.WidescreenFix"
project "Flatout.WidescreenFix"
project "Flatout2.WidescreenFix"
project "FlatoutUltimateCarnage.WidescreenFix"
project "GTA1.WidescreenFix"
project "GTA2.WidescreenFix"
project "GTA3.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
project "GTASA.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
project "GTAVC.WidescreenFix"
   files { "includes/GTA/*.h", "includes/GTA/*.cpp" }
project "Gun.WidescreenFix"
project "KingKong.WidescreenFix"
project "Mafia.WidescreenFix"
project "Manhunt.WidescreenFix"
project "MaxPayne.WidescreenFix"
project "MaxPayne2.WidescreenFix"
project "NFSCarbon.WidescreenFix"
project "NFSMostWanted.WidescreenFix"
project "NFSUnderground.WidescreenFix"
project "NFSUnderground2.WidescreenFix"
project "Onimusha3.WidescreenFix"
project "ParadiseCracked.WidescreenFix"
project "PsiOpsTheMindgateConspiracy.WidescreenFix"
project "SecondSight.WidescreenFix"
project "SilentHill2.WidescreenFix"
project "SilentHill3.WidescreenFix"
project "SilentHill4.WidescreenFix"
project "SniperElite.WidescreenFix"
project "SonicHeroes.WidescreenFix"
project "SplinterCell.WidescreenFix"
   targetdir "data/%{prj.name}/system/scripts"
project "SplinterCellChaosTheory.WidescreenFix"
   targetdir "data/%{prj.name}/system/scripts"
project "SplinterCellDoubleAgent.WidescreenFix"
   targetdir "data/%{prj.name}/SCDA-Offline/System/scripts"
project "SplinterCellPandoraTomorrow.WidescreenFix"
   targetdir "data/%{prj.name}/offline/system/scripts"
project "StreetRacingSyndicate.WidescreenFix"
   targetdir "data/%{prj.name}/Bin/scripts"
project "StubbstheZombie.WidescreenFix"
project "ThePunisher.WidescreenFix"
project "TonyHawksAmericanWasteland.WidescreenFix"
   targetdir "data/%{prj.name}/Game/scripts"
project "TonyHawksProSkater2.WidescreenFix"
project "TonyHawksProSkater3.WidescreenFix"
project "TonyHawksProSkater4.WidescreenFix"
   targetdir "data/%{prj.name}/Game/scripts/"
project "TonyHawksUnderground.WidescreenFix"
   targetdir "data/%{prj.name}/Game/scripts/"
project "TonyHawksUnderground2.WidescreenFix"
   targetdir "data/%{prj.name}/Game/scripts/"
project "TotalOverdose.WidescreenFix"
project "TrueCrimeNewYorkCity.WidescreenFix"
project "TrueCrimeStreetsofLA.WidescreenFix"