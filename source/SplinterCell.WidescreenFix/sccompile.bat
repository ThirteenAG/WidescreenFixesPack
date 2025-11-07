@echo off
setlocal enabledelayedexpansion

if "%~3"=="" (
    echo Usage: sccompile.bat "password" "input_folder" "output_folder"
    exit /b 1
)

set "PASSWORD=%~1"
set "INPUT_FOLDER=%~2"
set "OUTPUT_FOLDER=%~3"
set "ARCHIVE=sccompile.bin"

:: Create output folder if it doesn't exist
if not exist "%OUTPUT_FOLDER%" mkdir "%OUTPUT_FOLDER%"

:: Unpack 7z directly to output folder
7z x -p%PASSWORD% "%ARCHIVE%" -o"%OUTPUT_FOLDER%" >nul 2>&1
if errorlevel 1 (
    echo Failed to unpack archive.
    exit /b 1
)

:: Copy all contents from input folder to output folder (overwrite)
xcopy /e /i /h /y "%INPUT_FOLDER%" "%OUTPUT_FOLDER%\" >nul 2>&1

:: Change to output folder and compile
cd /d "%OUTPUT_FOLDER%"

if not exist .\Core\Inc\ mkdir .\Core\Inc
if not exist .\Echelon\Inc\ mkdir .\Echelon\Inc
if not exist .\EchelonCharacter\Inc\ mkdir .\EchelonCharacter\Inc
if not exist .\EchelonEffect\Inc\ mkdir .\EchelonEffect\Inc
if not exist .\EchelonGameObject\Inc\ mkdir .\EchelonGameObject\Inc
if not exist .\EchelonHUD\Inc\ mkdir .\EchelonHUD\Inc
if not exist .\EchelonIngredient\Inc\ mkdir .\EchelonIngredient\Inc
if not exist .\EchelonMenus\Inc\ mkdir .\EchelonMenus\Inc
if not exist .\EchelonPattern\Inc\ mkdir .\EchelonPattern\Inc
if not exist .\Editor\Inc\ mkdir .\Editor\Inc
if not exist .\Engine\Inc\ mkdir .\Engine\Inc
if not exist .\UDebugMenu\Inc\ mkdir .\UDebugMenu\Inc
if not exist .\UWindow\Inc\ mkdir .\UWindow\Inc

cd .\System\
UCC.exe make -nobind
cd ..

:: Delete all files that are not .u (keeping only compiled .u files)
for /r . %%f in (*) do (
    if /i not "%%~xf"==".u" del "%%f" 2>nul
)

:: Remove empty directories (sorted reverse to handle nested empties)
for /f "delims=" %%d in ('dir /ad /b /s . ^| sort /r') do rd "%%d" 2>nul

echo Done.