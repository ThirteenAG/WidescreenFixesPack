@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\packaging\Download.ps1" %*
exit /b %errorlevel%
