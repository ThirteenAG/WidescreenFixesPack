@echo off
powershell -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0tools\releases\ReleaseGui.ps1" %*
exit /b %errorlevel%
