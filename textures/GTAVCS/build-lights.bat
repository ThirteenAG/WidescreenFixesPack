@echo off
rem Regenerates the ImVehLM light packs from replacements\ (run manually; the build never runs this).
rem Writes source\GTAVCS.PCSX2F.ImVehLM\LightPack.hpp and data\GTAVCS.PPSSPP.ImVehLM\...\lights.bin, lights-hd.bin.
rem Needs Python 3 with numpy. VCS native sizes come from the carlist dumps when present (see pack.py --carlist).
cd /d "%~dp0..\.."
python tools\vehicle-lights\pack.py --game GTAVCS %*
if errorlevel 1 pause
