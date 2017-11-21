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
   copy "..\..\Ultimate-ASI-Loader\bin\x86\Release\dinput8.dll" !dll!
   SET "mu=%%~dpF\scripts\modupdater.asi"
   SET "mu2=%%~dpF\plugins\modupdater.asi"
   ECHO !mu!
   IF EXIST !mu! (copy "..\..\modupdater\bin\Release\modupdater.asi" !mu!) ELSE (copy "..\..\modupdater\bin\Release\modupdater.asi" !mu2!)
)
)

rem Additional files
FOR /R ".\" %%F IN (*.wrapper) DO (
findstr /c:"FPSLimit" "%%F" >nul 2>&1
if errorlevel 1 (
		findstr /c:"SetVertexShaderConstantHook" "%%F" >nul 2>&1
		if errorlevel 1 (
			echo String not found...
		) else (
		SET filepath=%%F
		SET dll=!filepath:.wrapper=.dll!
		ECHO !dll! 	
		copy "..\..\d3d8-wrapper\data\d3d8.dll" !dll!
)
) else (
   SET filepath=%%F
   SET dll=!filepath:.wrapper=.dll!
   ECHO !dll! 	
   copy "..\..\d3d9-wrapper\data\d3d9.dll" !dll!
)
)

rem Scarface PIX

copy "%DXSDK_DIR%\Utilities\bin\x86\Detoured.dll" ".\Scarface.GenericFix\Detoured.dll"
copy "%DXSDK_DIR%\Utilities\bin\x86\PIXHelper.dll" ".\Scarface.GenericFix\PIXHelper.dll"
copy "%DXSDK_DIR%\Utilities\bin\x86\PIXWin.exe" ".\Scarface.GenericFix\PIXWin.exe"

rem Creating archives

FOR /d %%X IN (*) DO (
7za a -tzip "Archives\%%X.zip" "%%X\" -r -xr^^!Archives -x^^!*.pdb -x^^!*.db -x^^!*.ipdb -x^^!*.iobj -x^^!*.tmp -x^^!*.iobj -x^^!*.ual -x^^!*.iobj -x^^!*.wrapper
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

