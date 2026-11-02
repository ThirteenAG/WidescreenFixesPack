@echo off
rem Regenerates the vehicle light packs from replacements\ (run manually; the build never runs this).
rem Writes source\GTAVCS.PCSX2F.FusionFix\LightPack.hpp and data\GTAVCS.PPSSPP.FusionFix\...\lights.bin, lights-hd.bin.
rem Needs Python 3 with numpy. VCS native sizes come from the carlist dumps when present (see pack.py --carlist).
cd /d "%~dp0..\.."
python tools\vehicle-lights\pack.py --game GTAVCS %*
if errorlevel 1 pause
