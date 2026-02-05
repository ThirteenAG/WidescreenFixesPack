echo off
RD /S /Q ".\Archives"
rem Copying asi loader
setlocal enabledelayedexpansion
FOR /R ".\" %%F IN (*.ual) DO (
findstr /c:"loadfromscriptsonly" "%%F" >nul 2>&1
if errorlevel 1 (
    echo String not found...
) else (
   SET filepath=%%F
   SET dll=!filepath:.ual=.dll!
   ECHO !dll!
   7za e -so "..\Ultimate-ASI-Loader.zip" *.dll -r > !dll!
)
)

FOR /R ".\" %%F IN (*.x64ual) DO (
findstr /c:"loadfromscriptsonly" "%%F" >nul 2>&1
if errorlevel 1 (
    echo String not found...
) else (
   SET filepath=%%F
   SET dll=!filepath:.x64ual=.dll!
   ECHO !dll!
   copy "..\..\Ultimate-ASI-Loader\bin\x64\Release\dinput8.dll" !dll!
   7za e -so "..\Ultimate-ASI-Loader_x64.zip" *.dll -r > !dll!
)
)

rem Additional files

rem Manhunt Widescreen Fix
copy /b/v/y "..\source\Manhunt.WidescreenFix\bin\Manhunt.WidescreenFix.ini" ".\Manhunt.WidescreenFix\scripts\Manhunt.WidescreenFix.ini"

rem dgVoodoo
7za e -so "..\dgVoodoo2.zip" "MS\x86\DDraw.dll" > ".\KnightRider.WidescreenFix\DDraw.dll"
7za e -so "..\dgVoodoo2.zip" "MS\x86\D3DImm.dll" > ".\KnightRider.WidescreenFix\D3DImm.dll"

7za e -so "..\dgVoodoo2.zip" "MS\x86\DDraw.dll" > ".\KnightRider2.WidescreenFix\DDraw.dll"
7za e -so "..\dgVoodoo2.zip" "MS\x86\D3DImm.dll" > ".\KnightRider2.WidescreenFix\D3DImm.dll"

7za e -so "..\dgVoodoo2.zip" "MS\x86\D3D8.dll" > ".\SplinterCell.WidescreenFix\system\d3d8.dll"

rem Xidi
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\Condemned.WidescreenFix\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\Scarface.FusionFix\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCell.WidescreenFix\system\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellPandoraTomorrow.WidescreenFix\system\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellChaosTheory.WidescreenFix\system\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellConviction.FusionMod\src\system\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/winmm.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\KingKong.WidescreenFix\" -y
7za e "..\xidi.zip" "Xidi-*/Win32/dinput.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\MaxPayne.WidescreenFix\scripts\" -y
move /Y ".\MaxPayne.WidescreenFix\scripts\dinput.dll" ".\MaxPayne.WidescreenFix\dinputHooked.dll"
move /Y ".\MaxPayne.WidescreenFix\scripts\Xidi.32.dll" ".\MaxPayne.WidescreenFix\Xidi.32.dll"

rem dxwrapper
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\Scarface.FusionFix\scripts\" -y
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\TonyHawksProSkater4.WidescreenFix\Game\scripts\" -y
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\TheSuffering.WidescreenFix\scripts\" -y
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\ThePunisher.WidescreenFix\scripts\" -y
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\MaxPayne.WidescreenFix\scripts\" -y
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\MaxPayne2.WidescreenFix\scripts\" -y
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\TrueCrimeNewYorkCity.WidescreenFix\scripts\" -y

rem dxwrapper-scda
7za e "..\dxwrapper-scda.zip" "dxwrapper.dll" -o".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\scripts\" -y
move /Y ".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\scripts\dxwrapper.dll" ".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\scripts\dxwrapper.asi"

7za e "..\dxwrapper-scda.zip" "dxwrapper.dll" -o".\KingKong.WidescreenFix\scripts\" -y
move /Y ".\KingKong.WidescreenFix\scripts\dxwrapper.dll" ".\KingKong.WidescreenFix\scripts\dxwrapper.asi"

rem DSOAL
7za e "..\DSOAL.7z" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\MaxPayne.WidescreenFix\" -y

rem Creating archives

FOR /D /r %%G in ("*PPSSPP*") DO (
 cd %%~nxG
 del /S *.elf
 cd ..
)

rem Additional texture archives
7za a "GTALCS.PPSSPP.ImVehLM\memstick\PSP\TEXTURES\ULUS10041\textures.zip" -mx=0 "..\textures\GTALCS\*" -x^^!buildps2.bat -x^^!texture_dump_alpha_scaler.py
7za a "GTAVCS.PPSSPP.ImVehLM\memstick\PSP\TEXTURES\ULUS10160\textures.zip" -mx=0 "..\textures\GTAVCS\*" -x^^!buildps2.bat -x^^!texture_dump_alpha_scaler.py
7za a "GTAVCS.PPSSPP.GamepadIcons\memstick\PSP\TEXTURES\ULUS10160\textures.zip" -mx=0 "..\textures\GTAVCS\*" -x^^!buildps2.bat -x^^!texture_dump_alpha_scaler.py

FOR /d %%X IN (*) DO (
7za a -tzip "Archives\%%X.zip" ".\%%X\*" -r -xr^^!Archives -x^^!*.pdb -x^^!*.db -x^^!*.ipdb -x^^!*.iobj -x^^!*.tmp -x^^!*.iobj -x^^!*.ual -x^^!*.x64ual -x^^!*.iobj -x^^!*.wrapper -x^^!*.lib -x^^!*.exp -x^^!*.ilk -x^^!*.map -x^^!*.gitkeep
)

rem Creating texture archives
if exist "..\textures\GTA3.WidescreenFrontend" 7za a "Archives\GTA3.WidescreenFrontend.zip" "..\textures\GTA3.WidescreenFrontend"
if exist "..\textures\GTAVC.WidescreenFrontend" 7za a "Archives\GTAVC.WidescreenFrontend.zip" "..\textures\GTAVC.WidescreenFrontend"
if exist "..\textures\Manhunt.WidescreenFrontend" 7za a "Archives\Manhunt.WidescreenFrontend.zip" "..\textures\Manhunt.WidescreenFrontend"
EXIT

7-Zip Extra
~~~~~~~~~~~
License for use and distribution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copyright (C) 1999-2016 Igor Pavlov.

7-Zip Extra files are under the GNU LGPL license.


Notes: 
  You can use 7-Zip Extra on any computer, including a computer in a commercial 
  organization. You don't need to register or pay for 7-Zip.


GNU LGPL information
--------------------

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You can receive a copy of the GNU Lesser General Public License from 
  http://www.gnu.org/

