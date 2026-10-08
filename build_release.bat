@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\packaging\Release.ps1" -Mode as-ready -Signing off %*
exit /b %errorlevel%
