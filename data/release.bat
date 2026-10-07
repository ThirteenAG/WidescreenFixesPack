echo off
setlocal enabledelayedexpansion
pushd "%~dp0" || exit /b 1
RD /S /Q ".\Archives"

rem Embedding PDBs
call EmbedPDB.bat

powershell -NoProfile -ExecutionPolicy Bypass -File "Sign.ps1" ^
    -SearchPaths ".\*.asi .\MaxPayne.WidescreenFix\MSVCP60.dll" ^
    -MaxParallel 8

if %errorlevel% neq 0 (
    echo ERROR: Signing failed!
    goto packaging_failed
)

rem Copying asi loader
FOR /R ".\" %%F IN (*.ual) DO (
findstr /c:"loadfromscriptsonly" "%%F" >nul 2>&1
if errorlevel 1 (
    echo String not found...
) else (
   SET filepath=%%F
   SET dll=!filepath:.ual=.dll!
   ECHO !dll!
   7za e -so "..\Ultimate-ASI-Loader.zip" *.dll -r > "!dll!"
   if errorlevel 1 goto packaging_failed
   for %%D in ("!dll!") do if %%~zD EQU 0 goto packaging_failed
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
   7za e -so "..\Ultimate-ASI-Loader_x64.zip" *.dll -r > "!dll!"
   if errorlevel 1 goto packaging_failed
   for %%D in ("!dll!") do if %%~zD EQU 0 goto packaging_failed
)
)

rem Additional files

rem Manhunt Widescreen Fix
copy /b/v/y "..\source\Manhunt.WidescreenFix\bin\Manhunt.WidescreenFix.ini" ".\Manhunt.WidescreenFix\scripts\Manhunt.WidescreenFix.ini"

rem dgVoodoo
7za e -so "..\dgVoodoo2.zip" "MS\x86\DDraw.dll" > ".\KnightRider.WidescreenFix\DDraw.dll"
if errorlevel 1 goto packaging_failed
7za e -so "..\dgVoodoo2.zip" "MS\x86\D3DImm.dll" > ".\KnightRider.WidescreenFix\D3DImm.dll"
if errorlevel 1 goto packaging_failed

7za e -so "..\dgVoodoo2.zip" "MS\x86\DDraw.dll" > ".\KnightRider2.WidescreenFix\DDraw.dll"
if errorlevel 1 goto packaging_failed
7za e -so "..\dgVoodoo2.zip" "MS\x86\D3DImm.dll" > ".\KnightRider2.WidescreenFix\D3DImm.dll"
if errorlevel 1 goto packaging_failed

7za e -so "..\dgVoodoo2.zip" "MS\x86\D3D8.dll" > ".\SplinterCell.WidescreenFix\system\d3d8.dll"
if errorlevel 1 goto packaging_failed

rem Xidi
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\Condemned.WidescreenFix\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\Scarface.FusionFix\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCell.WidescreenFix\system\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellPandoraTomorrow.WidescreenFix\system\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellChaosTheory.WidescreenFix\System\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput8.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\SplinterCellConviction.FusionFix\src\system\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/winmm.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\KingKong.WidescreenFix\" -y
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\MaxPayne.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
move /Y ".\MaxPayne.WidescreenFix\scripts\dinput.dll" ".\MaxPayne.WidescreenFix\dinputHooked.dll"
if errorlevel 1 goto packaging_failed
move /Y ".\MaxPayne.WidescreenFix\scripts\Xidi.32.dll" ".\MaxPayne.WidescreenFix\Xidi.32.dll"
if errorlevel 1 goto packaging_failed
7za e "..\xidi.zip" "Xidi-*/Win32/dinput.dll" "Xidi-*/Win32/Xidi.32.dll" -o".\MaxPayne2.WidescreenFix\" -y
if errorlevel 1 goto packaging_failed

rem dxwrapper
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\TonyHawksProSkater4.WidescreenFix\Game\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\TheSuffering.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\ThePunisher.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\MaxPayne.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\MaxPayne2.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\TrueCrimeNewYorkCity.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\DriverParallelLines.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
7za e "..\dxwrapper.zip" "dxwrapper.asi" -o".\Driv3r.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed

rem dxwrapper-scda
7za e "..\dxwrapper-scda.zip" "dxwrapper.dll" -o".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\scripts\" -y
if errorlevel 1 goto packaging_failed
move /Y ".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\scripts\dxwrapper.dll" ".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\scripts\dxwrapper.asi"
if errorlevel 1 goto packaging_failed

7za e "..\dxwrapper-scda.zip" "dxwrapper.dll" -o".\KingKong.WidescreenFix\scripts\" -y
if errorlevel 1 goto packaging_failed
move /Y ".\KingKong.WidescreenFix\scripts\dxwrapper.dll" ".\KingKong.WidescreenFix\scripts\dxwrapper.asi"
if errorlevel 1 goto packaging_failed

rem DSOAL
7za e "..\DSOAL.zip" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\MaxPayne.WidescreenFix\" -y
if errorlevel 1 goto packaging_failed
7za e "..\DSOAL.zip" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\MaxPayne2.WidescreenFix\" -y
if errorlevel 1 goto packaging_failed
7za e "..\DSOAL.zip" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\SplinterCell.WidescreenFix\system\" -y
if errorlevel 1 goto packaging_failed
7za e "..\DSOAL.zip" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\SplinterCellPandoraTomorrow.WidescreenFix\system\" -y
if errorlevel 1 goto packaging_failed
7za e "..\DSOAL.zip" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\SplinterCellChaosTheory.WidescreenFix\System\" -y
if errorlevel 1 goto packaging_failed
7za e "..\DSOAL.zip" "DSOAL+HRTF/Win32/dsound.dll" "DSOAL+HRTF/Win32/dsoal-aldrv.dll" -o".\SplinterCellDoubleAgent.WidescreenFix\SCDA-Offline\System\" -y
if errorlevel 1 goto packaging_failed

rem Creating archives

FOR /d %%X IN (*) DO (
if /I not "%%X"=="Archives" (
call :package_directory "%%X"
if errorlevel 1 goto packaging_failed
)
)

rem Creating texture archives
if exist "..\textures\GTA3.WidescreenFrontend" 7za a "Archives\GTA3.WidescreenFrontend.zip" "..\textures\GTA3.WidescreenFrontend"
if errorlevel 1 goto packaging_failed
if exist "..\textures\GTAVC.WidescreenFrontend" 7za a "Archives\GTAVC.WidescreenFrontend.zip" "..\textures\GTAVC.WidescreenFrontend"
if errorlevel 1 goto packaging_failed
if exist "..\textures\Manhunt.WidescreenFrontend" 7za a "Archives\Manhunt.WidescreenFrontend.zip" "..\textures\Manhunt.WidescreenFrontend"
if errorlevel 1 goto packaging_failed
popd
exit /b 0

:packaging_failed
echo ERROR: Packaging failed. No release should be uploaded.
popd
exit /b 1

:package_directory
rem 7-Zip exclusion markers contain !, so expand them without delayed expansion.
setlocal DisableDelayedExpansion
set "elf_exclusion="
echo "%~1" | findstr /I /C:"PPSSPP" >nul
if not errorlevel 1 set "elf_exclusion=-x!*.elf"
7za a -tzip "Archives\%~1.zip" ".\%~1\*" -r -xr!Archives -x!*.pdb -x!*.db -x!*.ipdb -x!*.iobj -x!*.tmp -x!*.ual -x!*.x64ual -x!*.wrapper -x!*.lib -x!*.exp -x!*.ilk -xr!*.objects -x!*.map -x!*.gitkeep %elf_exclusion%
exit /b %errorlevel%

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

