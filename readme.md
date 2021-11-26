# Widescreen Fixes Pack

[![Build status](https://ci.appveyor.com/api/projects/status/ek3t29o34h5owslf?svg=true&passingText=AppVeyor+build+passing)](https://ci.appveyor.com/project/ThirteenAG/widescreenfixespack) [![Main Workflow](https://github.com/ThirteenAG/WidescreenFixesPack/actions/workflows/main.yml/badge.svg)](https://github.com/ThirteenAG/WidescreenFixesPack/actions/workflows/main.yml)

https://thirteenag.github.io/wfp

Plugins to make or improve widescreen resolutions support in PC games.

# Important notes

- Not compatible with Windows XP.
- If you're using WINE, you may want to check these issues: [#480](https://github.com/ThirteenAG/WidescreenFixesPack/issues/480#issuecomment-406734717), [#207](https://github.com/ThirteenAG/WidescreenFixesPack/issues/207).

# Building and Installing

Requirements:

- [Premake 5](https://premake.github.io/) _(pre-built executable available in this repository root)_
- [Visual Studio](http://www.visualstudio.com/downloads).

Run the following command in the root of this directory to generate the project files (or simply launch **premake5.bat**):

    premake5 vs2022

The usage is as simple as inserting the files into game's root directory. Uninstalling is as easy as that too, delete the files and you are done.

# List of fixes:

## GTA SA The Definitive Edition Fusion Mod

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adds an ability to quicksave anywhere by pressing F5.

[Download](http://thirteenag.github.io/wfp#gtasade)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTASADE.FusionMod/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTASADE.FusionMod/Gameface/Binaries/Win64/scripts/GTASADE.FusionMod.ini)

## GTA VC The Definitive Edition Fusion Mod

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adds an ability to quicksave anywhere by pressing F5.
![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adds an ability to disable first person aim mode for rifles.

[Download](http://thirteenag.github.io/wfp#gtavcde)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCDE.FusionMod/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTAVCDE.FusionMod/Gameface/Binaries/Win64/scripts/GTAVCDE.FusionMod.ini)

## GTA III The Definitive Edition Fusion Mod

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adds an ability to quicksave anywhere by pressing F5.
![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adds an ability to disable first person aim mode for rifles.

[Download](http://thirteenag.github.io/wfp#gta3de)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA3DE.FusionMod/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA3DE.FusionMod/Gameface/Binaries/Win64/scripts/GTA3DE.FusionMod.ini)

## Bully: Scholarship Edition Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View (Ultra-Wide)

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Unlocked all resolutions supported by the OS

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Fixed an issue when last selected resolution is reset upon startup

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

[Download](http://thirteenag.github.io/wfp#bully)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Bully.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Bully.WidescreenFix/plugins/Bully.WidescreenFix.ini)

## Burnout 3: Takedown Widescreen Fix [PCSX2]

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#burnout3)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Burnout3.PCSX2.WidescreenFix/dllmain.cpp)

## Call of Cthulhu Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Issues unfixed: [#420](https://github.com/ThirteenAG/WidescreenFixesPack/issues/420)

[Download](http://thirteenag.github.io/wfp#callofcthulhu)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/CallOfCthulhu.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/CallOfCthulhu.WidescreenFix/Engine/scripts/CallOfCthulhu.WidescreenFix.ini)

## Cold Fear Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

[Download](http://thirteenag.github.io/wfp#coldfear)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ColdFear.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/ColdFear.WidescreenFix/scripts/ColdFear.WidescreenFix.ini)

## Condemned: Criminal Origins Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution (Config.exe)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix stretched menu

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix low framerate

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix issue when controls aren't saved

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make windowed mode borderless

[Download](http://thirteenag.github.io/wfp#condemned)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Condemned.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Condemned.WidescreenFix/scripts/Condemned.WidescreenFix.ini)

## Deer Avenger 4 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Some rendering issues are visible in widescreen.

[Download](http://thirteenag.github.io/wfp#da4)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/DeerAvenger4.WidescreenFix/dllmain.cpp)

## DRIV3R Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase draw distance

[Download](http://thirteenag.github.io/wfp#driv3r)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Driv3r.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Driv3r.WidescreenFix/scripts/Driv3r.WidescreenFix.ini)

## Driver Parallel Lines Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

[Download](http://thirteenag.github.io/wfp#driverpl)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/DriverParallelLines.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/DriverParallelLines.WidescreenFix/scripts/DriverParallelLines.WidescreenFix.ini)

## Enter the Matrix Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#enterthematrix)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/EnterTheMatrix.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/EnterTheMatrix.WidescreenFix/scripts/EnterTheMatrix.WidescreenFix.ini)

## Far Cry Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

[Download](http://thirteenag.github.io/wfp#farcry)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/FarCry.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/FarCry.WidescreenFix/Bin32/FarCry.WidescreenFix.ini)

## FlatOut Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

[Download](http://thirteenag.github.io/wfp#flatout)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Flatout.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Flatout.WidescreenFix/scripts/Flatout.WidescreenFix.ini)

## FlatOut 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

[Download](http://thirteenag.github.io/wfp#flatout2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Flatout2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Flatout2.WidescreenFix/scripts/Flatout2.WidescreenFix.ini)

## FlatOut Ultimate Carnage Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Only supports different aspect ratios like 21:9

[Download](http://thirteenag.github.io/wfp#flatoutuc)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/FlatoutUltimateCarnage.WidescreenFix/dllmain.cpp)

## Grand Theft Auto (1) Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

[Download](http://thirteenag.github.io/wfp#gta1)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA1.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA1.WidescreenFix/WINO/scripts/GTA1.WidescreenFix.ini)

## Grand Theft Auto 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to quicksave via customizable hotkey

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) If you have the issue with broken graphics when driving at high speeds, try setting the ini parameter `CameraZoomFactor = auto` to a hardcoded value. It's currently calculated this way: [((AspectRatio) / (4.0 / 3.0)) \* 2.5](<https://www.google.com/search?q=((2560/1080)+/+(4.0+/+3.0))+*+2.5>). Find out what that value is for your resolution using this [link](<https://www.google.com/search?q=((2560/1080)+/+(4.0+/+3.0))+*+2.5>) (e.g. for `2560x1080`, `CameraZoomFactor = 4.44`) and set `CameraZoomFactor` to anything less than that (e.g. for `2560x1080`, `CameraZoomFactor = 3.8`).

[Download](http://thirteenag.github.io/wfp#gta2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA2.WidescreenFix/scripts/GTA2.WidescreenFix.ini)

## Grand Theft Auto III Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Many different options available in the ini file

[Download](http://thirteenag.github.io/wfp#gta3)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA3.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA3.WidescreenFix/scripts/GTA3.WidescreenFix.ini)

## Grand Theft Auto Vice City Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Many different options available in the ini file

[Download](http://thirteenag.github.io/wfp#gtavc)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVC.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTAVC.WidescreenFix/scripts/GTAVC.WidescreenFix.ini)

## Grand Theft Auto San Andreas Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Many different options available in the ini file

[Download](http://thirteenag.github.io/wfp#gtasa)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTASA.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTASA.WidescreenFix/scripts/GTASA.WidescreenFix.ini)

## Grand Theft Auto Liberty City Stories Widescreen Fix [PCSX2]

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Plugin automatically skips intro

[Download](http://thirteenag.github.io/wfp#gtalcs)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTALCS.PCSX2.WidescreenFix.WidescreenFix/dllmain.cpp)

## Grand Theft Auto Vice City Stories Widescreen Fix [PCSX2]

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Plugin automatically skips intro

[Download](http://thirteenag.github.io/wfp#gtavcs)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCS.PCSX2.WidescreenFix.WidescreenFix/dllmain.cpp)

## Gun Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#gun)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Gun.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Gun.WidescreenFix/scripts/Gun.WidescreenFix.ini)

## Hidden and Dangerous 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Damage overlay is disabled (not scaled properly)

[Download](http://thirteenag.github.io/wfp#hd2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/HiddenandDangerous2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/HiddenandDangerous2.WidescreenFix/scripts/HiddenandDangerous2.WidescreenFix.ini)

## Just Cause Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#justcause)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/JustCause.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/JustCause.WidescreenFix/scripts/JustCause.WidescreenFix.ini)

## King Kong Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase mouse sensitivity beyond the game's limit

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to hide untextured objects (**experimental, not recommended to use**)

[Download](http://thirteenag.github.io/wfp#kingkong)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KingKong.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/KingKong.WidescreenFix/scripts/KingKong.WidescreenFix.ini)

## Knight Rider: The Game Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#kr)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KnightRider.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/KnightRider.WidescreenFix/scripts/KnightRider.WidescreenFix.ini)

## Knight Rider: The Game 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#kr2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KnightRider2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/KnightRider2.WidescreenFix/scripts/KnightRider2.WidescreenFix.ini)

## L.A. Rush Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#larush)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/LARush.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/LARush.WidescreenFix/plugins/LARush.WidescreenFix.ini)

## Mafia: The City of Lost Heaven Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase draw distance

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change fps limit

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix issue with settings

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make windowed mode borderless

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to set deadzone for left stick and right stick on gamepad (otherwise camera just spins on its own)

[Download](http://thirteenag.github.io/wfp#mafia)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Mafia.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Mafia.WidescreenFix/scripts/Mafia.WidescreenFix.ini)

## Manhunt Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

[Download](http://thirteenag.github.io/wfp#manhunt)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Manhunt.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Manhunt.WidescreenFix/scripts/Manhunt.WidescreenFix.ini)

## Mass Effect Trilogy FOV Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#masseffect)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MassEffectTrilogy.FOVFix/dllmain.cpp)

## Max Payne Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make graphic novels fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to load save game automatically on startup

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to keep the game running on minimize

[Download](http://thirteenag.github.io/wfp#mp1)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MaxPayne.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/MaxPayne.WidescreenFix/scripts/MaxPayne.WidescreenFix.ini)

## Max Payne 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make graphic novels fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to load save game automatically on startup

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to keep the game running on minimize

[Download](http://thirteenag.github.io/wfp#mp2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MaxPayne2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/MaxPayne2.WidescreenFix/scripts/MaxPayne2.WidescreenFix.ini)

## Need for Speed Underground Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed black magazine covers

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to switch between different 3D scaling

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to improve gamepad support (gamepad icons, fixed bindings etc)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase deadzone for left stick

[Download](http://thirteenag.github.io/wfp#nfsu)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUnderground.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSUnderground.WidescreenFix/scripts/NFSUnderground.WidescreenFix.ini)

## Need for Speed Underground 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to switch between different 3D scaling

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to write registry settings to a file

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to improve gamepad support (gamepad icons, fixed bindings etc)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change fps limit

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to for 60fps cutscenes

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix optical drive bug

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase deadzone for left stick

[Download](http://thirteenag.github.io/wfp#nfsu2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUnderground2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSUnderground2.WidescreenFix/scripts/NFSUnderground2.WidescreenFix.ini)

## Need for Speed Most Wanted (2005) Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to switch between different 3D scaling

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase shadow resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix dynamic shadows

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to enable rearview mirror for all camera views

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to write registry settings to a file

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to improve gamepad support (gamepad icons, fixed bindings etc)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable motion blur

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to force 44100Hz sample rate audio

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase deadzone for left stick

[Download](http://thirteenag.github.io/wfp#nfsmw)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSMostWanted.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSMostWanted.WidescreenFix/scripts/NFSMostWanted.WidescreenFix.ini)

## Need for Speed Carbon Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to switch between different 3D scaling

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to enable windowed mode

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix crash after loading a profile

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to write registry settings to a file

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to improve gamepad support (gamepad icons, fixed bindings etc)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to expand controller configuration

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase deadzone for left stick

[Download](http://thirteenag.github.io/wfp#nfsc)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSCarbon.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSCarbon.WidescreenFix/scripts/NFSCarbon.WidescreenFix.ini)

## Need for Speed ProStreet Generic Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs (Ultra-Wide)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix resolution detection

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix a game breaking bug after completing a race

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to uncap framerate

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix various crashes

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase shadow resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to enable windowed mode

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to write registry settings to a file

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to improve gamepad support (gamepad icons, fixed bindings etc)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase deadzone for left stick

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable motion blur

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix brake lights

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix default gamma setting

[Download](http://thirteenag.github.io/wfp#nfsps)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSProStreet.GenericFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSProStreet.GenericFix/scripts/NFSProStreet.GenericFix.ini)

## Need for Speed Undercover Generic Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View (Ultra-Wide)

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed FMVs (Ultra-Wide)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix resolution detection

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to enable windowed mode

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change save game directory

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to write registry settings to a file

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to improve gamepad support (gamepad icons, fixed bindings etc)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase deadzone for left stick

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to set bloom intensity

[Download](http://thirteenag.github.io/wfp#nfsuc)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUndercover.GenericFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSUndercover.GenericFix/scripts/NFSUndercover.GenericFix.ini)

## Onimusha 3 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#onimusha3)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Onimusha3.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Onimusha3.WidescreenFix/scripts/Onimusha3.WidescreenFix.ini)

## Paradise Cracked Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

[Download](http://thirteenag.github.io/wfp#paradisecracked)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ParadiseCracked.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/ParadiseCracked.WidescreenFix/scripts/ParadiseCracked.WidescreenFix.ini)

## Psi-Ops: The Mindgate Conspiracy Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed FMVs [(#272)](https://github.com/ThirteenAG/WidescreenFixesPack/issues/272)

[Download](http://thirteenag.github.io/wfp#psiops)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/PsiOpsTheMindgateConspiracy.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/PsiOpsTheMindgateConspiracy.WidescreenFix/scripts/PsiOpsTheMindgateConspiracy.WidescreenFix.ini)

## Psychonauts Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#psychonauts)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Psychonauts.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Psychonauts.WidescreenFix/scripts/Psychonauts.WidescreenFix.ini)

## Second Sight Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#secondsight)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SecondSight.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SecondSight.WidescreenFix/scripts/SecondSight.WidescreenFix.ini)

## Silent Hill 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to set core affinity

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable safe mode

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make fast transitions

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to set fps limit

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change camera speed to match PS2 version

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix gamepad bindings

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix certain lighting issues

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase noise effect resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make background images fullscreen

[Download](http://thirteenag.github.io/wfp#sh2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SilentHill2.WidescreenFix/scripts/SilentHill2.WidescreenFix.ini)

## Silent Hill 3 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make FMVs fullscreen

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable safe mode

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added options to increase resolution of dynamic shadows and other effects

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase fog complexity

[Download](http://thirteenag.github.io/wfp#sh3)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill3.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SilentHill3.WidescreenFix/scripts/SilentHill3.WidescreenFix.ini)

## Silent Hill 4 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable cutscene borders

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase pause background resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix cutscene framerate

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable registry dependency

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to disable safe mode

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

[Download](http://thirteenag.github.io/wfp#sh4)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill4.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SilentHill4.WidescreenFix/scripts/SilentHill4.WidescreenFix.ini)

## Sniper Elite Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#sniperelite)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SniperElite.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SniperElite.WidescreenFix/scripts/SniperElite.WidescreenFix.ini)

## Sonic Heroes Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#sonicheroes)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SonicHeroes.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SonicHeroes.WidescreenFix/scripts/SonicHeroes.WidescreenFix.ini)

## Splinter Cell Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#sc)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCell.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCell.WidescreenFix/system/scripts/SplinterCell.WidescreenFix.ini)

## Splinter Cell Pandora Tomorrow Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase night vision resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change goggles' light color

[Download](http://thirteenag.github.io/wfp#scpt)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellPandoraTomorrow.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellPandoraTomorrow.WidescreenFix/offline/system/scripts/SplinterCellPandoraTomorrow.WidescreenFix.ini)

## Splinter Cell Chaos Theory Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change goggles' light color

[Download](http://thirteenag.github.io/wfp#scct)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellChaosTheory.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellChaosTheory.WidescreenFix/system/scripts/SplinterCellChaosTheory.WidescreenFix.ini)

## Splinter Cell Double Agent Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change goggles' light color

[Download](http://thirteenag.github.io/wfp#scda)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgent.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellDoubleAgent.WidescreenFix/SCDA-Offline/System/scripts/SplinterCellDoubleAgent.WidescreenFix.ini)

## Splinter Cell Double Agent V2 Widescreen Fix [PCSX2]

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#scdaps2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgentPS2.WidescreenFix/dllmain.cpp)

## Splinter Cell Double Agent V2 Widescreen Fix [CXBXR]

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#scdaxbox)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgent.CXBXR.WidescreenFix/dllmain.cpp)

## Street Racing Syndicate Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#srs)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/StreetRacingSyndicate.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/StreetRacingSyndicate.WidescreenFix/Bin/scripts/StreetRacingSyndicate.WidescreenFix.ini)

## Stubbs the Zombie Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Fixed OpenGL crash on startup

[Download](http://thirteenag.github.io/wfp#stubbsthezombie)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/StubbstheZombie.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/StubbstheZombie.WidescreenFix/scripts/StubbstheZombie.WidescreenFix.ini)

## The Punisher Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

[Download](http://thirteenag.github.io/wfp#punisher)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ThePunisher.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/ThePunisher.WidescreenFix/scripts/ThePunisher.WidescreenFix.ini)

## The Godfather Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#thegodfather)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheGodfather.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TheGodfather.WidescreenFix/scripts/TheGodfather.WidescreenFix.ini)

## The Matrix: Path of Neo Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#thematrixpathofneo)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheMatrixPathOfNeo.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TheMatrixPathOfNeo.WidescreenFix/scripts/TheMatrixPathOfNeo.WidescreenFix.ini)

## The Suffering Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

[Download](http://thirteenag.github.io/wfp#thesuffering)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheSuffering.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TheSuffering.WidescreenFix/scripts/TheSuffering.WidescreenFix.ini)

## Tony Hawk's American Wasteland Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to randomize songs order

[Download](http://thirteenag.github.io/wfp#thaw)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksAmericanWasteland.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TonyHawksAmericanWasteland.WidescreenFix/Game/scripts/TonyHawksAmericanWasteland.WidescreenFix.ini)

## Tony Hawk's Underground 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to randomize songs order

[Download](http://thirteenag.github.io/wfp#thug2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksUnderground2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TonyHawksUnderground2.WidescreenFix/Game/scripts/TonyHawksUnderground2.WidescreenFix.ini)

## Tony Hawk's Underground Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to randomize songs order

[Download](http://thirteenag.github.io/wfp#thug)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksUnderground.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TonyHawksUnderground.WidescreenFix/Game/scripts/TonyHawksUnderground.WidescreenFix.ini)

## Tony Hawk's Pro Skater 4 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to randomize songs order

[Download](http://thirteenag.github.io/wfp#thps4)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksProSkater4.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TonyHawksProSkater4.WidescreenFix/Game/scripts/TonyHawksProSkater4.WidescreenFix.ini)

## Tony Hawk's Pro Skater 3 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to randomize songs order

[Download](http://thirteenag.github.io/wfp#thps3)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksProSkater3.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TonyHawksProSkater3.WidescreenFix/scripts/TonyHawksProSkater3.WidescreenFix.ini)

## Tony Hawk's Pro Skater 2 Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#thps2)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksProSkater2.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TonyHawksProSkater2.WidescreenFix/scripts/TonyHawksProSkater2.WidescreenFix.ini)

## Total Overdose Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (Except menu)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs (Ver-)

[Download](http://thirteenag.github.io/wfp#tod)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TotalOverdose.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TotalOverdose.WidescreenFix/scripts/TotalOverdose.WidescreenFix.ini)

## True Crime Streets of LA Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/31/qm/gv/31qmgv6q0kj8zie1itat5ygfsuq.png) Fixed Field of View

[Download](http://thirteenag.github.io/wfp#truecrimesola)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TrueCrimeStreetsofLA.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TrueCrimeStreetsofLA.WidescreenFix/scripts/TrueCrimeStreetsofLA.WidescreenFix.ini)

## True Crime New York City Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

[Download](http://thirteenag.github.io/wfp#truecrimenyc)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TrueCrimeNewYorkCity.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/TrueCrimeNewYorkCity.WidescreenFix/scripts/TrueCrimeNewYorkCity.WidescreenFix.ini)

## Ultimate Spider-Man Widescreen Fix

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (partially, radar map has to be removed)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change fps limit

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Adjustable FOV via ini

[Download](http://thirteenag.github.io/wfp#usm)
|
[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/UltimateSpiderMan.WidescreenFix/dllmain.cpp)
|
[Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/UltimateSpiderMan.WidescreenFix/scripts/UltimateSpiderMan.WidescreenFix.ini)
